/*
*********************************************************************************************************
*                                          MODBUS COMMUNICATION
*                                              SLAVE MODULE
*
*
*                           (c) Copyright 2019; XiaoJSoft Studio.;
*                    All rights reserved.  Protected by international copyright laws.
*
* File      : MBSLAVE_CMDLET_READCOILS.C
* Version   : V1.0.320
* By        : Ji WenCong
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define MB_SOURCE
#define MBSLAVE_SOURCE
#define MBSLAVE_CMDLET_READCOILS_SOURCE

#include <mbslave_cmdlet_readcoils.h>
#include <mbslave_cfg.h>

#include <mb_bufferemitter.h>
#include <mb_bufferfetcher.h>
#include <mb_constants.h>

#include <mb_types.h>

#include <cpu.h>

#include <lib_def.h>


#if (MB_CFG_SLAVE_EN == DEF_ENABLED) && (MB_CFG_SLAVE_BUILTIN_CMDLET_READCOILS == DEF_ENABLED)

/*
*********************************************************************************************************
*                                    MBSlave_CmdLet_ReadCoils()
*
* Description : Command implementation of "Read Coils (0x01)".
*
* Argument(s) : (1) request_fncode        Function code of the request.
*               (2) p_request_data        Pointer to the first element of the request data.
*               (2) request_data_size     Size(length) of the request data.
*               (2) p_response_fncode     Pointer to the variable that receives the function code of the response.
*               (2) p_response_buffer     Pointer to the first element of the response data buffer.
*               (2) response_buffer_size  Size of the response data buffer.
*               (2) p_response_data_size  Pointer to the variable that receives the size of the response data.
*               (2) p_cmdlet_ctx          Pointer to the command-specific context.
*               (4) p_error               Pointer to the variable that receives error code from this function:
*
*                                             MB_ERROR_NONE                    No error occurred.
*                                             MB_ERROR_NULLREFERENCE           One of following conditions occurred:
*
*                                                                                  (1) 'p_request_data' is NULL while 'request_data_size' is not zero.
*                                                                                  (2) 'p_response_fncode' is NULL.
*                                                                                  (3) 'p_response_buffer' is NULL while 'response_buffer_size' is not zero.
*                                                                                  (4) 'p_response_data_size' is NULL.
*                                                                                  (5) 'p_cmdlet_ctx' is NULL.
*
*                                             MB_ERROR_SLAVE_REQUESTTRUNCATED  Request data is truncated.
*                                             MB_ERROR_SLAVE_RESPONSETRUNCATED Response data buffer is too small.
*                                             MB_ERROR_SLAVE_CALLBACKFAILED    Error occurred while calling callbacks.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is NOT thread(task)-safe.
*               (2) The 'p_cmdlet_ctx' pointer MUST point to a MBSLAVE_READCOILS_CTX object.
*********************************************************************************************************
*/

void MBSlave_CmdLet_ReadCoils(
    CPU_INT08U               request_fncode,
    CPU_INT08U              *p_request_data,
    CPU_SIZE_T               request_data_size,
    CPU_INT08U              *p_response_fncode,
    CPU_INT08U              *p_response_buffer,
    CPU_SIZE_T               response_buffer_size,
    CPU_SIZE_T              *p_response_data_size,
    void                    *p_cmdlet_ctx,
    MB_ERROR                *p_error
) {
    MBSLAVE_READCOILS_CTX   *cmdlet_ctx;

    MB_BUFFEREMITTER         emitter;
    MB_BUFFERFETCHER         fetcher;

    CPU_INT16U               coilStartAddress;
    CPU_INT16U               coilEndAddress;
    CPU_INT16U               coilCurrentAddress;
    CPU_INT16U               coilQuantity;
    CPU_BOOLEAN              coilStatus;

    CPU_INT16U               outByteCount;

    CPU_INT08U               duffNbr;
    CPU_INT08U               duffBitMask;
    CPU_INT08U               duffByte;

    CPU_INT08U               ec;

    CPU_BOOLEAN              validity;

#if (MB_CFG_ARG_CHK_EN == DEF_ENABLED)
    /*  Check 'p_request_data' parameter.  */
    if (
        (p_request_data == (CPU_INT08U*)0) && 
        (request_data_size != (CPU_SIZE_T)0U)
    ) {
        *p_error = MB_ERROR_NULLREFERENCE;
        return;
    }

    /*  Check 'p_response_fncode' parameter.  */
    if (p_response_fncode == (CPU_INT08U*)0) {
        *p_error = MB_ERROR_NULLREFERENCE;
        return;
    }

    /*  Check 'p_response_buffer' parameter.  */
    if (
        (p_response_buffer == (CPU_INT08U*)0) && 
        (response_buffer_size != (CPU_SIZE_T)0U)
    ) {
        *p_error = MB_ERROR_NULLREFERENCE;
        return;
    }

    /*  Check 'p_response_data_size' parameter.  */
    if (p_response_data_size == (CPU_SIZE_T*)0) {
        *p_error = MB_ERROR_NULLREFERENCE;
        return;
    }

    /*  Check 'p_cmdlet_ctx' parameter.  */
    if (p_cmdlet_ctx == (void*)0) {
        *p_error = MB_ERROR_NULLREFERENCE;
        return;
    }
#endif

    /*  No error by default.  */
    *p_error = MB_ERROR_NONE;

    /*  Type cast.  */
    cmdlet_ctx = (MBSLAVE_READCOILS_CTX*)p_cmdlet_ctx;

    /*  Initialize the response data emitter.  */
    MBBufEmitter_Initialize(
        &(emitter),
        p_response_buffer,
        response_buffer_size,
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        return;
    }

    /*  Initialize the request data fetcher.  */
    MBBufFetcher_Initialize(
        &(fetcher),
        p_request_data,
        request_data_size,
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        return;
    }

    /*  Read the start address from the request data.  */
    coilStartAddress = MBBufFetcher_ReadUInt16BE(
        &(fetcher),
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        if (*p_error == MB_ERROR_BUFFETCHER_BUFFEREND) {
            *p_error = MB_ERROR_SLAVE_REQUESTTRUNCATED;
        }
        return;
    }

    /*  Read the coil quantity from the request data.  */
    coilQuantity = MBBufFetcher_ReadUInt16BE(
        &(fetcher),
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        if (*p_error == MB_ERROR_BUFFETCHER_BUFFEREND) {
            *p_error = MB_ERROR_SLAVE_REQUESTTRUNCATED;
        }
        return;
    }

    /*  Validate the coil quantity.  */
    if (coilQuantity == (CPU_INT16U)0x0000U || coilQuantity > (CPU_INT16U)0x07D0U) {
        goto MBSLAVE_RDCOILS_CATCH_INVALIDVALUE;
    }

    /*  Validate the start address.  */
    validity = cmdlet_ctx->cbValidateCoil(
        coilStartAddress, 
        cmdlet_ctx->cbArg, 
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        *p_error = MB_ERROR_SLAVE_CALLBACKFAILED;
        return;
    }
    if (!validity) {
        goto MBSLAVE_RDCOILS_CATCH_INVALIDADDR;
    }

    /*  Validate the end address.  */
    coilEndAddress = (CPU_INT16U)(coilStartAddress + coilQuantity - (CPU_INT16U)1U);
    if (coilEndAddress < coilStartAddress) {
        /*  An overflow error detected.  */
        goto MBSLAVE_RDCOILS_CATCH_INVALIDADDR;
    }
    validity = cmdlet_ctx->cbValidateCoil(
        coilEndAddress, 
        cmdlet_ctx->cbArg, 
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        *p_error = MB_ERROR_SLAVE_CALLBACKFAILED;
        return;
    }
    if (!validity) {
        goto MBSLAVE_RDCOILS_CATCH_INVALIDADDR;
    }

    /*  Write the count of output bytes.  */
    outByteCount = coilQuantity;
    --(outByteCount);
    outByteCount /= (CPU_INT16U)8U;
    ++(outByteCount);
    MBBufEmitter_WriteUInt8(
        &(emitter),
        (CPU_INT08U)outByteCount,
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        if (*p_error == MB_ERROR_BUFEMITTER_BUFFEREND) {
            *p_error = MB_ERROR_SLAVE_RESPONSETRUNCATED;
        }
        return;
    }

    /*  Process the request.  */
    coilCurrentAddress = coilStartAddress;
    while (coilQuantity != (CPU_INT16U)0U) {
        /*  Initialize the Duff's device.  */
        duffByte = (CPU_INT08U)0x00U;
        duffBitMask = (CPU_INT08U)0x01U;
        if (coilQuantity > (CPU_INT16U)8U) {
            duffNbr = (CPU_INT08U)8U;
        } else {
            duffNbr = (CPU_INT08U)coilQuantity;
        }

#define MBSLAVE_RDCOILS_DUFFUNIT()  {                        \
            /*  Read coil status.  */                        \
            coilStatus = cmdlet_ctx->cbReadCoil(             \
                coilCurrentAddress,                          \
                cmdlet_ctx->cbArg,                           \
                p_error                                      \
            );                                               \
            if (*p_error != MB_ERROR_NONE) {                 \
                *p_error = MB_ERROR_SLAVE_CALLBACKFAILED;    \
                return;                                      \
            }                                                \
                                                             \
            /*  Write to the target bit.  */                 \
            if (coilStatus) {                                \
                duffByte |= duffBitMask;                     \
            }                                                \
            duffBitMask <<= (CPU_INT08U)1U;                  \
                                                             \
            /*  Decrease the coil quantity.  */              \
            --(coilQuantity);                                \
                                                             \
            /*  Move to the next coil.  */                   \
            ++(coilCurrentAddress);                          \
}

        /*  Calculate the value of the output byte.  */
        switch (duffNbr) {
            case ((CPU_INT08U)8U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)7U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)6U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)5U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)4U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)3U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)2U):
                MBSLAVE_RDCOILS_DUFFUNIT();
            case ((CPU_INT08U)1U):
                MBSLAVE_RDCOILS_DUFFUNIT();
        }

#undef MBSLAVE_RDCOILS_DUFFUNIT

        /*  Write the output byte.  */
        MBBufEmitter_WriteUInt8(
            &(emitter),
            duffByte,
            p_error
        );
        if (*p_error != MB_ERROR_NONE) {
            if (*p_error == MB_ERROR_BUFEMITTER_BUFFEREND) {
                *p_error = MB_ERROR_SLAVE_RESPONSETRUNCATED;
            }
            return;
        }
    }

    /*  Write the function code.  */
    *p_response_fncode = request_fncode;

    goto MBSLAVE_RDCOILS_FINALLY;

MBSLAVE_RDCOILS_CATCH_INVALIDVALUE:
    ec = MB_APUEC_ILLEGALDATAVALUE;
    goto MBSLAVE_RDCOILS_CATCH_FINALLY;

MBSLAVE_RDCOILS_CATCH_INVALIDADDR:
    ec = MB_APUEC_ILLEGALDATAADDRESS;

MBSLAVE_RDCOILS_CATCH_FINALLY:
    /*  Write the function code.  */
    *p_response_fncode = (CPU_INT08U)(request_fncode + (CPU_INT08U)0x80U);

    /*  Discard emitted bytes.  */
    MBBufEmitter_Reset(
        &(emitter),
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        return;
    }

    /*  Write the exception code.  */
    MBBufEmitter_WriteUInt8(
        &(emitter),
        ec,
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        if (*p_error == MB_ERROR_BUFEMITTER_BUFFEREND) {
            *p_error = MB_ERROR_SLAVE_RESPONSETRUNCATED;
        }
        return;
    }

MBSLAVE_RDCOILS_FINALLY:
    /*  Get the length of the response data.  */
    *p_response_data_size = MBBufEmitter_GetWrittenLength(
        &(emitter),
        p_error
    );
    if (*p_error != MB_ERROR_NONE) {
        return;
    }
}

#endif  /*  #if (MB_CFG_SLAVE_EN == DEF_ENABLED) && (MB_CFG_SLAVE_BUILTIN_CMDLET_READCOILS == DEF_ENABLED)  */
