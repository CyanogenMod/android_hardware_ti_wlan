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
*   FILE NAME:	mcp_hal_fs.h
*
*   BRIEF:          	This file contains function prototypes, constant and type
*			   	definitions for MCP HAL FS module
*
*   DESCRIPTION:	The mcp_hal_fs.h is the fs API for the MCP HAL FS module. It holds the 
*				file operation that can be performed on files.
*
*   AUTHOR:         Udi Ron
*
\*******************************************************************************/

#ifndef __MCP_HAL_FS_H
#define __MCP_HAL_FS_H


#include "mcp_hal_types.h"
#include "mcp_hal_config.h"
#include "mcp_hal_defs.h"


/*-------------------------------------------------------------------------------
 *  McpHalFsStatus
 */
typedef enum tagMcpHalFsStatus
{
	MCP_HAL_FS_STATUS_SUCCESS =              								MCP_HAL_STATUS_SUCCESS,
	MCP_HAL_FS_STATUS_PENDING =              								MCP_HAL_STATUS_PENDING,

	/* FS-Specific codes */
	
	MCP_HAL_FS_STATUS_ERROR_GENERAL =   								MCP_HAL_STATUS_OPEN + 1,
	MCP_HAL_FS_STATUS_ERROR_NODEVICE =    							MCP_HAL_STATUS_OPEN + 2,
	MCP_HAL_FS_STATUS_ERROR_FS_CORRUPTED =							MCP_HAL_STATUS_OPEN + 3,
	MCP_HAL_FS_STATUS_ERROR_NOT_FORMATTED =						MCP_HAL_STATUS_OPEN + 4,
	MCP_HAL_FS_STATUS_ERROR_OUT_OF_SPACE =						MCP_HAL_STATUS_OPEN + 5,
	MCP_HAL_FS_STATUS_ERROR_FSFULL =       							MCP_HAL_STATUS_OPEN + 6,
	MCP_HAL_FS_STATUS_ERROR_BADNAME =    							MCP_HAL_STATUS_OPEN + 7,
	MCP_HAL_FS_STATUS_ERROR_NOTFOUND =    							MCP_HAL_STATUS_OPEN + 8,
	MCP_HAL_FS_STATUS_ERROR_EXISTS =       							MCP_HAL_STATUS_OPEN + 9,
	MCP_HAL_FS_STATUS_ERROR_ACCESS_DENIED  =						MCP_HAL_STATUS_OPEN + 10,
	MCP_HAL_FS_STATUS_ERROR_NAMETOOLONG =  						MCP_HAL_STATUS_OPEN + 11,
	MCP_HAL_FS_STATUS_ERROR_INVALID =      							MCP_HAL_STATUS_OPEN + 12,
	MCP_HAL_FS_STATUS_ERROR_DIRNOTEMPTY =  							MCP_HAL_STATUS_OPEN + 13,
	MCP_HAL_FS_STATUS_ERROR_FILETOOBIG =   							MCP_HAL_STATUS_OPEN + 14,
	MCP_HAL_FS_STATUS_ERROR_NOTAFILE =     							MCP_HAL_STATUS_OPEN + 15,
	MCP_HAL_FS_STATUS_ERROR_NUMFD =        							MCP_HAL_STATUS_OPEN + 16,
	MCP_HAL_FS_STATUS_ERROR_BADFD =        							MCP_HAL_STATUS_OPEN + 17,
	MCP_HAL_FS_STATUS_ERROR_LOCKED =       							MCP_HAL_STATUS_OPEN + 18,
	MCP_HAL_FS_STATUS_ERROR_NOT_IMPLEMENTED =						MCP_HAL_STATUS_OPEN + 19,
	MCP_HAL_FS_STATUS_ERROR_NOT_INITIALIZED =						MCP_HAL_STATUS_OPEN + 20,	
	MCP_HAL_FS_STATUS_ERROR_EOF =									MCP_HAL_STATUS_OPEN + 21,
	MCP_HAL_FS_STATUS_ERROR_FILE_NOT_CLOSE =						MCP_HAL_STATUS_OPEN + 22,
	MCP_HAL_FS_STATUS_ERROR_READING_FILE =							MCP_HAL_STATUS_OPEN + 23,
	MCP_HAL_FS_STATUS_ERROR_OPENING_FILE =							MCP_HAL_STATUS_OPEN + 24,
	MCP_HAL_FS_STATUS_ERROR_WRITING_TO_FILE =						MCP_HAL_STATUS_OPEN + 25,
	MCP_HAL_FS_STATUS_ERROR_MAX_FILE_HANDLE =						MCP_HAL_STATUS_OPEN + 26,
	MCP_HAL_FS_STATUS_ERROR_MAKE_DIR =								MCP_HAL_STATUS_OPEN + 27,
	MCP_HAL_FS_STATUS_ERROR_REMOVE_DIR =							MCP_HAL_STATUS_OPEN + 28,
	MCP_HAL_FS_STATUS_ERROR_MAX_DIRECTORY_HANDLE =				MCP_HAL_STATUS_OPEN + 29,
	MCP_HAL_FS_STATUS_ERROR_INVALID_DIRECTORY_HANDLE_VALUE =	MCP_HAL_STATUS_OPEN + 30,
	MCP_HAL_FS_STATUS_ERROR_DIRECTORY_NOT_EMPTY =					MCP_HAL_STATUS_OPEN + 31,
	MCP_HAL_FS_STATUS_ERROR_FILE_HANDLE =							MCP_HAL_STATUS_OPEN + 32,
	MCP_HAL_FS_STATUS_ERROR_FIND_NEXT_FILE =						MCP_HAL_STATUS_OPEN + 33,
	MCP_HAL_FS_STATUS_ERROR_OPEN_FLAG_NOT_SUPPORT=				MCP_HAL_STATUS_OPEN + 34,
	MCP_HAL_FS_STATUS_ERROR_DIRECTORY_HANDLE_OUT_OF_RANGE =		MCP_HAL_STATUS_OPEN + 35
} McpHalFsStatus;


/*-------------------------------------------------------------------------------
 *  McpHalFs
 *
 *  whether a file or a directory
 */
typedef enum tagMcpHalFsType
{
	MCP_HAL_FS_FILE,
	MCP_HAL_FS_DIR
} McpHalFsType;


/*-------------------------------------------------------------------------------
 *  McpHalFsPermission
 *
 *  bit masks for the access permissions on a file or directory.
 */
typedef McpU32 McpHalFsPermission;

#define MCP_HAL_FS_PERM_READ      		((McpHalFsPermission)0x00000001)      /* Read Permission  - applies to file or folder */   
#define MCP_HAL_FS_PERM_WRITE     		((McpHalFsPermission)0x00000002)      /* Write Permission - applies to file or folder */
#define MCP_HAL_FS_PERM_DELETE    		((McpHalFsPermission)0x00000004)      /* Delete Permission  - applies to file only */


/*-------------------------------------------------------------------------------
 *  McpHalFsStat structure
 *
 *  struct to hold data returned by stat functions.
 */
typedef struct tagMcpHalFsStat
{	
	McpHalFsType		        type;				/* file / dir */
	McpU32		        size;				/* in bytes */
	McpBool		        isReadOnly;			/* whether read-only file */
	McpHalDateAndTime	mTime;				/* modified time */
	McpHalDateAndTime	aTime;				/* accessed time */
	McpHalDateAndTime	cTime;				/* creation time */
	McpU16               userPerm; 	        /* applied user permission access */
	McpU16               groupPerm; 	        /* applied group permission access */
	McpU16               otherPerm; 	        /* applied other permission access */
} McpHalFsStat;


/*-------------------------------------------------------------------------------
 *  McpHalFsSeekOrigin 
 *
 *  origin of a seek operation.
 */
typedef enum tagMcpHalFsSeekOrigin
{	
	MCP_HAL_FS_CUR,	/* from current position */
	MCP_HAL_FS_END,	/* from end of file */
	MCP_HAL_FS_START	/* from start of file */
} McpHalFsSeekOrigin;


/*-------------------------------------------------------------------------------
 *  McpHalFsFileDesc
 * 
 *  file descriptor, instead of FILE 
 */
typedef McpS32 McpHalFsFileDesc;

#define	MCP_HAL_FS_INVALID_FILE_DESC				((McpHalFsFileDesc)-1)

/*-------------------------------------------------------------------------------
 *  McpHalFsFileDesc
 * 
 *  directory descriptor
 */
typedef McpU32 McpHalFsDirDesc;

#define	MCP_HAL_FS_INVALID_DIRECTORY_DESC		((McpHalFsDirDesc)-1)

/*-------------------------------------------------------------------------------
 *  McpHalFsStatFlags
 * 
 *   bit masks for MCP_HAL_FS_Stat flags
 */
typedef McpU32 McpHalFsStatFlags;

#define	MCP_HAL_FS_S_MTIME	    		((McpHalFsStatFlags)0x00000001)		/* Time of last modification of file */
#define	MCP_HAL_FS_S_ATIME	    		((McpHalFsStatFlags)0x00000002)		/* Time of last access of file */
#define	MCP_HAL_FS_S_CTIME	    		((McpHalFsStatFlags)0x00000004)		/* Time of creation of file */
#define	MCP_HAL_FS_S_SIZE		    		((McpHalFsStatFlags)0x00000008)		/* Size of the file in bytes */
#define	MCP_HAL_FS_S_USERPERM	   	((McpHalFsStatFlags)0x00000010)		/* convey the user access permission */
#define	MCP_HAL_FS_S_GROUPPERM		((McpHalFsStatFlags)0x00000020)		/* convey the group access permission */
#define	MCP_HAL_FS_S_OTHERPERM		((McpHalFsStatFlags)0x00000040)		/* convey the other access permission */

#define MCP_HAL_FS_S_ALL_FLAG	    		((McpHalFsStatFlags)0x000000FF)		/* enable all the flag */ 

/*-------------------------------------------------------------------------------
 *  McpHalFsOpenFlags
 * 
 *   bit masks for MCP_HAL_FS_Open flags
 */
typedef McpU32 McpHalFsOpenFlags;

#define	MCP_HAL_FS_O_WRONLY			((McpHalFsOpenFlags)0x00000001)		/* write only */
#define	MCP_HAL_FS_O_CREATE			((McpHalFsOpenFlags)0x00000002)		/* create only */

/* if file already exists and it is opened for writing, its length is truncated to zero. */
#define	MCP_HAL_FS_O_TRUNC			((McpHalFsOpenFlags)0x00000004)		
#define	MCP_HAL_FS_O_APPEND   		((McpHalFsOpenFlags)0x00000008)		/* append to EOF */
#define	MCP_HAL_FS_O_RDONLY			((McpHalFsOpenFlags)0x00000010)		/* read only */
#define	MCP_HAL_FS_O_RDWR     		((McpHalFsOpenFlags)0x00000020)		/* read & write */

/* generate error if MCP_HAL_FS_O_CREATE is also specified and the file already exists.*/
#define 	MCP_HAL_FS_O_EXCL			((McpHalFsOpenFlags)0x00000040)		
#define 	MCP_HAL_FS_O_BINARY			((McpHalFsOpenFlags)0x00000080)		/* binary file */
#define 	MCP_HAL_FS_O_TEXT			((McpHalFsOpenFlags)0x00000100)		/* text file */

/********************************************************************************
 *
 * Function declarations
 *
 *******************************************************************************/

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Init()
 *
 * Brief:  		Init FS module.
 *
 * Description:	Initiate the fs module and prepare it for fs operations.
 *
 * Type:
 *		blocking or non-blocking
 *
 * Parameters:
 *		callback [in] - callback function.
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 */
McpHalFsStatus MCP_HAL_FS_Init(void);

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_DeInit()
 *
 * Brief:  		Deinit FS module.
 *
 * Description:	Deinit FS module.
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 */
McpHalFsStatus MCP_HAL_FS_DeInit(void);

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Open()
 *
 * Brief:  		Open's a file.
 *
 * Description:	the file name should include all file-system and drive specification as
 *			   	used by convention in the target platform. from this name the type of file
 *			   	system should be derived (e.g. FFS, SD, hard disk, ...  )
 *			   	pFd is a pointer to the newly opened file 
 *
 * Type:
 *		blocking or non-blocking
 *
 * 	Parameters:
 *		fullpathfilename [in] - filename.
 *		flags [in] - file open flag e.g read, write ...
 *		fd [out] - file handle.
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Open(const McpUtf8* fullPathFileName, McpHalFsOpenFlags flags, McpHalFsFileDesc *fd);

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Close()
 *
 * Brief:  		Close file.
 *
 * Description: Close file.
 *
 * Type:
 *		blocking
 *
 * 	Parameters:
 *		fd [in] - file handle.
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Close( const McpHalFsFileDesc fd );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Read()
 *
 * Brief:  		Read from a file.
 *
 * Description: Read from a file.
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fd [in] - file handle.
 *		buf [in] - address of buffer to read data into.
 *		nSize [in] - maximum number of byte to be read.
 *		numRead [out] - points to actual number of bytes that have been read.
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Read ( const McpHalFsFileDesc fd, void* buf, McpU32 nSize, McpU32 *numRead );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Write()
 *
 * Brief:  		Write to a file.
 *
 * Description: Write to a file.
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fd [in] - file handle.
 *		buf [in] - address of buffer to write data from.
 *		nSize [in] - maximum number of byte to be read.
 *		numWritten [out] - points to actual number of bytes that have been written
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Write( const McpHalFsFileDesc fd, void* buf, McpU32 nSize, McpU32 *numWritten );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Flush()
 *
 * Brief:  		flush write buffers from memory to file.
 *
 * Description: flush write buffers from memory to file.
 *
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fd [in] - file handle.
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Flush( const McpHalFsFileDesc fd );

/*-------------------------------------------------------------------------------
 *  MCP_HAL_FS_Stat()
 *
 * Brief:  		get information from file / Directory.
 *
 * Description: get information from file / Directory.
 *
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fullPathName [in] - full path name directory / file.
 *
 *		McpHalFsStat [out] - points to McpHalFsStat structure that save the info: size, type ...  
 *							
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
 
McpHalFsStatus MCP_HAL_FS_Stat( const McpUtf8* fullPathName, McpHalFsStat* fileStat );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Tell()
 *
 * Brief:  		gets the current position of a file pointer.
 *
 * Description: gets the current position of a file pointer.
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fd [in] - file handle.
 *		curPosition [out] - points to current pointer locate in a file. 
 *							
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Tell( const McpHalFsFileDesc fd, McpU32 *curPosition );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Seek()
 *
 * Brief:  		moves the file pointer to a specified location.
 *
 * Description: moves the file pointer to a specified location.
 *
 * Type:
 *		blocking
 *
 * Parameters:
 *		fd [in] - file handle.
 *		offset [in] - number of bytes from origin
 *		from [in] - initial position origin.							
 *
 * Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Seek( const McpHalFsFileDesc fd, McpS32 offset, McpHalFsSeekOrigin from );

 /*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Mkdir()
 *
 * Brief:  		make directory.
 *
 * Description: make directory.
 *			   	both the backslash and the forward slash are valid path delimiters
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		dirFullPathName [in] - directory name.
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Mkdir( const McpUtf8 *dirFullPathName ); 

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Rmdir()
 *
 * Brief:  		remove directory.
 *
 * Description: remove directory.
 *			   	both the backslash and the forward slash are valid path delimiters
 *
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		dirFullPathName [in] - directory name.
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Rmdir( const McpUtf8 *dirFullPathName );
 
/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_OpenDir()
 *
 * Brief:  		Opens directory for reading.
 *
 * Description: Opens directory for reading.
 *
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		dirFullPathName [in] - pointer to directory name.
 *		dirDesc [out] - points to Directory handle  
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		MCP_HAL_FS_STATUS_ERROR_INVALID_DIRECTORY_HANDLE_VALUE -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_OpenDir( const McpUtf8 *dirFullPathName, McpHalFsDirDesc *dirDesc );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_ReadDir()
 *
 * Brief:  		get next file name in directory.
 *
 * Description: get first/next file name in a directory. 
 *				return the full path of the file
 *
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		dirDesc [in] - points to Directory handle .
 *		fileName [out] - return the full path name.  
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		MCP_HAL_FS_STATUS_ERROR_FIND_NEXT_FILE	 -  Operation failed.
 */

McpHalFsStatus MCP_HAL_FS_ReadDir( const McpHalFsDirDesc dirDesc, McpUtf8 **fileName ); 

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_CloseDir()
 *
 * Brief:  		close a directory for reading.
 *
 * Description: close a directory for reading. 
 *
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		dirDesc [in] - points to Directory handle .
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_CloseDir( const McpHalFsDirDesc dirDesc ); 

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Rename()
 *
 * Brief:  		renames a file or a directory.
 *
 * Description: renames a file or a directory. 
 *
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		fullPathOldName [in] - points to old name .
 *		fullPathNewName [in] - points to new name .
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Rename(const McpUtf8 *fullPathOldName, const McpUtf8 *fullPathNewName );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_Remove()
 *
 * Brief:  		removes a file.
 *
 * Description: removes a file. 
 *	
 *	Type:
 *		blocking
 *
 * 	Parameters:
 *		fullPathFileName [in] - points to file name .
 *
 * 	Returns:
 *		MCP_HAL_FS_STATUS_SUCCESS - Operation is successful.
 *
 *		other -  Operation failed.
 */
McpHalFsStatus MCP_HAL_FS_Remove( const McpUtf8 *fullPathFileName );

/*-------------------------------------------------------------------------------
 * MCP_HAL_FS_IsAbsoluteName()
 *
 * Brief:  		checks if the file name is full path or absolute path.
 *
 * Description: This function checks whether a given file name has an absolute path name.
 *
 * Type:
 *                Synchronous
 *
 * Parameters:
 *                fileName [in] - name of the file
 *
 *                isAbsolute [out] - result whether the name contains an absolute full path . 
 *                                        In case of failure to check it is set to FALSE
 *
 * Returns:
 *                MCP_HAL_FS_STATUS_SUCCESS - function succeeded to check if full path
 *
 *                MCP_HAL_FS_STATUS_ERROR_NOTAFILE -  failed to check. NULL name provided
 */
McpHalFsStatus MCP_HAL_FS_IsAbsoluteName( const McpUtf8 *fileName, McpBool *isAbsolute );

#endif /* __MCP_HAL_FS_H */



