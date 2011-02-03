/*
 * version.h
 *
 * Copyright(c) 1998 - 2010 Texas Instruments. All rights reserved.      
 * All rights reserved.                                                  
 *                                                                       
 * Redistribution and use in source and binary forms, with or without    
 * modification, are permitted provided that the following conditions    
 * are met:                                                              
 *                                                                       
 *  * Redistributions of source code must retain the above copyright     
 *    notice, this list of conditions and the following disclaimer.      
 *  * Redistributions in binary form must reproduce the above copyright  
 *    notice, this list of conditions and the following disclaimer in    
 *    the documentation and/or other materials provided with the         
 *    distribution.                                                      
 *  * Neither the name Texas Instruments nor the names of its            
 *    contributors may be used to endorse or promote products derived    
 *    from this software without specific prior written permission.      
 *                                                                       
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT      
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT   
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

                                                                                                             
/**************************************************************************/                                                                                                             
/*                                                                        */                                                                                                             
/*   MODULE:  version.h                                                   */                                                                                                             
/*   PURPOSE: release specific definitions file.                          */                                                                                                             
/*                                                                        */                                                                                                             
/**************************************************************************/                                                                                                             
                                                                                                             
                                                                                                             
#ifndef _VERSION_H_                                                                                                             
#define _VERSION_H_                                                                                                             

#include "TWDriver.h"

#define MCP_WL7_VERSION_STR "MCP-WL_2.5.0.10"

#define SW_VERSION_STR      "MCP-WiLink_Driver_2.5.3.0.18"
/*
based on WiLink 6.1.0.0.144
*/

#define SW_RELEASE_MONTH    01
#define SW_RELEASE_DAY      26

/*
Navilink version.  Navilink/inc/version.h
*/

#define GPS_SW_VERSION_STR "NaviLink_MCP2.5_RC1.9"
/*
BT version.     MCP_Common/Platform/bthal/LINUX/android_zoom2/inc/EBTIPS_version.h
*/

#define BT_SW_VERSION_STR "BTIPS 2.24.0.6"


#define FM_SW_VERSION_STR "L2.00.4"

/*
BSP version
*/
#define BSP_VERSION_STR "Android L25.inc.3.4.p2_2.5.0.9 Froyo"

#endif /* _VERSION_H_ */

TI_BOOL version_fwDriverMatch(TI_UINT8* fwVersion, TI_UINT16* pFwVersionOutput);
TI_BOOL version_IniDriverMatch(TI_UINT16 iniVersion);
