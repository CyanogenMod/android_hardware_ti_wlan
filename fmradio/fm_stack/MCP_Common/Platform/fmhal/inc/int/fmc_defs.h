/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and  
 * limitations under the License.
 */
/*******************************************************************************\
*
*   FILE NAME:      fmc_defs.h
*
*   DESCRIPTION:    TBD
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/


#ifndef __FMC_DEFS_H
#define __FMC_DEFS_H


/********************************************************************************
 *
 * Include files
 *
 *******************************************************************************/
#include "fmc_types.h"
#include "fmc_log.h"


/*APP Error Callback*/
#define FMC_APP_CB  FMC_OS_APPCB()

#define FMC_UNUSED_PARAMETER(_PARM)     ((_PARM) = (_PARM))
 
/* [ToDo] wrap this definition with some flag */
#define FMC_STATIC      static

/*---------------------------------------------------------------------------
 *
 * Used to define unused function parameters in EBTIPS RELEASE configuration. Some compilers warn if this
 * is not done.
 */

 #ifdef EBTIPS_RELEASE
#define FMC_LOG_API_FUNCTION_ENTRY_EXIT     FMC_FALSE
#else
#define FMC_LOG_API_FUNCTION_ENTRY_EXIT     FMC_TRUE
#endif

#if (FMC_TRUE == FMC_LOG_API_FUNCTION_ENTRY_EXIT)

#define FMC_LOG_FUNCTION_ENTRY              FMC_LOG_FUNCTION(("Entered %s", _fmcDbgFuncName))                               
#define FMC_LOG_FUNCTION_EXIT               FMC_LOG_FUNCTION(("Exiting %s", _fmcDbgFuncName))                                           
#define FMC_LOG_DEFINE_FUNC_NAME(funcName)  const char* _fmcDbgFuncName = funcName

#else

#define FMC_LOG_FUNCTION_ENTRY
#define FMC_LOG_DEFINE_FUNC_NAME(funcName)
#define FMC_LOG_FUNCTION_EXIT

#endif

#define FMC_ASSERT_TREAT_ERROR_AS_FATAL     FMC_FALSE

#define FMC_ASSERT(condition)                       \
        (((condition) != 0) ? (void)0 : FMC_OS_Assert(#condition, __FILE__, (FMC_U16)__LINE__))


#if (FMC_TRUE == FMC_ASSERT_TREAT_ERROR_AS_FATAL)

#define FMC_ASSERT_ERROR(condition)                 \
            FMC_ASSERT(condition)

#else

#define FMC_ASSERT_ERROR(condition)

#endif

#define FMC_FUNC_START_AND_LOCK(funcName)       \
    FMC_LOG_DEFINE_FUNC_NAME(funcName);         \
    FMC_OS_LockSemaphore(FMCI_GetMutex());      \
    FMC_LOG_FUNCTION_ENTRY  

#define FMC_FUNC_END_AND_UNLOCK()               \
    goto CLEANUP;                               \
    CLEANUP:                                    \
    FMC_LOG_FUNCTION_EXIT;                      \
    FMC_OS_UnlockSemaphore(FMCI_GetMutex())


#define FMC_FUNC_START(funcName)            \
    FMC_LOG_DEFINE_FUNC_NAME(funcName);     \
    FMC_LOG_FUNCTION_ENTRY

#define FMC_FUNC_END()                      \
    goto CLEANUP;                           \
    CLEANUP:                                \
    FMC_LOG_FUNCTION_EXIT

#define FMC_VERIFY_ERR_NORET(condition, msg)    \
        if ((condition) == 0)                       \
        {                                       \
            FMC_LOG_ERROR(msg);             \
            FMC_ASSERT_ERROR(condition);        \
	    FMC_APP_CB ; \
        }
        
#define FMC_VERIFY_FATAL_NORET(condition, msg)      \
        if ((condition) == 0)                           \
        {                                           \
            FMC_LOG_FATAL(msg);                 \
            FMC_ASSERT(condition);                  \
	    FMC_APP_CB ; \
        }

#define FMC_VERIFY_ERR_NO_RETVAR(condition, msg)    \
        if ((condition) == 0)                       \
        {                                       \
            FMC_LOG_ERROR(msg);             \
            FMC_ASSERT_ERROR(condition);        \
            goto CLEANUP;                       \
        }
        
#define FMC_VERIFY_FATAL_NO_RETVAR(condition, msg)      \
        if ((condition) == 0)                           \
        {                                           \
            FMC_LOG_FATAL(msg);                 \
            FMC_ASSERT(condition);                  \
	    FMC_APP_CB ; \
            goto CLEANUP;                           \
        }

#define FMC_ERR_NORET(msg)                          \
            FMC_LOG_ERROR(msg);                 \
            FMC_ASSERT_ERROR(0);		\
	    FMC_APP_CB ;
        
#define FMC_FATAL_NORET(msg)                        \
            FMC_LOG_FATAL(msg);                 \
            FMC_ASSERT(0);		\
            FMC_APP_CB ;

#define FMC_RET_SET_RETVAR(setRetVarExp)            \
    (setRetVarExp);                                 \
    goto CLEANUP
    
#define FMC_VERIFY_ERR_SET_RETVAR(condition, setRetVarExp, msg)     \
        if ((condition) == 0)                                           \
        {                                                           \
            FMC_LOG_ERROR(msg);                                 \
            (setRetVarExp);                                         \
            FMC_ASSERT_ERROR(condition);                            \
            FMC_APP_CB ; \
            goto CLEANUP;                                           \
        }           

#define FMC_VERIFY_FATAL_SET_RETVAR(condition, setRetVarExp, msg)   \
        if ((condition) == 0)                       \
        {                                       \
            FMC_LOG_FATAL(msg);             \
            (setRetVarExp);                     \
            FMC_ASSERT(condition);              \
            FMC_APP_CB ;	\
            goto CLEANUP;                       \
        }

#define FMC_ERR_SET_RETVAR(setRetVarExp, msg)       \
        FMC_LOG_ERROR(msg);                     \
        (setRetVarExp);                             \
        FMC_ASSERT_ERROR(0);                        \
        FMC_APP_CB ;		\
        goto CLEANUP;

#define FMC_FATAL_SET_RETVAR(setRetVarExp, msg) \
            FMC_LOG_FATAL(msg);                 \
            (setRetVarExp);                         \
            FMC_ASSERT(0);                          \
            FMC_APP_CB ; 		\
            goto CLEANUP;


#define FMC_RET(returnCode)                             \
        FMC_RET_SET_RETVAR(status = returnCode)

#define FMC_RET_NO_RETVAR()                         \
        FMC_RET_SET_RETVAR(status = status)

#define FMC_VERIFY_ERR(condition, returnCode, msg)      \
        FMC_VERIFY_ERR_SET_RETVAR(condition, (status = (returnCode)), msg)

 #define FMC_VERIFY_FATAL(condition, returnCode, msg)   \
        FMC_VERIFY_FATAL_SET_RETVAR(condition, (status = (returnCode)),  msg)

#define FMC_ERR(returnCode, msg)        \
        FMC_ERR_SET_RETVAR((status = (returnCode)), msg)

 #define FMC_FATAL(returnCode, msg) \
        FMC_FATAL_SET_RETVAR((status = (returnCode)),  msg)

#define FMC_ERR_NO_RETVAR(msg)                  \
            FMC_LOG_ERROR(msg);             \
            FMC_ASSERT_ERROR(condition);            \
	    FMC_APP_CB ; 		\
            goto CLEANUP;

#define FMC_FATAL_NO_RETVAR(msg)                \
            FMC_LOG_FATAL(msg);             \
            FMC_ASSERT(0);                      \
	    FMC_APP_CB ; 	\
            goto CLEANUP;


/*-------------------------------------------------------------------------------
 * FmcStatus Type
 *
 *  Status codes have 3 ranges:
 *  1. Common codes
 *  2. RX-specific codes
 *  3. Tx-specific codes
 *
 *  The common codes are defined in this file. Rx-specific codes are defined in fm_rx.h and
 *  Tx-specific codes in fm_tx.h
 *  
 *  For consistency, the common codes are duplicated in RX & TX. They have the same value
 *  and the same meaning as the corresponding common code. That way when a status code 
 *  0 (for example) is returned form any FM stack function (Rx / Tx / Common) it always means SUCCESS.
 *
 *  Codes that are specific to Rx start from FMC_FIRST_FM_RX_STATUS_CODE
 *  Codes that are specific to Tx start from FMC_FIRST_FM_TX_STATUS_CODE
 */
typedef FMC_UINT FmcStatus;

#define FMC_STATUS_SUCCESS                          ((FmcStatus)0)
#define FMC_STATUS_FAILED                               ((FmcStatus)1)
#define FMC_STATUS_PENDING                              ((FmcStatus)2)
#define FMC_STATUS_INVALID_PARM                     ((FmcStatus)3)
#define FMC_STATUS_IN_PROGRESS                      ((FmcStatus)4)
#define FMC_STATUS_NOT_APPLICABLE                       ((FmcStatus)5)
#define FMC_STATUS_NOT_SUPPORTED                        ((FmcStatus)6)
#define FMC_STATUS_INTERNAL_ERROR                       ((FmcStatus)7)
#define FMC_STATUS_TRANSPORT_INIT_ERR                   ((FmcStatus)8)
#define FMC_STATUS_HARDWARE_ERR                     ((FmcStatus)9)
#define FMC_STATUS_NO_VALUE_AVAILABLE               ((FmcStatus)10)
#define FMC_STATUS_CONTEXT_DOESNT_EXIST             ((FmcStatus)11)
#define FMC_STATUS_CONTEXT_NOT_DESTROYED            ((FmcStatus)12)
#define FMC_STATUS_CONTEXT_NOT_ENABLED              ((FmcStatus)13)
#define FMC_STATUS_CONTEXT_NOT_DISABLED             ((FmcStatus)14)
#define FMC_STATUS_NOT_DE_INITIALIZED                   ((FmcStatus)15)
#define FMC_STATUS_NOT_INITIALIZED                      ((FmcStatus)16)
#define FMC_STATUS_TOO_MANY_PENDING_CMDS            ((FmcStatus)17)
#define FMC_STATUS_DISABLING_IN_PROGRESS                ((FmcStatus)18)
#define FMC_STATUS_NO_RESOURCES                     ((FmcStatus)19)
#define FMC_STATUS_FM_COMMAND_FAILED                ((FmcStatus)20)
#define FMC_STATUS_SCRIPT_EXEC_FAILED                   ((FmcStatus)21)
#define FMC_STATUS_PROCESS_TIMEOUT_FAILURE                  ((FmcStatus)22)

#define FMC_STATUS_FAILED_BT_NOT_INITIALIZED                                ((FmcStatus)23)
#define FMC_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES            ((FmcStatus)24)
#define FMC_STATUS_LAST                             ((FmcStatus)FMC_STATUS_AUDIO_OPERATION_UNAVAILIBLE_RESOURCES)

#define FMC_FIRST_FM_RX_STATUS_CODE                 ((FmRxStatus)100)
#define FMC_FIRST_FM_TX_STATUS_CODE                 ((FmTxStatus)200)

#endif /* __FMC_DEFS_H */


