/*
 * TI's FM Stack
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
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


#ifndef __FM_RX_SM_I_H
#define __FM_RX_SM_I_H

#include "fm_rx_sm.h"


/**/
#define FM_RX_INTERNAL_HANDLE_GEN_INT			((FmRxCmdType)FM_RX_LAST_API_CMD+1)	/* Internal use */
#define FM_RX_INTERNAL_READ_RDS 					((FmRxCmdType)FM_RX_LAST_API_CMD+2)	/* Internal use */
#define FM_RX_INTERNAL_HANDLE_AF_JUMP			((FmRxCmdType)FM_RX_LAST_API_CMD+3)	/* Internal use */

#define FM_RX_INTERNAL_HANDLE_STEREO_CHANGE 	((FmRxCmdType)FM_RX_LAST_API_CMD+4)
#define FM_RX_INTERNAL_HANDLE_HW_MAL			((FmRxCmdType)FM_RX_LAST_API_CMD+5)	/* Internal use */
#define FM_RX_INTERNAL_HANDLE_TIMEOUT			((FmRxCmdType)FM_RX_LAST_API_CMD+6)	/* Internal use */

#define FM_RX_LAST_CMD								((FmRxCmdType)FM_RX_INTERNAL_HANDLE_TIMEOUT+1)	

/********************************************************************************
 *
 * Types
 *
 *******************************************************************************/





/***********************************************************
*
*	RDS related definitions
*
***********************************************************/
/*
|	 block A		   |	block B 													   |   block C				 |	 block D		| 
|	 16it	 |8-bit   |    4-bit		|1-bit	|1-bit |5-bit| 5 -bit|8-bit   |    16 -bit	 |8-bit   |    16-bit|8-bit | 
|	PI code|c.w 	|	group type|B type|tp	 |pty	|other	|c.w	 |	 PI code   |c.w 	|	PI code|c.w  |
*/
typedef struct _FmRxRdsDataFormat {
	FmcRdsPiCode piCode;
	FmcRdsGroupTypeMask groupBitInMask;
	FmcRdsPtyCode ptyCode;
	FMC_U8 dataIndex;
	
	union {
		struct {
			FMC_U8 rdsBuff[8];
			} groupDataBuff;
		struct {
			FmcRdsPiCode piData;
			FMC_U8		blockB_byte1;
			FMC_U8		blockB_byte2;
			FMC_U8		blockC_byte1;
			FMC_U8		blockC_byte2;
			FMC_U8		blockD_byte1;
			FMC_U8		blockD_byte2;
			} groupGeneral;
		struct {
			FmcRdsPiCode piData;
			FMC_U8		blockB_byte1;
			FMC_U8		blockB_byte2;
			FMC_U8		firstAf;
			FMC_U8		secondAf;
			FMC_U8		firstPsByte;
			FMC_U8		secondPsByte;
			} group0A;
		
		struct {
			FmcRdsPiCode piData;
			FMC_U8		blockB_byte1;
			FMC_U8		blockB_byte2;
			FmcRdsPiCode piData2;
			FMC_U8		firstPsByte;
			FMC_U8		secondPsByte;
		} group0B;
	
		struct {
			FmcRdsPiCode piData;
			FMC_U8		blockB_byte1;
			FMC_U8		blockB_byte2;
			FMC_U8		firstRtByte;
			FMC_U8		secondRtByte;
			FMC_U8		thirdRtByte;
			FMC_U8		fourthRtByte;
		} group2A;
		struct {
			FmcRdsPiCode piData;
			FMC_U8		blockB_byte1;
			FMC_U8		blockB_byte2;
			FmcRdsPiCode piData2;
			FMC_U8		firstRtByte;
			FMC_U8		secondRtByte;
		} group2B;
	} rdsData;	
} FmRxRdsDataFormat;

/***********************************************************
*
*	
*
***********************************************************/

#define RDS_NUM_TUPLES				64
#define RDS_BLOCK_SIZE				3
#define RDS_BLOCKS_IN_GROUP			4
#define RDS_GROUP_SIZE				((RDS_BLOCK_SIZE) * (RDS_BLOCKS_IN_GROUP))
#define RDS_PS_NAME_SIZE			8
#define RDS_PS_NAME_LAST_INDEX		((RDS_PS_NAME_SIZE) / 2)
#define RDS_RADIO_TEXT_SIZE			64
#define RDS_RT_LAST_INDEX			((RDS_RADIO_TEXT_SIZE) / 4) /*[ToDo Zvi] This number is depended on 2A/2B*/

#define RDS_BLOCK_A		0
#define RDS_BLOCK_B		1
#define RDS_BLOCK_C		2
#define RDS_BLOCK_Ctag	3
#define RDS_BLOCK_D		4
#define RDS_BLOCK_E		5

#define RDS_BLOCK_INDEX_A		0
#define RDS_BLOCK_INDEX_B		1
#define RDS_BLOCK_INDEX_C		2
#define RDS_BLOCK_INDEX_D		3
#define RDS_BLOCK_INDEX_UNKNOWN	0xF0

#define RDS_NEXT_PS_INDEX_RESET				9	/* Bigger than the max index */
#define RDS_NEXT_RT_INDEX_RESET				0	/* Bigger than the max index *//*[ToDo Zvi] This number is depended on 2A/2B*/
#define RDS_PREV_RT_AB_FLAG_RESET			2	/* AB flag can be 0 or 1 */

#define RDS_BLOCK_A_PI_INDEX				0
#define RDS_BLOCK_A_STATUS_INDEX			2
#define RDS_BLOCK_B_GROUP_TYPE_INDEX		3
#define RDS_BLOCK_B_STATUS_INDEX			5
#define RDS_BLOCK_C_AF1						6
#define RDS_BLOCK_C_AF2						7
#define RDS_BLOCK_C_STATUS_INDEX			8
#define RDS_BLOCK_D_PS1_INDEX				9
#define RDS_BLOCK_D_PS2_INDEX				10
#define RDS_BLOCK_D_STATUS_INDEX			11
#define RDS_BLOCK_C_RT1_INDEX				6
#define RDS_BLOCK_C_RT2_INDEX				7
#define RDS_BLOCK_D_RT1_INDEX				9
#define RDS_BLOCK_D_RT2_INDEX				10

#define RDS_BIT_0_TO_BIT_3		0x0f
#define RDS_BIT_4_TO_BIT_7		0xf0

#define RDS_STATUS_ERROR_MASK				0x18
#define RDS_BLOCK_B_GROUP_TYPE_MASK			0xF0
#define RDS_BLOCK_B_PTY_MASK 					0x03E0
#define RDS_BLOCK_B_AB_BIT_MASK				0x0800
#define RDS_BLOCK_B_PS_INDEX_MASK			0x0003
#define RDS_BLOCK_B_RT_INDEX_MASK			0x000F
#define RDS_BLOCK_B_AB_FLAG_MASK			0x0010

#define RDS_NUM_OF_BITS_BEFOR_PTY	5

#define RDS_RT_END_CHARACTER				0x0D	/* indicates carriage return	*/

#define RDS_GROUP_TYPE_0					0
#define RDS_GROUP_TYPE_2					2

#define RDS_MIN_AF			1
#define RDS_MAX_AF			204
#define RDS_MAX_AF_JAPAN	140
#define RDS_1_AF_FOLLOWS	225
#define RDS_25_AF_FOLLOWS	249
#define RDS_MAX_AF_LIST		25
#define RDS_RAW_GROUP_DATA_LEN		8
/***********************************************************
*
*	Interrupt related definitions
*
***********************************************************/
#define GEN_INT_MAL_STAGE						0
#define GEN_INT_AFTER_MAL_STAGE 				1
#define GEN_INT_AFTER_STEREO_CHANGE_STAGE		2
#define GEN_INT_AFTER_RDS_STAGE 				3
#define GEN_INT_AFTER_LOW_RSSI_STAGE			4
#define GEN_INT_AFTER_FINISH_STAGE				5

#define INT_READ_MASK		0
#define INT_READ_FLAG		1
#define INT_HANDLE_INT		2

/* Interrupts handling states */
#define INT_STATE_NONE								0
#define INT_STATE_WAIT_FOR_CMD_CMPLT				1
#define INT_STATE_READ_INT							2


typedef struct {
	FMC_U16 	gen_int_mask;
	FMC_U16 	opHandler_int_mask;
	FMC_BOOL	interruptInd;	/* an interrupt was received */
	FMC_U16 	fmFlag; 		/* fm interrupts flag */
	FMC_U16 	fmMask; 		/* fm interrupts mask */
	FMC_U16 	genIntSetBits;	/* The actual interrupts that happened saved for the general int */
	FMC_U16 	opHandlerIntSetBits; /* The actual interrupts that happened saved for the opHandler int */
	
	FMC_BOOL	intReadEvt; 	/* Indication that interrupt flag was read */
	FMC_U8		readIntState;	/* states for interrupts handling */ 
	
} FmRxInterruptInfo;

/***********************************************************
*
*	bbb
*
***********************************************************/
typedef struct {
    FMC_U8 last_block_index;
	FMC_U8 rdsGroup[RDS_GROUP_SIZE];  /* Contains a whole group */
	FMC_U8 nextPsIndex;
	FMC_U8 psNameSameCount;
	FMC_U8 psName[RDS_PS_NAME_SIZE+1];
	FMC_U8 prevABFlag;
	FMC_BOOL abFlagChanged;
	FMC_U8 nextRtIndex;
	FMC_U8 radioText[RDS_RADIO_TEXT_SIZE+1];
	FMC_U8 rtLength;
	FMC_U8 numOfRtBytesInGroup;
	FMC_BOOL wasRepertoireUpdated;
} RdsParams;

#define NO_PI_CODE			0
typedef struct {

	FMC_U16 piCode;
	FMC_U8 psName[RDS_PS_NAME_SIZE+1];
	FmcFreq afList[RDS_MAX_AF_LIST];
	FMC_U8 afListSize;
	FMC_U8 afListCurMaxSize;  /* The number of af we are expecting to receive */
} TunedStationParams;



/* This flag indicates the read command complete type*/
typedef enum
{	
	FM_RX_NONE,
	FM_RX_READING_MASK_REGISTER,	
	FM_RX_READING_STATUS_REGISTER,
	FM_RX_READING_RDS_DATA,
} FmRxSmReadState;


#define INCREMENT_STAGE		0xFF

typedef enum
{	
	FM_RX_SM_CALL_REASON_NONE,
	FM_RX_SM_CALL_REASON_START,
	FM_RX_SM_CALL_REASON_CMD_CMPLT_EVT,
	FM_RX_SM_CALL_REASON_INTERRUPT,
	FM_RX_SM_CALL_REASON_UPPER_EVT
} FmRxSmCallReason;



/********************************************************************************/


#endif /*__FM_RX_SM__I_H*/
