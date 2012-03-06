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

#ifndef LOGGER_H
#define LOGGER_H

#define LOGGER_VER_2

#ifndef LOGGER_VER_2
#error LOGGER_VER_2 should be defined while using Logger.cpp and .h
#endif

#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h> 

// Use pre-define symbol LOG_RELEASE if using this module in release mode.
// It allows direct calls to functions (not the standard macros)

///////////////////////////////////////////////////////
// Debug Levels definitions
///////////////////////////////////////////////////////
#define DL_INFO0        0
#define DL_INFO1        1
#define DL_INFO2        2
#define DL_INFO3        3
#define DL_INFO4        4
#define DL_INFO5        5
#define DL_INFO6        6
#define DL_INFO7        7
#define DL_INFO8        8
#define DL_INFO9        9
#define DL_INFO10       10

#define DL_NONE         DL_INFO0
#define DL_CRITICAL     DL_INFO1
#define DL_NORMAL       DL_INFO2
#define DL_INFO         DL_INFO3

#define MAX_MODULES     255

#define DEBUGWIN_DLL_NAME       ("Logger.DLL")

/*lint -printf(w5,Logger_LogString) */

#ifdef __cplusplus
void Logger_SetModuleName(LPCTSTR lpszModuleDebugName, int module= 0);
void Logger_LogString(int module, LPCTSTR lpszFileName, int nLine, int nLevel, LPCTSTR lpszFormat, ...);
void Logger_LogString(LPCTSTR lpszFormat, ...);

extern "C" {
#endif

    
void C_Logger_Trace(LPCTSTR lpszFormat, ...);
void C_Logger_Debug(int module, LPCTSTR lpszFileName, int nLine, int nLevel, LPCTSTR lpszFormat, ...);

#ifdef __cplusplus
}
#endif

// Old Functions Compatibility
#define SetModuleDebugName      Logger_SetModuleName
#define BFCSendDebugString      Logger_LogString

#ifndef LOGGER_NO_MACROS


#ifdef __cplusplus
#define LOG_COMMAND     ::Logger_LogString
#else
#define LOG_COMMAND     C_Logger_Debug
#endif

///////////////////////////////////////////////////////
// DEBUG0, DEBUG1, DEBUG2, DEBUG3 
///////////////////////////////////////////////////////
#ifdef _DEBUG

#ifndef DONT_UNDEF_MFC_TRACE

#ifdef TRACE
#undef TRACE
#endif
#ifdef TRACE0
#undef TRACE0
#endif
#ifdef TRACE1
#undef TRACE1
#endif
#ifdef TRACE2
#undef TRACE2
#endif
#ifdef TRACE3
#undef TRACE3
#endif
#ifdef ASSERT
#undef ASSERT
#endif

#define TRACE Logger_LogString
#define TRACE0(sz)              DEBUG0(DL_INFO9, sz)
#define TRACE1(sz, p1)          DEBUG1(DL_INFO9, sz, p1)
#define TRACE2(sz, p1, p2)      DEBUG2(DL_INFO9, sz, p1, p2)
#define TRACE3(sz, p1, p2, p3)  DEBUG3(DL_INFO9, sz, p1, p2, p3)
#define ASSERT(f) \
    do \
    { \
        if (!(f)) \
        {  \
            DEBUG2(DL_CRITICAL, _T("ASSERT Failed. File= <%s>, Line= <%d>"), __FILE__, (int)__LINE__); \
            if (AfxAssertFailedLine(THIS_FILE, __LINE__)) \
                AfxDebugBreak(); \
        } \
    } while (0) \

#endif // DONT_UNDEF_MFC_TRACE


//////////////////// DEBUG0-7 /////////////////////////
#define DEBUG0(level, sz) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, _T("%s"), sz)

#define DEBUG1(level, sz, p1) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1)

#define DEBUG2(level, sz, p1, p2) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2)

#define DEBUG3(level, sz, p1, p2, p3) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2, p3)

#define DEBUG4(level, sz, p1, p2, p3, p4) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2, p3, p4)

#define DEBUG5(level, sz, p1, p2, p3, p4, p5) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5)

#define DEBUG6(level, sz, p1, p2, p3, p4, p5, p6) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5, p6)

#define DEBUG7(level, sz, p1, p2, p3, p4, p5, p6, p7) \
    LOG_COMMAND(0, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5, p6, p7)

//////////////////// DEBUGX0-3 ////////////////////////

#define DEBUGX0(module, level, sz) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, _T("%s"), sz)

#define DEBUGX1(module, level, sz, p1) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1)

#define DEBUGX2(module, level, sz, p1, p2) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2)

#define DEBUGX3(module, level, sz, p1, p2, p3) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2, p3)

#define DEBUGX4(module, level, sz, p1, p2, p3, p4) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2, p3, p4)

#define DEBUGX5(module, level, sz, p1, p2, p3, p4, p5) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5)

#define DEBUGX6(module, level, sz, p1, p2, p3, p4, p5, p6) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5, p6)

#define DEBUGX7(module, level, sz, p1, p2, p3, p4, p5, p6, p7) \
    LOG_COMMAND(module, __FILE__, __LINE__, level, sz, p1, p2, p3, p4, p5, p6, p7)

#else

#define DEBUG0(level, sz)   
#define DEBUG1(level, sz, p1)   
#define DEBUG2(level, sz, p1, p2)   
#define DEBUG3(level, sz, p1, p2, p3)   
#define DEBUG4(level, sz, p1, p2, p3, p4)   
#define DEBUG5(level, sz, p1, p2, p3, p4, p5)   
#define DEBUG6(level, sz, p1, p2, p3, p4, p5, p6)   
#define DEBUG7(level, sz, p1, p2, p3, p4, p5, p6, p7)   

#define DEBUGX0(module, level, sz)  
#define DEBUGX1(module, level, sz, p1)  
#define DEBUGX2(module, level, sz, p1, p2)  
#define DEBUGX3(module, level, sz, p1, p2, p3)  
#define DEBUGX4(module, level, sz, p1, p2, p3, p4)  
#define DEBUGX5(module, level, sz, p1, p2, p3, p4, p5)  
#define DEBUGX6(module, level, sz, p1, p2, p3, p4, p5, p6)  
#define DEBUGX7(module, level, sz, p1, p2, p3, p4, p5, p6, p7)  

#endif // _DEBUG

#endif // LOGGER_NO_MACROS

#endif // LOGGER_H
