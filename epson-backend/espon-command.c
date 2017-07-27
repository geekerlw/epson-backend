#include "epson-typdefs.h"
#include <stdio.h>
#include <time.h>

static const EPS_UINT8 ExitPacketMode[] = {
	0x00, 0x00, 0x00, 0x1B, 0x01, 0x40, 0x45, 0x4A, 0x4C, 0x20,
	0x31, 0x32, 0x38, 0x34, 0x2E, 0x34, 0x0A, 0x40, 0x45, 0x4A,
	0x4C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0A };

static const EPS_UINT8 InitPrinter[] = {
	0x1B, 0x40, };

static const EPS_UINT8 EnterRemoteMode[] = {
	0x1B, 0x28, 0x52, 0x08, 0x00, 0x00, 'R', 'E', 'M', 'O', 'T', 'E', '1', };

static const EPS_UINT8 RemoteJS[] = {
	'J', 'S', 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00 };

static const EPS_UINT8 RemoteCH[] = {
	'C', 'H', 0x02, 0x00, 0x00, 0x00 };
static const EPS_UINT8 RemoteNC[] = {
	'N', 'C', 0x02, 0x00, 0x00, 0x00 };
static const EPS_UINT8 RemoteVI[] = {
	'V', 'I', 0x02, 0x00, 0x00, 0x00 };

static const EPS_UINT8 ExitRemoteMode[] = {
	0x1B, 0x00, 0x00, 0x00, };

static const EPS_UINT8 DataCR[] = { 0x0D };
static const EPS_UINT8 DataLF[] = { 0x0A };
static const EPS_UINT8 DataFF[] = { 0x0C };

static const EPS_UINT8 RemoteLD[] = {
	'L', 'D', 0x00, 0x00, };

static const EPS_UINT8 RemoteJE[] = {
	'J', 'E', 0x01, 0x00, 0x00, };


static const EPS_UINT8 RemoteTI[] = {
	'T', 'I', 0x08, 0x00, 0x00,
	0x00, 0x00, /* YYYY */
	0x00,       /* MM */
	0x00,       /* DD */
	0x00,       /* hh */
	0x00,       /* mm */
	0x00,       /* ss */ };

#define EPS_MEM_GROW(t, p, pCurSize, nNewSize)						\
		/*EPS_DBGPRINT(("GROW %d->%d\n", *pCurSize, (nNewSize)))*/	\
		if(*pCurSize < (nNewSize)){									\
			p = (t)memRealloc(p, *pCurSize, (nNewSize));			\
			*pCurSize = (nNewSize);									\
		}

#define Min(a,b) ( ((a) < (b)) ? (a) : (b))
#define REMOTE_HEADER_LENGTH 5

typedef enum _EPS_ENDIAN {
	EPS_ENDIAN_NOT_TESTED = 1000,
	EPS_ENDIAN_BIG,
	EPS_ENDIAN_LITTLE
} EPS_ENDIAN;

typedef enum _EPS_BYTE_SIZE {
	EPS_2_BYTES = 2000,
	EPS_4_BYTES = 4000
} EPS_BYTE_SIZE;

/*******************************************|********************************************/
/*                                                                                      */
/* Function name:     memRealloc()   				                                    */
/*                                                                                      */
/* Arguments                                                                            */
/* ---------                                                                            */
/* Name:        Type:               Description:                                        */
/* buffer		void*				I/O: pointer of buffer								*/
/* oldSize		EPS_UINT32			I:   original buffer size							*/
/* newSize		EPS_UINT32			I:   new buffer size								*/
/*                                                                                      */
/* Return value:                                                                        */
/*				EPS_INT8*			pointer to finded string                            */
/*                                                                                      */
/* Description:                                                                         */
/*      Realocate buffer.																*/
/*                                                                                      */
/*******************************************|********************************************/
void* memRealloc(

	void*       buffer,
	EPS_UINT32  oldSize,
	EPS_UINT32  newSize

) {
	/* Create a temporary pointer to a new buffer of the desired size */
	void* newBuffer = malloc(newSize);
	if (NULL == newBuffer) {
		free(buffer);
		return NULL;
	}

	memset(newBuffer, 0, newSize);

	/* Copy the data from the old buffer to the new one */
	if (oldSize < newSize)
	{
		memcpy(newBuffer, buffer, oldSize);
	}
	else
	{
		memcpy(newBuffer, buffer, newSize);
	}
	/* Free the original buffer */
	free(buffer);

	/* Return a pointer to the new block of memory */
	return newBuffer;
}



/*******************************************|********************************************/
/*                                                                                      */
/* Function name:   AddCmdBuff()                                                        */
/*                                                                                      */
/* Arguments                                                                            */
/* ---------                                                                            */
/* Name:			Type:               Description:                                    */
/* pBuff            EPS_UINT8**         IO: pointer to command buffer                   */
/* pPos             EPS_UINT8**         IO: pointer to append position                  */
/* bufSize          EPS_UINT32*         IO: pointer to size of command buffer(pBuff)    */
/* cmd              EPS_UINT8*          I : pointer to command                          */
/* cmdSize          EPS_UINT32          I : size of command(cmd)                        */
/*                                                                                      */
/* Return value:                                                                        */
/*      EPS_ERR_NONE                    - Success                                       */
/*      EPS_ERR_MEMORY_ALLOCATION       - Failed to allocate memory                     */
/*                                                                                      */
/* Description:                                                                         */
/*      Append command to buffer. If the buffer is short, expand it.                    */
/*                                                                                      */
/*******************************************|********************************************/
EPS_ERR_CODE AddCmdBuff(

	EPS_UINT8 **pBuff,
	EPS_UINT8 **pPos,
	EPS_UINT32 *bufSize,
	const EPS_UINT8 *cmd,
	EPS_UINT32 cmdSize

) {
	EPS_UINT32	cmdPosDist = (EPS_UINT32)(*pPos - *pBuff);	/* command offset distance  */

	EPS_MEM_GROW(EPS_UINT8*, *pBuff, bufSize, cmdPosDist + cmdSize)

		if (*pBuff != NULL) {
			*pPos = *pBuff + cmdPosDist;
			memcpy(*pPos, cmd, cmdSize);
			*pPos += cmdSize;
			return(STS_NO_ERROR);
		}
		else {
			return(-1);
		}
}



/*******************************************|********************************************/
/* Function name:   memSetEndian()                                                      */
/*                                                                                      */
/* Arguments                                                                            */
/* ---------                                                                            */
/* Name:        Type:               Description:                                        */
/* Endianess    EPS_ENDIAN          I: Desired Endianess (Big/Little)                   */
/* byteSize     EPS_BYTE_SIZE       I: 2-byte or 4 bytes to convert                     */
/* value        EPS_UINT32          I: 4 Bytes to be swapped if necesaray               */
/* array        EPS_UINT8*          O: Correct endian-ness bytes                        */
/*                                                                                      */
/* Return value:                                                                        */
/*      None                                                                            */
/*                                                                                      */
/* Description:                                                                         */
/*      Swap data depending on endian-ness.                                             */
/*                                                                                      */
/*******************************************|********************************************/
void     memSetEndian(

	EPS_ENDIAN      Endianess,          /* Desired Endianess (Big/Little)           */
	EPS_BYTE_SIZE   byteSize,           /* 2-byte or 4 bytes to convert             */
	EPS_UINT32      value,              /* 4 Bytes to be swapped if necesaray       */
	EPS_UINT8*      array               /* Correct endian-ness bytes                */

) {

	/*** Declare Variable Local to Routine                                                  */
	EPS_UINT16  value2byte;
	EPS_UINT32  value4byte;

	/*** Initialize Local Variables                                                         */

	/*** Based on desired Eniandess - Perform test and swap, if necessary                   */
	switch (byteSize + Endianess) {
		/*** Change 2 bytes value to the little endianness                              */
	case (EPS_2_BYTES + EPS_ENDIAN_LITTLE):
#if 0  /* Not Used */
		value2byte = (EPS_UINT16)value;
		array[0] = (EPS_UINT8)((value2byte) & 0x00ff);
		array[1] = (EPS_UINT8)((value2byte >> 8) & 0x00ff);
#endif
		break;
		/*** Change 2 bytes value to the big endianness                                 */
	case (EPS_2_BYTES + EPS_ENDIAN_BIG):
		value2byte = (EPS_UINT16)value;
		array[0] = (EPS_UINT8)((value2byte >> 8) & 0x00ff);
		array[1] = (EPS_UINT8)((value2byte) & 0x00ff);
		break;
		/*** Change 4 bytes value to the little endianness                              */
	case (EPS_4_BYTES + EPS_ENDIAN_LITTLE):
		value4byte = (EPS_UINT32)value;
		array[0] = (EPS_UINT8)((value4byte) & 0x000000ff);
		array[1] = (EPS_UINT8)((value4byte >> 8) & 0x000000ff);
		array[2] = (EPS_UINT8)((value4byte >> 16) & 0x000000ff);
		array[3] = (EPS_UINT8)((value4byte >> 24) & 0x000000ff);
		break;
		/*** Change 4 bytes value to the big endianness                                 */
	case (EPS_4_BYTES + EPS_ENDIAN_BIG):
		value4byte = (EPS_UINT32)value;
		array[0] = (EPS_UINT8)((value4byte >> 24) & 0x000000ff);
		array[1] = (EPS_UINT8)((value4byte >> 16) & 0x000000ff);
		array[2] = (EPS_UINT8)((value4byte >> 8) & 0x000000ff);
		array[3] = (EPS_UINT8)((value4byte) & 0x000000ff);
		break;
	default:
		break;
	}
}



/* Local time                                                                       */
/*** -------------------------------------------------------------------------------*/
typedef struct _tagEPS_LOCAL_TIME_ {
	EPS_UINT16          year;
	EPS_UINT8           mon;
	EPS_UINT8           day;
	EPS_UINT8           hour;
	EPS_UINT8           min;
	EPS_UINT8           sec;
}EPS_LOCAL_TIME;

/*******************************************|********************************************/
/* Function name:   epsmpGetLocalTime()                                                 */
/*                                                                                      */
/* Arguments                                                                            */
/* ---------                                                                            */
/* Name:        Type:               Description:                                        */
/* epsTime      EPS_LOCAL_TIME*     O: time structure.                                  */
/*                                                                                      */
/* Return value:                                                                        */
/*      0                                                                               */
/*                                                                                      */
/* Description:                                                                         */
/*      get the system localtime.                                                       */
/*                                                                                      */
/*******************************************|********************************************/
EPS_UINT32 getLocalTime(EPS_LOCAL_TIME *epsTime)
{
	time_t now;
	struct tm *t;

	now = time(NULL);
	t = localtime(&now);

	epsTime->year = (EPS_UINT16)t->tm_year + 1900;
	epsTime->mon = (EPS_UINT8)t->tm_mon + 1;
	epsTime->day = (EPS_UINT8)t->tm_mday;
	epsTime->hour = (EPS_UINT8)t->tm_hour;
	epsTime->min = (EPS_UINT8)t->tm_min;
	epsTime->sec = (EPS_UINT8)t->tm_sec;

	return 0;
}

/*======================================================================================*/
/*** Set up Remote "TI" Command                                                         */
/*======================================================================================*/
static void    MakeRemoteTICmd(

	EPS_UINT8*		pBuf

) {
	/*** Declare Variable Local to Routine                                                  */
	EPS_LOCAL_TIME  locTime;
	EPS_UINT8       array2[2] = { 0, 0 };         /* Temporary Buffer for 2 byte Big Endian   */



												  /* Get platform local time */
	getLocalTime(&locTime);

	/*** Skip Header                                                                    */
	pBuf += REMOTE_HEADER_LENGTH;

	/*** Set Attributes/Values                                                          */
	memSetEndian(EPS_ENDIAN_BIG, EPS_2_BYTES, locTime.year, array2);
	memcpy(pBuf, array2, sizeof(array2));
	pBuf += sizeof(array2);
	*pBuf++ = locTime.mon;
	*pBuf++ = locTime.day;
	*pBuf++ = locTime.hour;
	*pBuf++ = locTime.min;
	*pBuf = locTime.sec;



	return;

}


/*******************************************|********************************************/
/*                                                                                      */
/* Function name:   epsMakeMainteCmd()                                                  */
/*                                                                                      */
/* Arguments                                                                            */
/* ---------                                                                            */
/* Name:        Type:               Description:                                        */
/* cmd          EPS_INT32           I:maintenance command type.                         */
/* buffer       EPS_UINT8*          I:pointer to command buffer.                        */
/* buffersize   EPS_UINT32*         I: buffer size.                                     */
/*                                  O: need size.                                       */
/*                                                                                      */
/* Return value:                                                                        */
/*      << Normal >>                                                                    */
/*      EPS_ERR_NONE                    - Success                                       */
/*      << Error >>                                                                     */
/*      EPS_ERR_LIB_NOT_INITIALIZED		- ESC/P-R Lib is NOT initialized				*/
/*      EPS_ERR_INV_ARG_CMDTYPE			- Invalid argument "cmd"						*/
/*      EPS_ERR_OPR_FAIL				- Internal Error								*/
/*      EPS_ERR_MEMORY_ALLOCATION		- Failed to allocate memory						*/
/*      EPS_ERR_PRINTER_NOT_SET         - Target printer is not specified               */
/*      EPS_ERR_LANGUAGE_NOT_SUPPORTED  - Unsupported function Error (language)         */
/*                                                                                      */
/* Description:                                                                         */
/*      Make maintenance command.                                                       */
/*      Call this function as specifying NULL to buffer, so that the memory size        */
/*      necessary for executing the maintenance command is returned to buffersize.      */
/*                                                                                      */
/*******************************************|********************************************/
EPS_ERR_CODE epsMakeMainteCmd(

	EPS_INT32		cmd,
	EPS_UINT8*		buffer,
	EPS_UINT32*		buffersize

) {

	/*** Declare Variable Local to Routine                                                  */
	EPS_ERR_CODE retStatus = STS_NO_ERROR;      /* Return status of internal calls      */
	EPS_UINT8 *pCmdBuf = NULL;
	EPS_UINT8 *pCmdPos = NULL;
	EPS_UINT32 cmdSize = 0;
	EPS_UINT32 cmdBufSize = 0;

	/*** Has a target printer specified                                                     */
	//	if(NULL == printJob.printer){
	//		EPS_RETURN( EPS_ERR_PRINTER_NOT_SET );
	//	}
	//
	//	if(EPS_LANG_ESCPR != printJob.printer->language ){
	//		EPS_RETURN( EPS_ERR_LANGUAGE_NOT_SUPPORTED );
	//	}


	if (buffer) {
		cmdBufSize = 256;
		pCmdBuf = (EPS_UINT8*)malloc(cmdBufSize);
		pCmdPos = pCmdBuf;
	}

#define MakeMainteCmd_ADDCMD(CMD) {														\
		if(buffer){																		\
			retStatus = AddCmdBuff(&pCmdBuf, &pCmdPos, &cmdBufSize, CMD, sizeof(CMD));	\
			if(STS_NO_ERROR != retStatus){												\
				goto epsMakeMainteCmd_END;												\
			}																			\
		}																				\
		cmdSize += sizeof(CMD);															\
	}

	switch (cmd) {
	case EPS_MNT_NOZZLE:				/* nozzle check */
		MakeMainteCmd_ADDCMD(ExitPacketMode)

			MakeMainteCmd_ADDCMD(InitPrinter)
			MakeMainteCmd_ADDCMD(InitPrinter)

			MakeMainteCmd_ADDCMD(EnterRemoteMode)
			//		if(getLocalTime){
			MakeMainteCmd_ADDCMD(RemoteTI)
			if (buffer) {
				MakeRemoteTICmd(pCmdPos - sizeof(RemoteTI));
			}
		//		}
		MakeMainteCmd_ADDCMD(RemoteJS)
			MakeMainteCmd_ADDCMD(RemoteNC)
			//		if(pCmdPos && obsIsA3Model(EPS_MDC_NOZZLE)){
			//			*(pCmdPos - sizeof(RemoteNC) + 5) = 0x10;
			//		}

			MakeMainteCmd_ADDCMD(ExitRemoteMode)

			MakeMainteCmd_ADDCMD(DataCR)
			MakeMainteCmd_ADDCMD(DataLF)
			MakeMainteCmd_ADDCMD(DataCR)
			MakeMainteCmd_ADDCMD(DataLF)

			MakeMainteCmd_ADDCMD(EnterRemoteMode)
			MakeMainteCmd_ADDCMD(RemoteVI)
			MakeMainteCmd_ADDCMD(RemoteLD)
			MakeMainteCmd_ADDCMD(ExitRemoteMode)

			MakeMainteCmd_ADDCMD(DataFF)
			MakeMainteCmd_ADDCMD(InitPrinter)
			MakeMainteCmd_ADDCMD(InitPrinter)

			MakeMainteCmd_ADDCMD(EnterRemoteMode)
			MakeMainteCmd_ADDCMD(RemoteJE)
			MakeMainteCmd_ADDCMD(ExitRemoteMode)

			break;

	case EPS_MNT_CLEANING:					/* head cleaning */
		MakeMainteCmd_ADDCMD(ExitPacketMode)
			MakeMainteCmd_ADDCMD(InitPrinter)
			MakeMainteCmd_ADDCMD(InitPrinter)

			MakeMainteCmd_ADDCMD(EnterRemoteMode)
			//		if(epsCmnFnc.getLocalTime){
			MakeMainteCmd_ADDCMD(RemoteTI)
			if (buffer) {
				MakeRemoteTICmd(pCmdPos - sizeof(RemoteTI));
			}
		//		}
		MakeMainteCmd_ADDCMD(RemoteCH)
			MakeMainteCmd_ADDCMD(ExitRemoteMode)

			MakeMainteCmd_ADDCMD(InitPrinter)
			MakeMainteCmd_ADDCMD(InitPrinter)

			MakeMainteCmd_ADDCMD(EnterRemoteMode)
			MakeMainteCmd_ADDCMD(RemoteJE)
			MakeMainteCmd_ADDCMD(ExitRemoteMode)
			break;

	default:
		break;
	}

epsMakeMainteCmd_END:

	if (STS_NO_ERROR == retStatus) {



		if (buffer) {
			if (cmdSize > 0) {
				memcpy(buffer, pCmdBuf, cmdSize);
			}
		}
		*buffersize = cmdSize;
	}
	free(pCmdBuf);

	return(retStatus);

}
