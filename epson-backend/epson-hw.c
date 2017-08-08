/*
* Copyright (C) Seiko Epson Corporation 2014.
*
* This file is part of the `ecbd' program.
*
*  This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
* Epson CBT engine
*
* Offering of service with the host side requiring by CBT packet
* communication.  Do packet switching with client-side here.
*
*  FUNCTIONS:
*       DllMain()
*       ECBT_Open()
*       ECBT_Close()
*       ECBT_OpenChannel()
*       ECBT_CloseChannel()
*       ECBT_Write()
*       ECBT_Read()
*
*/
#include <stdio.h>
#include <string.h>

#include "epson-hw.h"
#include "epson-local.h"
//#include "daemon_osdef.h"
//#include "daemon_osfunc.h"

void ReadThread(LPVOID param);


/******************************************************************************\
*                               GLOBAL VARIABLES
\******************************************************************************/

struct  tag_PORT    Port[PortNumber]; /* LPT0 - LPT15 */
int iInitFlag = 0;            /* The first time is 0 */

BYTE    DATA_SERVICE[] = "EPSON-DATA"; /* Service name of JOB data channel */
BYTE    SCAN_SERVICE[] = "EPSON-SCAN"; /* Service name of scanner data channel */

int iLastPriority = THREAD_PRIORITY_NORMAL;
int iCurrentPriority = THREAD_PRIORITY_NORMAL;

/******************************************************************************\
*
*  FUNCTION:    DllMain
*
*  INPUTS:      hDLL       - handle of DLL
*               dwReason   - indicates why DLL called
*               lpReserved - reserved
*
*  RETURNS:     TRUE (always, in this example.)
*
*               Note that the retuRn value is used only when
*               dwReason = DLL_PROCESS_ATTACH.
*
*               Normally the function would return TRUE if DLL initial-
*               ization succeeded, or FALSE it it failed.
*
*  GLOBAL VARS: ghMod - handle of DLL (initialized when PROCESS_ATTACHes)
*
*  COMMENTS:    The function will display a dialog box informing user of
*               each notification message & the name of the attaching/
*               detaching process/thread. For more information see
*               "DllMain" in the Win32 API reference.
*
\******************************************************************************/

BOOL APIENTRY DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
	int     i;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		for (i = 0; i<PortNumber; i++) {
			Port[i].hOpen = NULL;

			if (iInitFlag == 0) {
				/* Create event object to synchronize every functions. */
				Port[i].hFunc_Obj = CreateEvent(NULL, FALSE, TRUE, NULL);


				/* CriticalSection Object generation for Read/Write synchronization */
				InitializeCriticalSection(&Port[i].CsPort);
				Port[i].lpCsPort = &Port[i].CsPort;
			}
		}
		iInitFlag = 1;
		break;

	case DLL_PROCESS_DETACH:
		for (i = 0; i<PortNumber; i++) {
			if (Port[i].hOpen != NULL) {
				Port[i].Status = CBT_STS_MUSTCLOSE;
				Terminate_Fnc((LPPORT)&(Port[i]), (int)CBT_ERR_NORMAL);
			}

			if (Port[i].lpCsPort != NULL) {
				DeleteCriticalSection(&Port[i].CsPort);
				Port[i].lpCsPort = NULL;
			}

			if (Port[i].hFunc_Obj != NULL) {
				CloseHandle(Port[i].hFunc_Obj);
				Port[i].hFunc_Obj = NULL;
			}
		}
		iInitFlag = 0;
		break;
	}
	return TRUE;
}


/******************************************************************************\
*
*  FUNCTION: fCODE ECBT_Open (HANDLE hOpen, LPHANDLE lpHECBT)
*
*  INPUTS:      hOpen       - file handle
*               lpHECBT     - pointer to area returning CBT handle
*
*  RETURNS:     end code (negative number is error)
*
*  GLOBAL VARS: Port[]      - array of port structure (0-15)
*
*  COMMENTS:    Shift to packet mode (CBT) with a printer.
*               And secure communication path with control channel.
*
\******************************************************************************/

int ECBT_Open(HANDLE hOpen, LPHANDLE lpHECBT)
{
	int     fRet;
	int     iDevice;
	LPPORT      pPort;	/* ECBT handle */
	DWORD       RT_ID;
	HANDLE      hEvent;


	if (hOpen == NULL)
		return CBT_ERR_PARAM;

	fRet = PortCheck(hOpen);

	if (fRet < 0)
		return  fRet;
	else
		iDevice = fRet;

	pPort = &(Port[iDevice]); /* Pointer setting of port management data */

	if (pPort->hFunc_Obj == NULL) {
		fRet = CBT_ERR_MEMORY;
		return fRet;
	}

	// Synchronization of function - start

	pPort->Counter += 1;	/* add process function counter */

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
		pPort->Counter -= 1;
		return CBT_ERR_EVTIMEOUT;
	}

	if (pPort->hOpen != NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBT2NDOPEN;
	}

	pPort->Counter = 1; /* initialization */

						/* Create event object to examine end of ReadThread */
	if (NULL == (hEvent = CreateEvent(NULL, TRUE, TRUE, NULL))) {
		fRet = CBT_ERR_MEMORY;
		goto    Open_Done;
	}
	pPort->hRThread_Obj = hEvent;

	/* Read thread execution event object */
	if (NULL == (hEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		fRet = CBT_ERR_MEMORY;
		goto    Open_Done;
	}
	pPort->hRX_Obj = hEvent;

	/* CH0 command synchronization event object */
	if (NULL == (hEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		fRet = CBT_ERR_MEMORY;
		goto    Open_Done;
	}
	pPort->hCMD_Obj = hEvent;

	/* CH0 reply reception completion event object */
	if (NULL == (hEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		fRet = CBT_ERR_MEMORY;
		goto    Open_Done;
	}
	pPort->hRPL_Obj = hEvent;

	/* Data reception completion event object */
	if (NULL == (hEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
		fRet = CBT_ERR_MEMORY;
		goto    Open_Done;
	}
	pPort->hDAT_Obj = hEvent;


	pPort->hOpen = hOpen; /* R/W handle */
	pPort->fRTstat = RT_NotAct; /* 0:Not Active 1:Active 2:Exit */
	pPort->lpSocketCtrl = NULL;
	pPort->Status = CBT_STS_NONPACKET;
	pPort->Pre_Reply = CBT_RPY_NONE; /* The status that doesnot process a command */
	pPort->Wait_Reply = CBT_RPY_NONE;
	pPort->Wait_Channel = 0;
	pPort->DATA_Channel = Def_DataCH;   /* 0x40 */
	pPort->SCAN_Channel = Def_ScanCH;   /* 0x50 */


	DummyRead_Fnc(pPort);

	/* --------------------------------------------------------- */
	/* EpsonPackingCommand */

	pPort->Status = CBT_STS_PACKETMODE;       /* Compulsion */

	fRet = Tx_EpsonPacking(pPort);

	if (fRet != CBT_ERR_NORMAL) {
		fRet = CBT_ERR_FATAL;
		pPort->Status = CBT_STS_MUSTCLOSE;
		goto  Open_Done;
	}

	/* Create ReadThread */

	pPort->Pre_Reply = CBT_RPY_EPSONPACKING; /* Reply to expect */
	pPort->fRTstat = RT_Active; /* = 1 */

	ResetEvent(pPort->hRThread_Obj);
	SetEvent(pPort->hRX_Obj);
	pPort->hRThread = CreateThread(NULL, 0x4000, (LPTHREAD_START_ROUTINE)ReadThread,
		(LPVOID)pPort, 0, &RT_ID);

	/* confirm reception of Reply */
	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {
		if (pPort->CH0BUFFER[07] == 0x00)        // Result
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_CBTDISABLE;      // retry
	}

	if (pPort->Wait_Reply == CBT_RPY_EPSONPACKING)
		pPort->Wait_Reply = CBT_RPY_NONE;       // command completion

	SetEvent(pPort->hCMD_Obj);

	// Init transaction
	if (fRet == CBT_ERR_NORMAL)
		// Init Command  Write & Reply check
		fRet = Init_Command(pPort);

	if (fRet == CBT_ERR_NORMAL) {
		pPort->Status = CBT_STS_PACKETMODE;
		*lpHECBT = pPort;                       // return it as a handle of ECBT
	}
	else
		pPort->Status = CBT_STS_MUSTCLOSE;

Open_Done:

	pPort->Counter -= 1;

	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - end

	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);


	return fRet;
}




/******************************************************************************\
*
*  FUNCTION: int ECBT_Close (HANDLE hECBT)
*
*  INPUTS:      hECBT       - ECBT handle
*
*  RETURNS:     end code (negative number is error)
*
*  GLOBAL VARS: Port[]      - array of port structure (0-15)
*
*  COMMENTS:    Let a printer shift to non-packet mode.
*               ontrol channel is closed.
*
\******************************************************************************/

int ECBT_Close(HANDLE hECBT)
{
	int         fRet;
	int         iPort;
	LPPORT      pPort = (LPPORT)hECBT;


	iPort = HandleCheck(hECBT);

	if (iPort < 0) {
		return  iPort;
	}

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL) {
		pPort->Counter += 1;            // add process function counter
		if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
			pPort->Counter -= 1;
			return CBT_ERR_EVTIMEOUT;
		}
	}
	else
		return CBT_ERR_CBTNOTOPEN;

	if (pPort->hOpen == NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBTNOTOPEN;
	}

	fRet = Close_AllSocket(pPort);

	// Exit Command  Write & Reply check
	if (pPort->Status != CBT_STS_AFTERERROR) {
		if (fRet > CBT_ERR_FATAL)
			fRet = Exit_Command(pPort);
	}

	// wait for other function being finished
	while (1) {
		SetEvent(pPort->hFunc_Obj);
		if (pPort->Counter <= 1) {
			break;
		}
		Sleep(20);
	}

	// A printer shifts to non-packet mode
	pPort->Status = CBT_STS_MUSTCLOSE;
	fRet = CBT_ERR_NORMAL;  // Error packet cannot send a message

	pPort->Counter -= 1;
	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - end
	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);

	return CBT_ERR_NORMAL;	// 0x00
}


/******************************************************************************\
*
*  FUNCTION: int ECBT_OpenChannel (HANDLE hECBT, BYTE SID)
*
*  INPUTS:      hECBT       - ECBT handle
*               SID         - 0x02 Control Data channel
*                             0x40 JOB Data channel
*                             0x50 Scanner Data channel
*
*  RETURNS:     end code (negative number is error)
*
*  GLOBAL VARS: none
*
*  COMMENTS:    Open logical channel.
*               A socket sets it in No Credit Mode.
*
\******************************************************************************/

int ECBT_OpenChannel(HANDLE hECBT, BYTE SID)
{
	int         fRet;
	int         iCredit;
	LPPORT      pPort = (LPPORT)hECBT;
	LPSOCKET    pSocket = NULL;
	WORD        PtSsize;
	WORD        StPsize;
	DWORD       BUFsize = 0;
	LPBYTE      pTxBuf = NULL;
	LPBYTE      pRxBuf = NULL;
	HANDLE      hEvent = NULL;


	fRet = HandleCheck(hECBT);
	if (fRet < 0)
		return  fRet;

	if (pPort->Status != CBT_STS_PACKETMODE)
		return CBT_ERR_FNCDISABLE;

	if ((SID == 0x00) || (SID == 0xff))
		return  CBT_ERR_PARAM;

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL) {
		pPort->Counter += 1;            // add process function counter
		if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
			pPort->Counter -= 1;
			return CBT_ERR_EVTIMEOUT;
		}
	}
	else
		return CBT_ERR_CBTNOTOPEN;

	if (pPort->hOpen == NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBTNOTOPEN;
	}


	pSocket = Get_SocketCtrl(pPort, SID);
	if (pSocket != NULL) {
		fRet = CBT_ERR_CH2NDOPEN;
		goto    OC_Done;
	}

	// Packet size to require
	if (SID == pPort->DATA_Channel) {   // JOB Data Channel
		PtSsize = TxPSIZE;      // 4102(4096+6)
		StPsize = RxPSIZE;      // 512  Reverse
	}
	else {
		if (SID == pPort->SCAN_Channel) {   // Scanner Data Channel
			PtSsize = RxPSIZE;              // 512 Forward
			StPsize = ScanSIZE;             // 32774(32768+6) Reverse
		}
		else {                          // Control Data Channel
			PtSsize = RxPSIZE;              // 512 (506+6)
			StPsize = RxPSIZE;              // 512 (506+6)
		}
	}

	// Prepare area of receive data buffer (8 packet)
	if (SID != pPort->SCAN_Channel) {
		BUFsize = (StPsize - 6) * Def_Credit;
		if (BUFsize > 0xffff)
			BUFsize = 0xffff;
		if (NULL == (pRxBuf = (LPBYTE)VirtualAlloc(NULL, BUFsize,
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)))
			fRet = CBT_ERR_MEMORY;
	}
	else {          // Scanner Data Channel
		if (NULL != (pRxBuf = (LPBYTE)VirtualAlloc(NULL, Size128K,
			MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)))
			BUFsize = Size128K;
		else
			if (NULL != (pRxBuf = (LPBYTE)VirtualAlloc(NULL, Size64K,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)))
				BUFsize = Size64K;
			else
				if (NULL != (pRxBuf = (LPBYTE)VirtualAlloc(NULL, Size32K,
					MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)))
					BUFsize = Size32K;
				else
					fRet = CBT_ERR_MEMORY;
	}
	if (fRet < 0)
		goto    OC_Done;


	// OpenChannel Command  Write & Reply check
	fRet = OpenChannel_Command(pPort, SID, &PtSsize, &StPsize,
		0, 0);    // NoCredit Mode = 0

	if (fRet >= 0)
		iCredit = fRet;

	// check that the packet size that I decided is OK.
	if (fRet >= 0) {
		if (0x00 == PtSsize)
			if (0x00 == StPsize)
				fRet = CBT_ERR_RPLYPSIZE;
	}

	if (fRet >= 0) {
		if ((0 < PtSsize) && (PtSsize < 6))    // 1-5 is error
			fRet = CBT_ERR_RPLYPSIZE;
	}

	if (fRet >= 0) {
		if ((0 < StPsize) && (StPsize < 6))    // 1-5 is error
			fRet = CBT_ERR_RPLYPSIZE;
	}


	// send data buffer
	if (fRet >= 0)
		if (PtSsize > 0)
			if (NULL == (pTxBuf = (LPBYTE)VirtualAlloc(NULL, (DWORD)PtSsize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)))
				fRet = CBT_ERR_MEMORY;

	// socket management data
	if (fRet >= 0)
		if (NULL == (pSocket = (LPSOCKET)LocalAlloc(LMEM_FIXED, sizeof(CBTSOCKET))))
			fRet = CBT_ERR_MEMORY;

	// Synchronization of buffer post of a socket
	if (fRet >= 0)
		if (NULL == (hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
			fRet = CBT_ERR_MEMORY;

	if (fRet >= 0) {
		pSocket->PreSocket = NULL;         // prev
		pSocket->PostSocket = NULL;         // next
		pSocket->fOpen = 0x01;         // finish open
		pSocket->SID = SID;          // socket ID
		pSocket->fMODE = 0;            // active
		pSocket->Counter = 0;            // function execution counter

		pSocket->PtSsize = PtSsize;      // PC->printer
		pSocket->StPsize = StPsize;      // printer->PC
		pSocket->Credit_P = iCredit;      // credit from a printer
		pSocket->Credit_H = 0;            // credit from a host

		pSocket->lpTxBuf = pTxBuf;          // sending buffer
		pSocket->szTxBuf = (DWORD)PtSsize;  // buffer size

		pSocket->hBUF_Obj = hEvent;          // buffer update synchronization event object
		pSocket->lpRxBuf = pRxBuf;          // Receiving buffer
		pSocket->szRxBuf = BUFsize;         // buffer size
		pSocket->offsetRxBDTop = 0;         // top offset
		pSocket->offsetRxBDEnd = 0;         // end offset

		Link_SocketCtrl(pPort, pSocket);

		SetEvent(pSocket->hBUF_Obj);
		fRet = CBT_ERR_NORMAL;
	}
	else {
		if (hEvent != NULL)
			CloseHandle(hEvent);
		if (pRxBuf != NULL)
			VirtualFree((LPVOID)pRxBuf, 0, MEM_RELEASE);
		if (pTxBuf != NULL)
			VirtualFree((LPVOID)pTxBuf, 0, MEM_RELEASE);
		if (pSocket != NULL)
			LocalFree((HANDLE)pSocket);
	}

OC_Done:
	pPort->Counter -= 1;

	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);

	return fRet;
}


/******************************************************************************\
*
*  FUNCTION: int ECBT_CloseChannel (HANDLE hECBT, BYTE SID)
*
*  INPUTS:      hECBT       - ECBT handle
*               SID         - 0x02 Control Data channel
*                             0x40 JOB Data channel
*
*  RETURNS:     end code (negative number is error)
*
*  GLOBAL VARS: none
*
*  COMMENTS:    Close logical channel.
*
\******************************************************************************/

int ECBT_CloseChannel(HANDLE hECBT, BYTE SID)
{
	int         fRet;
	LPPORT      pPort = (LPPORT)hECBT;
	LPSOCKET    pSocket;
	HANDLE      hEvent;


	fRet = HandleCheck(hECBT);
	if (fRet < 0)
		return  fRet;

	if (pPort->Status == CBT_STS_AFTERERROR)
		return CBT_ERR_NORMAL;

	if ((SID == 0x00) || (SID == 0xff))
		return  CBT_ERR_PARAM;

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL) {
		pPort->Counter += 1;            // add process function counter
		if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
			pPort->Counter -= 1;
			return CBT_ERR_EVTIMEOUT;
		}
	}
	else
		return CBT_ERR_CBTNOTOPEN;

	if (pPort->hOpen == NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBTNOTOPEN;
	}

	// check of a socket
	pSocket = Get_SocketCtrl(pPort, SID);

	if (pSocket == NULL) {
		fRet = CBT_ERR_CHNOTOPEN;
		goto    CC_Done;
	}

	if (pSocket->fMODE == 1) {             // received CloseChannel?
		fRet = CBT_ERR_CHNOTOPEN;
		goto    CC_Done;
	}

	pSocket->fMODE = 2;                 // mark socket Close

										// CloseChannel Command  Write & Reply check
	fRet = CloseChannel_Command(pPort, SID);

	if (WAIT_TIMEOUT == WaitForSingleObject(pSocket->hBUF_Obj, Timeout_Time))
		fRet = CBT_ERR_EVTIMEOUT;


	hEvent = pSocket->hBUF_Obj;

	// send data buffer
	if (pSocket->lpTxBuf != NULL)
		VirtualFree((LPVOID)(pSocket->lpTxBuf), 0, MEM_RELEASE);

	// receive data buffer
	if (pSocket->lpRxBuf != NULL)
		VirtualFree((LPVOID)(pSocket->lpRxBuf), 0, MEM_RELEASE);

	// socket management data
	Eject_SocketCtrl(pPort, pSocket);
	LocalFree((HANDLE)pSocket);

	if (hEvent != NULL)
		CloseHandle(hEvent);

CC_Done:
	pPort->Counter -= 1;

	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);

	return fRet;
}


/******************************************************************************\
*
*  FUNCTION: int ECBT_Write (HANDLE hECBT,BYTE SID,LPBYTE lpBuffer,LPINT lpSize)
*
*  INPUTS:      hECBT       - ECBT handle
*               SID         - 0x02 Control Data channel
*                             0x40 JOB Data channel
*               lpBuffer    - send data buffer
*               lpSize      - send data size
*
*  RETURNS:     Normal      - The data size that transmitted a message
*                             (= lpSize)
*               Error       - end code (negative number is error)
*
*  GLOBAL VARS: none
*
*  COMMENTS:    Send data from a host to a printer.
*
\******************************************************************************/

int ECBT_Write(HANDLE hECBT, BYTE SID, LPBYTE lpBuffer, LPINT lpSize)
{
	int         fRet;
	LPPORT      pPort = (LPPORT)hECBT;
	LPSOCKET    pSocket;
	LPBYTE      lpSource;
	int         TxCount;        // finish sending
	int         RestSize;       // not sending
	HANDLE      hEvent;


	TxCount = *lpSize;
	*lpSize = 0;

	fRet = HandleCheck(hECBT);
	if (fRet < 0)
		return  fRet;

	if (pPort->Status != CBT_STS_PACKETMODE)
		return CBT_ERR_FNCDISABLE;

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL) {
		pPort->Counter += 1;            // add process function counter
		if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
			pPort->Counter -= 1;
			return CBT_ERR_EVTIMEOUT;
		}
	}
	else
		return CBT_ERR_CBTNOTOPEN;

	if (pPort->hOpen == NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBTNOTOPEN;
	}

	// check of a socket
	pSocket = Get_SocketCtrl(pPort, SID);

	if (pSocket == NULL) {
		fRet = CBT_ERR_CHNOTOPEN;
		goto    W_Done;
	}

	if (pSocket->fMODE == 2) {             // socket close?
		fRet = CBT_ERR_CHNOTOPEN;
		goto    W_Done;
	}

	if (pSocket->fMODE == 1) {             // received CloseChannel?
		if (pSocket->Counter == 0) {
			hEvent = pSocket->hBUF_Obj;
			// send data buffer
			if (pSocket->lpTxBuf != NULL)
				VirtualFree((LPVOID)(pSocket->lpTxBuf), 0, MEM_RELEASE);
			// receive data buffer
			if (pSocket->lpRxBuf != NULL)
				VirtualFree((LPVOID)(pSocket->lpRxBuf), 0, MEM_RELEASE);
			// socket management data
			Eject_SocketCtrl(pPort, pSocket);
			LocalFree((HANDLE)pSocket);

			if (hEvent != NULL)
				CloseHandle(hEvent);
		}
		fRet = CBT_ERR_CHNOTOPEN;
		goto    W_Done;
	}

	/* Is send from a host to a printer admitted ? */
	if (pSocket->PtSsize == 0) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    W_Done;
	}

	if (pSocket->lpTxBuf == NULL) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    W_Done;
	}

	if (pSocket->szTxBuf == 0) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    W_Done;
	}

	pSocket->Counter += 1;

	// send process to a printer
	lpSource = lpBuffer;
	RestSize = TxCount;             // Size of total
	TxCount = 0;

	while (RestSize > 0) {

		if (pPort->hOpen == NULL) {
			fRet = CBT_ERR_EXITED;              // Error or Exit
			break;
		}

		if (pSocket->Credit_P == 0) {
			// CreditRequest Command  Write & Reply check
			fRet = CreditRequest_Command(pPort, SID, Lpt_Credit, 0xffff);
			// Limited Credit   CreditRequested = 8

			//                  Maximum Outstanding = FFFF
			if (fRet < 0)
				break;
			// was not able to get credit
			if (fRet == 0) {
				Sleep(50);
				fRet = CBT_ERR_FNCDISABLE;
				break;                      // retry
			}
			pSocket->Credit_P += fRet;          // add credit
		}

		// Data Packet  Write
		fRet = DataWrite_Fnc(pPort, pSocket, lpSource, RestSize);
		if (fRet < 0)
			break;

		(pSocket->Credit_P)--;          // credit
		TxCount += fRet;               // sending completion data length
		RestSize -= fRet;               // not sending data length
		lpSource += fRet;               // lpBuffer offset

	}       // while

	*lpSize = TxCount;

	if (fRet >= 0)
		fRet = TxCount;

	pSocket->Counter -= 1;

W_Done:
	pPort->Counter -= 1;

	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);

	return fRet;
}


/******************************************************************************\
*
*  FUNCTION: int ECBT_Read (HANDLE hECBT,int SID,LPBYTE lpBuffer,LPINT lpSize)
*
*  INPUTS:      hECBT       - ECBT handle
*               SID         - 0x02 Control Data channel
*                             0x40 JOB Data channel
*               lpBuffer    - receive data buffer
*                             (NULL = return the data size that can receive)
*               lpSize      - receive data size
*
*  RETURNS:     Normal      - The data size that received
*                             (= lpSize)
*                             In case of lpBuffer=NULL, return the data size that can receive
*               Error       - end code (negative number)
*
*  GLOBAL VARS: none
*
*  COMMENTS:    Receive data from a printer.
*
\******************************************************************************/

int ECBT_Read(HANDLE hECBT, BYTE SID, LPBYTE lpBuffer, LPINT lpSize)
{
	int     fRet;
	LPPORT      pPort = (LPPORT)hECBT;
	LPSOCKET    pSocket;
	int     DataSize;           // Length of data of receive queue
	WORD        wCredit;            // Credit to send to a printer

	int     RxCount = 0;            // Length of read data
	LPBYTE      lpSource;
	int     cnt;
	HANDLE      hEvent;


	fRet = HandleCheck(hECBT);
	if (fRet < 0)
		return  fRet;

	if (pPort->Status != CBT_STS_PACKETMODE)
		return CBT_ERR_FNCDISABLE;

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL) {
		pPort->Counter += 1;            // add process function counter
		if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hFunc_Obj, 60000)) {
			pPort->Counter -= 1;
			return CBT_ERR_EVTIMEOUT;
		}
	}
	else
		return CBT_ERR_CBTNOTOPEN;

	if (pPort->hOpen == NULL) {
		pPort->Counter -= 1;
		if (pPort->hFunc_Obj != NULL)
			SetEvent(pPort->hFunc_Obj);
		return CBT_ERR_CBTNOTOPEN;
	}

	// check of a socket
	pSocket = Get_SocketCtrl(pPort, SID);

	if (pSocket == NULL) {
		fRet = CBT_ERR_CHNOTOPEN;
		goto    R_Done;
	}

	if (pSocket->fMODE == 2) {             // socket close?
		fRet = CBT_ERR_CHNOTOPEN;
		goto    R_Done;
	}

	if (pSocket->fMODE == 1) {             // received CloseChannel ?
		if (pSocket->Counter == 0) {
			hEvent = pSocket->hBUF_Obj;
			// send data buffer
			if (pSocket->lpTxBuf != NULL)
				VirtualFree((LPVOID)(pSocket->lpTxBuf), 0, MEM_RELEASE);
			// receive data buffer
			if (pSocket->lpRxBuf != NULL)
				VirtualFree((LPVOID)(pSocket->lpRxBuf), 0, MEM_RELEASE);
			// socket management data
			Eject_SocketCtrl(pPort, pSocket);
			LocalFree((HANDLE)pSocket);

			if (hEvent != NULL)
				CloseHandle(hEvent);
		}
		fRet = CBT_ERR_CHNOTOPEN;
		goto    R_Done;
	}

	// Is send from a printer to a host admitted ?
	if (pSocket->StPsize == 0) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    R_Done;
	}

	if (pSocket->lpRxBuf == NULL) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    R_Done;
	}

	if (pSocket->szRxBuf == 0) {
		fRet = CBT_ERR_FNCDISABLE;
		goto    R_Done;
	}

	fRet = CBT_ERR_NORMAL;

	pSocket->Counter += 1;


	if (lpBuffer == NULL) {

		// 1. Get receive size (case of lpBuffer=NULL) 

		DataSize = pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop; // Data size in buffer

																	// do not send Credit from ReadThread (dead lock)

		if (pSocket->Credit_H == 0) {
			wCredit = (WORD)((pSocket->szRxBuf - (DWORD)DataSize) / (pSocket->StPsize - 6));
			if (wCredit > 0) {
				wCredit = 1;
				// Credit Command  Write & Reply check
				fRet = Credit_Command(pPort, SID, wCredit);
			}
		}

		pPort->Wait_Channel = SID;
		SetEvent(pPort->hRX_Obj);           // Receive thread -> ON

		WaitForSingleObject(pPort->hDAT_Obj, Timeout_Time);

		pPort->Wait_Channel = 0;

		fRet = pPort->fRTerror;

		DataSize = pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop;

		RxCount = DataSize;
		if (lpSize != NULL)
			*lpSize = DataSize;

	}
	else {
		// 2. Get receive data (lpBuffer!=NULL)

		if (WAIT_TIMEOUT == WaitForSingleObject(pSocket->hBUF_Obj, Timeout_Time)) {  // 5sec
			fRet = CBT_ERR_BUFLOCKED;
		}
		else {
			DataSize = pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop; // Data size in buffer

			if (*lpSize < DataSize)
				DataSize = *lpSize;

			lpSource = pSocket->lpRxBuf + pSocket->offsetRxBDTop;       // Data address in buffer

			memcpy(lpBuffer, lpSource, DataSize);

			pSocket->offsetRxBDTop += DataSize;             // add offset

			RxCount = DataSize;
			*lpSize = DataSize;

			DataSize = pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop;

			if (DataSize == 0) {
				// reset pointer
				pSocket->offsetRxBDTop = 0;
				pSocket->offsetRxBDEnd = 0;
			}
			else {
				lpSource = pSocket->lpRxBuf + pSocket->offsetRxBDTop;
				for (cnt = 0; cnt < DataSize; cnt++) {
					*(pSocket->lpRxBuf + cnt) = *(lpSource + cnt);
				}
				pSocket->offsetRxBDTop = 0;
				pSocket->offsetRxBDEnd = DataSize;
			}

			SetEvent(pSocket->hBUF_Obj);
		}
	}

	if (fRet > CBT_ERR_FATAL)
		fRet = RxCount;

	pSocket->Counter -= 1;

R_Done:
	pPort->Counter -= 1;

	Terminate_Fnc(pPort, fRet);

	// Synchronization of function - start
	if (pPort->hFunc_Obj != NULL)
		SetEvent(pPort->hFunc_Obj);

	return fRet;
}




//------------------------------------------------------------------------------
//
//  FUNCTION:   int Terminate_Fnc(LPPORT pPort,int RetCode)
//
//  INPUTS:     pPort       - ECBT handle
//              RetCode     - The error code which caused it
//
//  RETURNS:    CBT_ERR_NORMAL
//
//  COMMENTS:   EXIT process at fatal error occurred or having received error packet
//              (work in the CriticalSection)
//
//------------------------------------------------------------------------------
int Terminate_Fnc(LPPORT pPort, int RetCode)
{
	if (RetCode > CBT_ERR_FATAL)
		if (pPort->Status != CBT_STS_MUSTCLOSE)
			return CBT_ERR_NORMAL;

	// transmit ErrorPacket in case of fatal error-->do exit of a printer
	if ((RetCode <= CBT_ERR_FATAL) &&
		(pPort->Status != CBT_STS_AFTERERROR)) {

		switch (RetCode) {
		case  CBT_ERR_GETERRORPACKET:
			break;

		case  CBT_ERR_MULFORMEDPACKET:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x80);
			break;

		case  CBT_ERR_UEPACKET:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x81);
			break;

		case  CBT_ERR_UEREPLY:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x82);
			break;

		case  CBT_ERR_BIGGERPACKET:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x83);
			break;

		case  CBT_ERR_UEDATA:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x84);
			break;

		case  CBT_ERR_UERESULT:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x85);
			break;

		case  CBT_ERR_PIGGYCREDIT:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x86);
			break;

		case  CBT_ERR_UNKNOWNREPLY:
			Tx_ErrorPacket(pPort, (BYTE)0, (BYTE)0x87);
			break;

		case  CBT_ERR_NOMEMORY:
			break;

		default:
			;
		}

		pPort->Status = CBT_STS_AFTERERROR;
	}

	// When function competed with the other thread, do Terminate
	// by process of the last thread.

	if (pPort->Counter == 0) {

		// ReadThread finished
		if (pPort->hRThread != NULL) {


			pPort->fRTstat = RT_Exit;   // Exit
			SetEvent(pPort->hRX_Obj);   // Read Event

			if (pPort->hRThread_Obj != NULL) {
				if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRThread_Obj, 15000)) {
					TerminateThread(pPort->hRThread, (DWORD)0);          // Compulsion exiting
				}
				CloseHandle(pPort->hRThread_Obj);
				pPort->hRThread_Obj = NULL;
			}
			else
				WaitForSingleObject(pPort->hRThread, 15000);         // Compulsion exiting

																	 // ExitThread with ReadThread is not completed yet
			Sleep(20);


			CloseHandle(pPort->hRThread);
			pPort->hRThread = NULL;
			pPort->fRTstat = RT_NotAct;
			pPort->fRTerror = CBT_ERR_NORMAL;
		}

		if (pPort->hRX_Obj != NULL) {       // Read thread execution event object
			CloseHandle(pPort->hRX_Obj);
			pPort->hRX_Obj = NULL;
		}

		if (pPort->hCMD_Obj != NULL) {      // CH0 command synchronization event object
			CloseHandle(pPort->hCMD_Obj);
			pPort->hCMD_Obj = NULL;
		}

		if (pPort->hRPL_Obj != NULL) {      // CH0 reply reception completion event object
			CloseHandle(pPort->hRPL_Obj);
			pPort->hRPL_Obj = NULL;
		}

		if (pPort->hDAT_Obj != NULL) {      // Data reception completion event object
			CloseHandle(pPort->hDAT_Obj);
			pPort->hDAT_Obj = NULL;
		}

		Free_AllSocketAndBuffer(pPort);

		pPort->hOpen = NULL;
		pPort->lpSocketCtrl = NULL;
		pPort->Status = CBT_STS_NONPACKET;

		pPort->Pre_Reply = 0;
		pPort->Wait_Reply = 0;
		pPort->Wait_Channel = 0;
	}

	return CBT_ERR_NORMAL;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Init_Command (LPPORT pPort)            
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    Normal      - 0x00
//              Error       - end code (negative number)
//
//  COMMENTS:   CBT  Init Command (0x00)
//
//------------------------------------------------------------------------------
int Init_Command(LPPORT pPort)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[Init_Sz];

	BUFFER[0] = 0x00;      // PSID
	BUFFER[1] = 0x00;      // SSID
	BUFFER[2] = 0x00;      // Length(Hi)
	BUFFER[3] = Init_Sz;   // Length(Lo)
	BUFFER[4] = 0x01;      // Credit
	BUFFER[5] = 0x00;      // Control
	BUFFER[6] = 0x00;      // Init Command
	BUFFER[7] = 0x10;      // Revision

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_INIT;

	fRet = Write_Fnc(pPort, BUFFER, Init_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00)
			fRet = CBT_ERR_NORMAL;
		else
			if (pPort->CH0BUFFER[07] == 0x01)
				fRet = CBT_ERR_INITDENIED;
			else
				if (pPort->CH0BUFFER[07] == 0x02)
					fRet = CBT_ERR_VERSION;
				else
					if (pPort->CH0BUFFER[07] == 0x0b)
						fRet = CBT_ERR_INITFAILED;
					else
						fRet = CBT_ERR_UERESULT;
	}

	if (pPort->Wait_Reply == CBT_RPY_INIT)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:  int OpenChannel_Command (LPPORT pPort, BYTE SID,
//                                          LPWORD pPtSsz, LPWORD pStPsz
//                                          WORD CreditReq, WORD MaxCredit)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//              pPtSsz      - Pointer to PrimaryToSecondaryPacketSize
//              pStPsz      - Pointer to SecondaryToPrimaryPacketSize
//              CreditReq   - Value of CreditRequested
//              MaxCredit   - Value of MaximumOutstandingCredit
//
//  RETURNS:    Normal      - Credit from a printer
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  OpenChannel Command (0x01)
//
//------------------------------------------------------------------------------
int OpenChannel_Command(LPPORT pPort, BYTE SID, LPWORD pPtSsz, LPWORD pStPsz,
	WORD CreditReq, WORD MaxCredit)
{
	int     fRet;
	int     cnt;
	WORD    size;
	/* LPBYTE  pbsize = (LPBYTE)&size;*/
	WORD    credit;
	/*LPBYTE  pbcredit = (LPBYTE)&credit;*/

	BYTE    BUFFER[OpenChannel_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = OpenChannel_Sz;        // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x01;                  // OpenChannel Command
	BUFFER[7] = SID;                   // Primary   SocketID
	BUFFER[8] = SID;                   // Secondary SocketID
	BUFFER[9] = HIBYTE(*pPtSsz);      // PrimaryToSecondaryPktSiz(High)
	BUFFER[10] = LOBYTE(*pPtSsz);      //                         (Low)
	BUFFER[11] = HIBYTE(*pStPsz);      // SecondaryToPrimaryPktSiz(High)
	BUFFER[12] = LOBYTE(*pStPsz);      //                         (Low)
	BUFFER[13] = HIBYTE(CreditReq);    // CreditRequested(High)
	BUFFER[14] = LOBYTE(CreditReq);    //                (Low)
	BUFFER[15] = HIBYTE(MaxCredit);    // MaximumOutstandingCredit(High)
	BUFFER[16] = LOBYTE(MaxCredit);    //                         (Low)

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_OPENCHANNEL;

	fRet = Write_Fnc(pPort, BUFFER, OpenChannel_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00)
			fRet = CBT_ERR_NORMAL;
		else
			if (pPort->CH0BUFFER[07] == 0x04)
				fRet = CBT_ERR_RESOURCE;
			else
				if (pPort->CH0BUFFER[07] == 0x05)
					fRet = CBT_ERR_OPENCHANNEL;
				else
					if (pPort->CH0BUFFER[07] == 0x06)
						fRet = CBT_ERR_NORMAL;
					else
						if (pPort->CH0BUFFER[07] == 0x09)
							fRet = CBT_ERR_CHNOTSUPPORT;
						else
							if (pPort->CH0BUFFER[07] == 0x0C)
								fRet = CBT_ERR_PACKETSIZE;
							else
								if (pPort->CH0BUFFER[07] == 0x0D)
									fRet = CBT_ERR_NULLPACKETSZ;
								else
									fRet = CBT_ERR_UERESULT;
	}

	if (fRet == CBT_ERR_NORMAL) {

		/* kitayama - Wed Jun 11 18:51:23 2003 */
		// PtS PacketSize
		size = pPort->CH0BUFFER[11] + ((WORD)(pPort->CH0BUFFER[10]) << 8);
		if (*pPtSsz > size)
			*pPtSsz = size;

		// StP PacketSize
		size = pPort->CH0BUFFER[13] + ((WORD)(pPort->CH0BUFFER[12]) << 8);
		if (*pStPsz > size)
			*pStPsz = size;

		// Credit from a printer
		credit = pPort->CH0BUFFER[15] + ((WORD)(pPort->CH0BUFFER[14]) << 8);

		/* 		// PtS PacketSize */
		/* 		*(pbsize + 0) = pPort->CH0BUFFER[11];   // Low */
		/* 		*(pbsize + 1) = pPort->CH0BUFFER[10];   // Hi */
		/* 		if  (*pPtSsz > size) */
		/* 			*pPtSsz = size; */

		/* 		// StP PacketSize */
		/* 		*(pbsize + 0) = pPort->CH0BUFFER[13];   // Low */
		/* 		*(pbsize + 1) = pPort->CH0BUFFER[12];   // Hi */
		/* 		if  (*pStPsz > size) */
		/* 			*pStPsz = size; */

		/* 		// Credit from a printer */
		/* 		*(pbcredit + 0) = pPort->CH0BUFFER[15]; // Low */
		/* 		*(pbcredit + 1) = pPort->CH0BUFFER[14]; // Hi */

		/* 		fRet = (int)credit; */
		/* kitayama - end */
	}

	if (pPort->Wait_Reply == CBT_RPY_OPENCHANNEL)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  CloseChannel_Command (LPPORT pPort, BYTE SID)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  CloseChannel Command (0x02)
//
//------------------------------------------------------------------------------
int CloseChannel_Command(LPPORT pPort, BYTE SID)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[CloseChannel_Sz];        // Draft 1.50


	BUFFER[0] = 0x00;              // PSID
	BUFFER[1] = 0x00;              // SSID
	BUFFER[2] = 0x00;              // Length(Hi)
	BUFFER[3] = CloseChannel_Sz;   // Length(Lo)
	BUFFER[4] = 0x01;              // Credit
	BUFFER[5] = 0x00;              // Control
	BUFFER[6] = 0x02;              // CloseChannel Command
	BUFFER[7] = SID;               // Primary   SocketID
	BUFFER[8] = SID;               // Secondary SocketID
	BUFFER[9] = 0x00;              // Closing Mode(00:Nomal/01:Flash)

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_CLOSECHANNEL;

	fRet = Write_Fnc(pPort, BUFFER, CloseChannel_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;


	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00)
			fRet = CBT_ERR_NORMAL;
		else
			if (pPort->CH0BUFFER[07] == 0x03)
				fRet = CBT_ERR_CLOSEDENIED;
			else
				if (pPort->CH0BUFFER[07] == 0x08)
					fRet = CBT_ERR_CMDDENIED;
				else
					fRet = CBT_ERR_UERESULT;
	}

	if (pPort->Wait_Reply == CBT_RPY_CLOSECHANNEL)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_CloseChannelReply (LPPORT pPort, BYTE SID, int Error)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary = Secondary)
//              Error       - ECBT error code
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  CloseChannelReply Command (0x82)
//
//------------------------------------------------------------------------------
int Tx_CloseChannelReply(LPPORT pPort, BYTE SID, int Error)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[CloseChannelReply_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = CloseChannelReply_Sz;  // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x82;                  // CreditReply Command
	BUFFER[8] = SID;                   // Primary   SocketID
	BUFFER[9] = SID;                   // Secondary SocketID

	switch (Error) {
	case    CBT_ERR_CLOSEDENIED:
		BUFFER[7] = 0x03;          // Result
		break;
	case    CBT_ERR_CHNOTOPEN:
		BUFFER[7] = 0x08;          // Result
		break;
	default:
		BUFFER[7] = 0x00;          // Result
	}

	fRet = Write_Fnc(pPort, BUFFER, CloseChannelReply_Sz, &cnt);

	if (fRet >= 0)
	{
		if (cnt == CloseChannelReply_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_TXIMPERFECT;
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Credit_Command (LPPORT pPort, BYTE SID, WORD Credit)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//              Credit      - Credit to issue
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  Credit Command (0x03)
//
//------------------------------------------------------------------------------
int Credit_Command(LPPORT pPort, BYTE SID, WORD Credit)
{
	int         fRet;
	int         cnt;
	LPSOCKET    pSocket;

	BYTE        BUFFER[Credit_Sz];

	BUFFER[0] = 0x00;              // PSID
	BUFFER[1] = 0x00;              // SSID
	BUFFER[2] = 0x00;              // Length(Hi)
	BUFFER[3] = Credit_Sz;         // Length(Lo)
	BUFFER[4] = 0x01;              // Credit
	BUFFER[5] = 0x00;              // Control
	BUFFER[6] = 0x03;              // Credit Command
	BUFFER[7] = SID;               // Primary   SocketID
	BUFFER[8] = SID;               // Secondary SocketID
	BUFFER[9] = HIBYTE(Credit);        // Credit(Hi)
	BUFFER[10] = LOBYTE(Credit);        //       (Lo)

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_CREDIT;

	fRet = Write_Fnc(pPort, BUFFER, Credit_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	pSocket = Get_SocketCtrl(pPort, SID);
	if (pSocket != NULL)
		pSocket->Credit_H += Credit;

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00)
			fRet = CBT_ERR_NORMAL;
		else
			if (pPort->CH0BUFFER[07] == 0x07)
				fRet = CBT_ERR_CREDITOVF;
			else
				if (pPort->CH0BUFFER[07] == 0x08)
					fRet = CBT_ERR_CMDDENIED;
				else
					fRet = CBT_ERR_UERESULT;
	}

	if (pPort->Wait_Reply == CBT_RPY_CREDIT)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_CreditReply (LPPORT pPort, BYTE SID, int Error)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//              Error       - ECBT error code
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  CreditReply Command (0x83)
//
//------------------------------------------------------------------------------
int Tx_CreditReply(LPPORT pPort, BYTE SID, int Error)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[CreditReply_Sz];

	BUFFER[0] = 0x00;              // PSID
	BUFFER[1] = 0x00;              // SSID
	BUFFER[2] = 0x00;              // Length(Hi)
	BUFFER[3] = CreditReply_Sz;    // Length(Lo)
	BUFFER[4] = 0x01;              // Credit
	BUFFER[5] = 0x00;              // Control
	BUFFER[6] = 0x83;              // CreditReply Command
	BUFFER[8] = SID;               // Primary   SocketID
	BUFFER[9] = SID;               // Secondary SocketID

	switch (Error) {
	case    CBT_ERR_CREDITOVER:
		BUFFER[7] = 0x07;          // Result
		break;
	case    CBT_ERR_CHNOTOPEN:
		BUFFER[7] = 0x08;          // Result
		break;
	default:
		BUFFER[7] = 0x00;          // Result
	}

	fRet = Write_Fnc(pPort, BUFFER, CreditReply_Sz, &cnt);

	if (fRet >= 0)
	{
		if (cnt == CreditReply_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_TXIMPERFECT;
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  CreditRequest_Command (LPPORT pPort, BYTE SID,
//                                              WORD CreditReq, WORD MaxCredit)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//              pPtSsz      - Pointer to PrimaryToSecondaryPacketSize
//              pStPsz      - Pointer to SecondaryToPrimaryPacketSize
//              CreditReq   - Value of CreditRequested
//              MaxCredit   - Value of MaximumOutstandingCredit
//
//  RETURNS:    Normal      - Credit from a printer
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  CreditRequest Command (0x04)
//
//------------------------------------------------------------------------------
int CreditRequest_Command(LPPORT pPort, BYTE SID,
	WORD CreditReq, WORD MaxCredit)
{
	int     fRet;
	int     cnt;
	WORD    credit;
	/*LPBYTE  pbcredit = (LPBYTE)&credit;*/

	BYTE    BUFFER[CreditRequest_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = CreditRequest_Sz;      // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x04;                  // CreditRequest Command
	BUFFER[7] = SID;                   // Primary   SocketID
	BUFFER[8] = SID;                   // Secondary SocketID
	BUFFER[9] = HIBYTE(CreditReq);    // CreditRequested(High)
	BUFFER[10] = LOBYTE(CreditReq);    //                (Low)
	BUFFER[11] = HIBYTE(MaxCredit);    // MaximumOutstandingCredit(High)
	BUFFER[12] = LOBYTE(MaxCredit);    //                         (Low)


	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_CREDITREQUEST;

	fRet = Write_Fnc(pPort, BUFFER, CreditRequest_Sz, &cnt);

	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;


	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00)
			fRet = CBT_ERR_NORMAL;
		else
			if (pPort->CH0BUFFER[07] == 0x08)
				fRet = CBT_ERR_CMDDENIED;
			else
				fRet = CBT_ERR_UERESULT;
	}

	if (fRet == CBT_ERR_NORMAL) {
		/* kitayama - Wed Jun 11 18:59:52 2003 */
		credit = pPort->CH0BUFFER[11] + ((WORD)(pPort->CH0BUFFER[10]) << 8);
		/* 		*(pbcredit + 0) = pPort->CH0BUFFER[11]; // Low */
		/* 		*(pbcredit + 1) = pPort->CH0BUFFER[10]; // Hi */
		fRet = (int)credit;
		/* kitayama - end */
	}

	if (pPort->Wait_Reply == CBT_RPY_CREDITREQUEST)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_CreditRequestReply (LPPORT pPort, BYTE SID, WORD wCredit,
//                                                                      int Error)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID (Primary=Secondary)
//              wCredit     - Credit
//              Error       - ECBT error code
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  CreditRequestReply Command (0x84)
//
//------------------------------------------------------------------------------
int Tx_CreditRequestReply(LPPORT pPort, BYTE SID, WORD wCredit, int Error)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[CreditRequestReply_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = CreditRequestReply_Sz; // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x84;                  // CreditRequestReply Command
	BUFFER[8] = SID;                   // Primary   SocketID
	BUFFER[9] = SID;                   // Secondary SocketID
	BUFFER[10] = HIBYTE(wCredit);       // Credit (Hi)
	BUFFER[11] = LOBYTE(wCredit);       // Credit (Lo)

	switch (Error) {
	case    CBT_ERR_CHNOTOPEN:
		BUFFER[7] = 0x08;          // Result
		break;
	default:
		BUFFER[7] = 0x00;          // Result(success)
	}

	fRet = Write_Fnc(pPort, BUFFER, CreditRequestReply_Sz, &cnt);

	if (fRet >= 0)
	{
		if (cnt == CreditRequestReply_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_TXIMPERFECT;
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Exit_Command (LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  Exit Command (0x08)
//
//------------------------------------------------------------------------------
int Exit_Command(LPPORT pPort)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[Exit_Sz];

	BUFFER[0] = 0x00;      // PSID
	BUFFER[1] = 0x00;      // SSID
	BUFFER[2] = 0x00;      // Length
	BUFFER[3] = Exit_Sz;   // Length
	BUFFER[4] = 0x01;      // Credit
	BUFFER[5] = 0x00;      // Control
	BUFFER[6] = 0x08;      // Exit Command

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_EXIT;

	fRet = Write_Fnc(pPort, BUFFER, Exit_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}


	if (pPort->Wait_Reply == CBT_RPY_EXIT)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return CBT_ERR_NORMAL;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_ExitReply(LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  ExitReply Command (0x88)
//
//------------------------------------------------------------------------------
int Tx_ExitReply(LPPORT pPort)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[ExitReply_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = ExitReply_Sz;          // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit(ignored)
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x88;                  // ExitReply Command
	BUFFER[7] = 0x00;                  // Result(success)

	fRet = Write_Fnc(pPort, BUFFER, ExitReply_Sz, &cnt);

	if (fRet >= 0)
	{
		if (cnt == ExitReply_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_TXIMPERFECT;
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  GetSocketID_Command (LPPORT pPort,BYTE *lpName,int Size,
//                                                      LPBYTE lpSocketID);
//
//  INPUTS:     pPort       - ECBT handle
//              lpName      - Pointer to Service Name(Maximum 40byte)
//              Size        - Size of Service Name
//              lpSocketID  - Address to set socket ID at
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  GetSocketID Command (0x09)
//
//------------------------------------------------------------------------------
int GetSocketID_Command(LPPORT pPort, LPBYTE lpName, int Size, LPBYTE lpSocketID)
{
	int     fRet;
	int     cnt;

	BYTE    BUFFER[GetSocketID_Sz + 40];          // 1-40

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = GetSocketID_Sz +       // Length(Lo)
		LOBYTE(LOWORD(Size));
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x09;                  // GetSocketID Command

	for (cnt = 0; cnt < Size; cnt++) {
		BUFFER[7 + cnt] = *(lpName + cnt);  // Service Name
	}

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_GETSOCKETID;

	fRet = Write_Fnc(pPort, BUFFER, GetSocketID_Sz + Size, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[7] == 0x00) {
			*lpSocketID = pPort->CH0BUFFER[8];
			fRet = CBT_ERR_NORMAL;
		}
		else
			if (pPort->CH0BUFFER[07] == 0x0A)
				fRet = CBT_ERR_NOSERVICE;
			else
				fRet = CBT_ERR_UERESULT;
	}

	if (pPort->Wait_Reply == CBT_RPY_GETSOCKETID)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  GetServiceName_Command(LPPORT pPort,BYTE SID,
//                                              BYTE *lpNameBuf,int Size);
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID which requires that ServiceName
//              lpNameBuf   - Pointer of buffer to return ServiceName to
//              Size        - Size of buffer to return ServiceName to
//
//  RETURNS:    Normal      - The SeviceName size that forwarded to NameBuf
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  GetServiceName Command (0x0A)
//
//------------------------------------------------------------------------------

int GetServiceName_Command(LPPORT pPort, BYTE SID, BYTE *lpNameBuf, int Size)
{
	int     fRet;
	int     cnt;
	int     i;

	BYTE    BUFFER[GetServiceName_Sz];

	BUFFER[0] = 0x00;                  // PSID
	BUFFER[1] = 0x00;                  // SSID
	BUFFER[2] = 0x00;                  // Length(Hi)
	BUFFER[3] = GetServiceName_Sz;     // Length(Lo)
	BUFFER[4] = 0x01;                  // Credit
	BUFFER[5] = 0x00;                  // Control
	BUFFER[6] = 0x0A;                  // GetServiceName Command
	BUFFER[7] = SID;                   // SocketID


	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hCMD_Obj, Timeout_Time))
		return  CBT_ERR_WRITETIMEOUT;

	pPort->Pre_Reply = CBT_RPY_GETSERVICENAME;

	fRet = Write_Fnc(pPort, BUFFER, GetServiceName_Sz, &cnt);
	if (fRet < 0) {
		SetEvent(pPort->hCMD_Obj);
		return fRet;
	}

	fRet = CBT_ERR_NORMAL;
	SetEvent(pPort->hRX_Obj);

	if (WAIT_TIMEOUT == WaitForSingleObject(pPort->hRPL_Obj, Timeout_Time)) {
		fRet = CBT_ERR_NOREPLY;
	}

	if (pPort->fRTerror <= CBT_ERR_FATAL)
		fRet = pPort->fRTerror;

	if (fRet == CBT_ERR_NORMAL) {

		if (pPort->CH0BUFFER[07] == 0x00) {
			cnt = (int)(pPort->CH0BUFFER[3] - GetServiceNameReply_Sz);
			if (Size < cnt)
				cnt = Size;
			for (i = 0; i < cnt; i++) {
				*(lpNameBuf + i) = pPort->CH0BUFFER[GetServiceNameReply_Sz + i];
			}
			fRet = cnt;
		}
		else
			if (pPort->CH0BUFFER[07] == 0x09)
				fRet = CBT_ERR_CHNOTSUPPORT;
			else
				fRet = CBT_ERR_UERESULT;
	}

	if (pPort->Wait_Reply == CBT_RPY_GETSERVICENAME)
		pPort->Wait_Reply = CBT_RPY_NONE;

	SetEvent(pPort->hCMD_Obj);

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_EpsonPacking (LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   CBT  EpsonPackingCommand (0x45)
//
//------------------------------------------------------------------------------
int Tx_EpsonPacking(LPPORT pPort)
{
	int         fRet;
	int         cnt = 0;

	BYTE        BUFFER[EpsonPacking_Sz];

	BUFFER[0] = 0x00;
	BUFFER[1] = 0x00;
	BUFFER[2] = 0x00;
	BUFFER[3] = 0x1b;      // <esc>
	BUFFER[4] = 0x01;      // <soh>
	BUFFER[5] = 0x40;      // @
	BUFFER[6] = 0x45;      // EJL
	BUFFER[7] = 0x4a;
	BUFFER[8] = 0x4c;
	BUFFER[9] = 0x20;
	BUFFER[10] = 0x31;      // 1284.4
	BUFFER[11] = 0x32;
	BUFFER[12] = 0x38;
	BUFFER[13] = 0x34;
	BUFFER[14] = 0x2e;
	BUFFER[15] = 0x34;
	BUFFER[16] = 0x0a;      // <lf>
	BUFFER[17] = 0x40;      // @
	BUFFER[18] = 0x45;      // EJL
	BUFFER[19] = 0x4a;
	BUFFER[20] = 0x4c;
	BUFFER[21] = 0x0a;      // <lf>
	BUFFER[22] = 0x40;      // @
	BUFFER[23] = 0x45;      // EJL
	BUFFER[24] = 0x4a;
	BUFFER[25] = 0x4c;
	BUFFER[26] = 0x0a;      // <lf>


	pPort->Pre_Reply = CBT_RPY_EPSONPACKING;

	fRet = Write_Fnc(pPort, BUFFER, EpsonPacking_Sz, &cnt);

	if (fRet < 0)
		fRet = CBT_ERR_CBTDISABLE;
	else
		if (cnt == EpsonPacking_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_CBTDISABLE;

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Tx_ErrorPacket (LPPORT pPort, BYTE SID, byte Error)
//
//  INPUTS: pPort       - ECBT handle
//          SID         - Socket ID (Primary=Secondary)
//          Error       - End code
//
//  RETURNS:    Normal      - 0x00
//
//  COMMENTS:   CBT  Error Packet (0x7F)
//
//------------------------------------------------------------------------------
int Tx_ErrorPacket(LPPORT pPort, BYTE SID, byte Error)
{
	int     cnt;
	int     fRet;

	BYTE    BUFFER[Error_Sz];

	BUFFER[0] = 0x00;      // PSID
	BUFFER[1] = 0x00;      // SSID
	BUFFER[2] = 0x00;      // Length
	BUFFER[3] = Error_Sz;  // Length
	BUFFER[4] = 0x00;      // Credit
	BUFFER[5] = 0x00;      // Control
	BUFFER[6] = 0x7f;      // Error Command
	BUFFER[7] = SID;       // Primary   SocketID
	BUFFER[8] = SID;       // Secondary SocketID
	BUFFER[9] = Error;     // Errorode

	pPort->Pre_Reply = CBT_RPY_NONE;
	fRet = Write_Fnc(pPort, BUFFER, Error_Sz, &cnt);

	if (fRet >= 0)
	{
		if (fRet == Error_Sz)
			fRet = CBT_ERR_NORMAL;
		else
			fRet = CBT_ERR_TXIMPERFECT;
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Write_Fnc (LPPORT pPort, LPBYTE lpBuffer,
//                                              int Size, LPINT lpWritten)
//
//  INPUTS:     pPort       - ECBT handle
//              lpBuffer    - Address of transmit data
//              Size        - Size of transmit data
//              lpWritten   - Pointer to size of data which transmitted
//
//  RETURNS:    Normal      - Size of data which transmitted
//              Error       - End code (negative number)
//
//  COMMENTS: Transmit packet to a printer
//
//------------------------------------------------------------------------------
int Write_Fnc(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpWritten)
{
	int     fRet;

	fRet = CBT_ERR_NORMAL;

	if (pPort->Status != CBT_STS_PACKETMODE)    // NON PACKET or Must EXIT
		return CBT_ERR_INTERRUPT;

	// Synchronization of Read/Write - start

	EnterCriticalSection(pPort->lpCsPort);

	fRet = WriteFile((pPort->hOpen), lpBuffer, Size, (LPDWORD)lpWritten, (LPOVERLAPPED)NULL);

	if (fRet) {
		if (*lpWritten == Size) {
			fRet = *lpWritten;
			if (pPort->Pre_Reply != CBT_RPY_NONE)
				pPort->Wait_Reply = pPort->Pre_Reply;
		}
		else {
			fRet = CBT_ERR_TXIMPERFECT;
		}
	}
	else{	
		printf("debug: %d\n",GetLastError());
		
		fRet = CBT_ERR_WRITEERROR;
	}

	pPort->Pre_Reply = CBT_RPY_NONE;

	// Synchronization of Read/Write - end
	LeaveCriticalSection(pPort->lpCsPort);

	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  DataWrite_Fnc (LPPORT pPort, LPSOCKET pSocket,
//                                      LPBYTE lpSource, int DataSize)
//
//  INPUTS:     pPort       - ECBT handle
//              pSocket     - Socket handle
//              lpSource    - Address of transmit data
//              DataSize    - Transmit data size
//
//  RETURNS:    Normal      - The data size that transmitted
//              Error       - End code (negative number)
//
//  COMMENTS:   Transmit DATA packet to a printer
//
//------------------------------------------------------------------------------
int DataWrite_Fnc(LPPORT pPort, LPSOCKET pSocket,
	LPBYTE lpSource, int DataSize)
{
	int                 fRet;
	int                 cnt;
	DWORD               length;
	LPBYTE              pTxBuf;

	int                 wDataSize;          // Length of data of receive queue
	WORD                wCredit;            // Credit to Scanner
	BYTE                bCredit;            // piggyback Credit

	if (pPort->Status != CBT_STS_PACKETMODE)    // NON PACKET or Must EXIT
		return CBT_ERR_INTERRUPT;

	pTxBuf = pSocket->lpTxBuf;
	if (pTxBuf == NULL)
		return CBT_ERR_FNCDISABLE;

	*(pTxBuf + 0) = pSocket->SID;     // PSID
	*(pTxBuf + 1) = pSocket->SID;     // SSID

									  // piggy back Credit
	bCredit = 0x00;                 // Credit (No piggy back Credit)

	if (pSocket->SID == pPort->SCAN_Channel) {  // Scanner uses piggyback positively
		wDataSize = pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop;
		wCredit = (WORD)((pSocket->szRxBuf - (DWORD)wDataSize) / (pSocket->StPsize - 6));
		if ((wCredit > 0) && (wCredit < 256))
			if (wCredit > pSocket->Credit_H) {
				wCredit -= pSocket->Credit_H;
				bCredit = LOBYTE(wCredit);
			}
	}

	*(pTxBuf + 4) = bCredit;          // piggy back Credit


	if ((DataSize + 6) > (int)(pSocket->szTxBuf)) {
		length = pSocket->szTxBuf;
		*(pTxBuf + 5) = 0x00;         // Control (Continuation data)
	}
	else {
		length = (DWORD)DataSize + 6;
		*(pTxBuf + 5) = 0x01;         // Control (Trailing end data)
	}

	*(pTxBuf + 2) = HIBYTE((WORD)length); // Length (Hi)
	*(pTxBuf + 3) = LOBYTE((WORD)length); //        (Lo)

										  // Copy Tx data behind pTxBuf+6
	memcpy(pTxBuf + 6, lpSource, (DWORD)length - 6);

	fRet = Write_Fnc(pPort, pTxBuf, (int)length, &cnt);

	if (fRet < 0)
		return  fRet;

	pSocket->Credit_H += (WORD)bCredit;
	return  (cnt - 6);
}


//==============================================================================
//
//  FUNCTION:   int  Check_CommandReply(LPPORT pPort,BYTE * BUFFER)
//
//  INPUTS:     pPort       - ECBT handle
//              BUFFER      - CBT command (Reply)packet
//
//  RETURNS:    Normal      - 0x00  not defined
//                            0x01  receive command of CH0
//                            0x02  receive reply of CH0
//              Error       - End code (negative number)
//
//  COMMENTS:   Check it whether CBT command and Reply are right.
//              When a command was effective, execute sending of reply.
//
//==============================================================================
int Check_CommandReply(LPPORT pPort, BYTE *BUFFER)
{
	int     fRet;
	BYTE    Command;

	fRet = HeaderCheck(BUFFER);
	if (fRet < 0)
		return fRet;

	if (pPort->Wait_Reply == CBT_RPY_NONE)	/* fatal */
		if (BUFFER[6] && CBT_CMD_REPLY)
			return CBT_ERR_UEREPLY;

	Command = BUFFER[6];

	switch (BUFFER[6]) {

		// *** Init Command
	case  CBT_CMD_INIT:
		return CBT_ERR_MULFORMEDPACKET;

		// *** InitReply Command
	case  CBT_CMD_REPLY | CBT_CMD_INIT:
		if (pPort->Wait_Reply != CBT_RPY_INIT)
			return CBT_ERR_UEREPLY;
		break;

		// *** OpenChannel Command
	case  CBT_CMD_OPENCHANNEL:
		return CBT_ERR_MULFORMEDPACKET;

		// *** OpenChannelReply Command
	case  CBT_CMD_REPLY | CBT_CMD_OPENCHANNEL:
		if (pPort->Wait_Reply != CBT_RPY_OPENCHANNEL)
			return CBT_ERR_UEREPLY;
		break;

		// *** CloseChannel Command
	case  CBT_CMD_CLOSECHANNEL:
	{
		int         fRet2;
		LPSOCKET    pSocket = NULL;

		if (BUFFER[7] != BUFFER[8])      // PSID!=SSID
			return  CBT_ERR_MULFORMEDPACKET;        // inconsistency

		if (BUFFER[7] == 0x00)
			fRet = CBT_ERR_CLOSEDENIED;        // inconsistency
		else {
			// check of a socket
			pSocket = Get_SocketCtrl(pPort, BUFFER[7]);

			if (pSocket == NULL)
				fRet = CBT_ERR_CHNOTOPEN;
			else
				fRet = CBT_ERR_NORMAL;
		}

		// reply
		fRet2 = Tx_CloseChannelReply(pPort, BUFFER[7], fRet);

		if (fRet == CBT_ERR_NORMAL)
			pSocket->fMODE = 1;
		// Close
		fRet = fRet2;
		break;          // fRet is NORMAL or FATAL
	}
	// *** CloseChannelReply Command
	case  CBT_CMD_REPLY | CBT_CMD_CLOSECHANNEL:
		if (pPort->Wait_Reply != CBT_RPY_CLOSECHANNEL)
			return CBT_ERR_UEREPLY;
		break;

		// *** Credit Command
	case  CBT_CMD_CREDIT:
	{
		LPSOCKET        pSocket;
		WORD            wCredit;

		if (BUFFER[7] != BUFFER[8])      // PSID!=SSID
			return  CBT_ERR_MULFORMEDPACKET;        // inconsistency

		if (BUFFER[7] == 0x00)
			return  CBT_ERR_MULFORMEDPACKET;        // inconsistency

		fRet = CBT_ERR_NORMAL;

		// check of a socket
		pSocket = Get_SocketCtrl(pPort, BUFFER[7]);

		if (pSocket == NULL)
			fRet = CBT_ERR_CHNOTOPEN;
		else {
			// get Credit
			wCredit = (BUFFER[9] << 8) + BUFFER[10];  // Credit Low+High
			pSocket->Credit_P += (int)wCredit;
			if (pSocket->Credit_P > 0xffff) {
				pSocket->Credit_P -= (int)wCredit;
				fRet = CBT_ERR_CREDITOVER;
			}
		}
		// reply
		fRet = Tx_CreditReply(pPort, BUFFER[7], fRet);
		break;          // fRet is NORMAL or FATAL
	}

	// *** CreditReply Command
	case  CBT_CMD_REPLY | CBT_CMD_CREDIT:
		if (pPort->Wait_Reply != CBT_RPY_CREDIT)
			return CBT_ERR_UEREPLY;
		break;

		// *** CreditRequest Command
	case  CBT_CMD_CREDITREQUEST:
	{
		LPSOCKET    pSocket;
		WORD        wCredit;

		if (BUFFER[7] != BUFFER[8])              // PSID!=SSID
			return  CBT_ERR_MULFORMEDPACKET;        // inconsistency

		if (BUFFER[7] == 0x00)
			return  CBT_ERR_MULFORMEDPACKET;        // inconsistency

		fRet = CBT_ERR_NORMAL;

		//  check of a socket
		pSocket = Get_SocketCtrl(pPort, BUFFER[7]);

		if (pSocket == NULL) {
			fRet = CBT_ERR_CHNOTOPEN;
			wCredit = 0;
		}
		else {
			// CreditMode(MaximumOutstandingCredit) is ignored
			wCredit = (BUFFER[9] << 8) + BUFFER[10];  // Credit which was required
			pSocket->Credit_H += (int)wCredit;
		}
		// reply
		fRet = Tx_CreditRequestReply(pPort, BUFFER[7], wCredit, fRet);
		break;          // fRet is NORMAL or FATAL
	}

	// *** CreditRequestReply Command
	case  CBT_CMD_REPLY | CBT_CMD_CREDITREQUEST:
		if (pPort->Wait_Reply != CBT_RPY_CREDITREQUEST)
			return CBT_ERR_UEREPLY;
		break;

		// *** Exit Command
	case  CBT_CMD_EXIT:
		fRet = Tx_ExitReply(pPort);
		pPort->Status = CBT_STS_MUSTCLOSE;
		break;

		// *** ExitReply Command
	case  CBT_CMD_REPLY | CBT_CMD_EXIT:
		if (pPort->Wait_Reply != CBT_RPY_EXIT)
			return CBT_ERR_UEREPLY;
		break;

		// *** EpsonPackingCommand Packet
	case  CBT_CMD_EPSONPACKING:
		return CBT_ERR_MULFORMEDPACKET;

		// *** EpsonPackingCommandReply Packet
	case  CBT_CMD_REPLY | CBT_CMD_EPSONPACKING:
		if (pPort->Wait_Reply != CBT_RPY_EPSONPACKING)
			return CBT_ERR_UEREPLY;
		break;

		// *** ErrorPacket
	case  CBT_CMD_ERROR:
		return CBT_ERR_GETERRORPACKET;

		// *** GetSocketIDPacket
	case  CBT_CMD_GETSOCKETID:
		return CBT_ERR_MULFORMEDPACKET;

		// *** GetSocketIDReplyPacket
	case  CBT_CMD_REPLY | CBT_CMD_GETSOCKETID:
		if (pPort->Wait_Reply != CBT_RPY_GETSOCKETID)
			return CBT_ERR_UEREPLY;
		break;

		// GetServiceNamePacket
	case  CBT_CMD_GETSERVICENAME:
		return CBT_ERR_MULFORMEDPACKET;

		// GetServiceNameReplyPacket
	case  CBT_CMD_REPLY | CBT_CMD_GETSERVICENAME:
		if (pPort->Wait_Reply != CBT_RPY_GETSERVICENAME)
			return CBT_ERR_UEREPLY;
		break;

		//Command which is not defined
	default:
	{
		if (BUFFER[6] && CBT_CMD_REPLY) {   // Unknown Reply
			return CBT_ERR_UNKNOWNREPLY;
		}
		else {                              // Unknown Command
			return CBT_ERR_MULFORMEDPACKET;
		}
	}
	}

	if (fRet >= 0) {
		if (Command && CBT_CMD_REPLY)
			fRet = CBT_RCV_REPLY;       // 0x02  Received Reply of CH0
		else
			fRet = CBT_RCV_COMMAND;     // 0x01  Received Command of CH0
	}
	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  HeaderCheck(BYTE *BUFFER)
//
//  INPUTS:     *BUFFER     - Command packet
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (CBT_ERR_CBTCOMMUNICATION)
//
//  COMMENTS:   Check matching nature of CommandPacketHeader (6byte).
//
//------------------------------------------------------------------------------
int HeaderCheck(BYTE *BUFFER)
{
	int     fRet = CBT_ERR_NORMAL;

	// A check of each field
	if (BUFFER[0] != 0x00)     // PSID
		return  CBT_ERR_MULFORMEDPACKET;

	if (BUFFER[1] != 0x00)     // SSID
		return  CBT_ERR_MULFORMEDPACKET;

	if (BUFFER[2] != 0x00)     // Length(Hi)
		return  CBT_ERR_MULFORMEDPACKET;

	switch (BUFFER[6]) {            // length check

	case  CBT_CMD_INIT:
		if (BUFFER[3] != Init_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_INIT:
		if (BUFFER[3] != InitReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_OPENCHANNEL:
		if (BUFFER[3] != OpenChannel_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_OPENCHANNEL:
		if (BUFFER[3] != OpenChannelReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_CLOSECHANNEL:
		if (BUFFER[3] != CloseChannel_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_CLOSECHANNEL:
		if (BUFFER[3] != CloseChannelReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_CREDIT:
		if (BUFFER[3] != Credit_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_CREDIT:
		if (BUFFER[3] != CreditReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_CREDITREQUEST:
		if (BUFFER[3] != CreditRequest_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_CREDITREQUEST:
		if (BUFFER[3] != CreditRequestReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_EXIT:
		if (BUFFER[3] != Exit_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_EXIT:
		if (BUFFER[3] != ExitReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_EPSONPACKING:
		if (BUFFER[3] != EpsonPacking_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_EPSONPACKING:
		if (BUFFER[3] != EpsonPackingReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_GETSOCKETID:
		if (BUFFER[3] < GetSocketID_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_GETSOCKETID:
		if (BUFFER[3] < GetSocketIDReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_GETSERVICENAME:
		if (BUFFER[3] != GetServiceName_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_REPLY | CBT_CMD_GETSERVICENAME:
		if (BUFFER[3] < GetServiceNameReply_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	case  CBT_CMD_ERROR:
		if (BUFFER[3] != Error_Sz)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	default:                                // Unknown Command
		if (BUFFER[3] < 0x06)
			fRet = CBT_ERR_MULFORMEDPACKET;
		break;
	}

	return  fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  PortCheck(HANDLE hOpen)
//
//  INPUTS:     pOpen       - Port handle
//
//  RETURNS:    Normal      - Number of port structure that can open CBT (0-15)
//              Error       - End code (negative number)
//
//  COMMENTS:   Check a port Open handle.
//
//------------------------------------------------------------------------------
int PortCheck(HANDLE hOpen)
{
	int i;      // 0-15

	for (i = 0; i<PortNumber; i++) {
		if (hOpen == Port[i].hOpen)
			return CBT_ERR_CBT2NDOPEN;     // duplication
	}

	for (i = 0; i<PortNumber; i++) {
		if (Port[i].hOpen == NULL)
			return  i;
	}

	return  CBT_ERR_PORTOVER;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  GetPortNumber(HANDLE hOpen)
//
//  INPUTS:     pOpen       - Port handle
//
//  RETURNS:    Normal      - Number of port structure that can open CBT (0-15)
//              Error       - End code (negative number)
//
//  COMMENTS:   Get number of port structure of Open status.
//
//------------------------------------------------------------------------------
int GetPortNumber(HANDLE hOpen)
{
	int i;      // 1-15

	for (i = 0; i<PortNumber; i++) {
		if (hOpen == Port[i].hOpen)
			return  i;
	}

	return  CBT_ERR_CBTNOTOPEN;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  HandleCheck(HANDLE hECBT)
//
//
//  INPUTS:     hECBT       - ECBT handle
//
//  RETURNS:    Normal      - Number of port structure that a handle is registered with (0-15)
//              Error       - End code (negative number)
//
//  COMMENTS:   Check it whether an ECBT handle is effective.
//
//------------------------------------------------------------------------------

int HandleCheck(HANDLE hECBT)
{
	int i;      // 0-15 or ErrorCode   

	if (hECBT == NULL)
		return CBT_ERR_PARAM;
	else {
		for (i = 0; i<PortNumber; i++) {
			if (hECBT == &(Port[i]))
				if (Port[i].hOpen != NULL)
					return i;
		}
	}

	return CBT_ERR_CBTNOTOPEN;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   LPSOCKET  Get_SocketCtrl(LPPORT pPort, BYTE SID)
//
//  INPUTS:     pPort       - ECBT handle
//              SID         - Socket ID
//
//  RETURNS:    pSocket     - Socket handle
//              0x0000      - A socket is not opened
//
//  COMMENTS:   Get socket handle.
//
//------------------------------------------------------------------------------
LPSOCKET  Get_SocketCtrl(LPPORT pPort, BYTE SID)
{
	LPSOCKET    pSocket;

	if (pPort->lpSocketCtrl == NULL)
		return NULL;

	pSocket = pPort->lpSocketCtrl;

	while (pSocket != NULL) {
		if (pSocket->SID == SID)
			return  pSocket;
		else
			pSocket = (LPSOCKET)(pSocket->PostSocket);

	}

	return NULL;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   VOID  Link_SocketCtrl(LPPORT pPort,
//                                          LPSOCKET lpSocket)
//
//  INPUTS:     pPort       - ECBT handle
//              lpSocket    - Socket handle
//
//  RETURNS:    VOID        
//
//  COMMENTS:   Register socket management data.
//
//------------------------------------------------------------------------------
VOID Link_SocketCtrl(LPPORT pPort, LPSOCKET lpSocket)
{
	LPSOCKET        ptr;

	if (pPort->lpSocketCtrl == NULL) {
		pPort->lpSocketCtrl = (LPVOID)lpSocket;
		lpSocket->PreSocket = NULL;
		lpSocket->PostSocket = NULL;
		return;
	}

	ptr = (LPSOCKET)(pPort->lpSocketCtrl);  // top

	while (ptr->PostSocket != NULL) {
		ptr = (LPSOCKET)(ptr->PostSocket);
	}

	ptr->PostSocket = (LPVOID)lpSocket;

	lpSocket->PreSocket = (LPVOID)ptr;
	lpSocket->PostSocket = NULL;

	return;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   VOID  Eject_SocketCtrl(LPPORT pPort,
//                                          LPSOCKET lpSocket)
//
//  INPUTS:     pPort       - ECBT handle
//              lpSocket    - Socket handle
//
//  RETURNS:    VOID        
//
//  COMMENTS:   Delete socket management data.
//
//------------------------------------------------------------------------------
VOID Eject_SocketCtrl(LPPORT pPort, LPSOCKET lpSocket)
{
	LPSOCKET        ptr;

	if (lpSocket->PreSocket == NULL)
		pPort->lpSocketCtrl = lpSocket->PostSocket;
	else {
		ptr = (LPSOCKET)(lpSocket->PreSocket);
		ptr->PostSocket = lpSocket->PostSocket;
	}

	if (lpSocket->PostSocket != NULL) {
		ptr = (LPSOCKET)(lpSocket->PostSocket);
		ptr->PreSocket = lpSocket->PreSocket;
	}

	return;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Close_AllSocket(LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    Normal      - 0x00
//              Error       - End code (negative number)
//
//  COMMENTS:   Close all sockets.
//
//------------------------------------------------------------------------------
int Close_AllSocket(LPPORT pPort)
{
	LPSOCKET    pSocket, nextSocket;
	int         fRet;

	pSocket = pPort->lpSocketCtrl;
	nextSocket = NULL;

	while (pSocket != NULL) {

		// Saving
		nextSocket = (LPSOCKET)(pSocket->PostSocket);

		fRet = ECBT_CloseChannel((HANDLE)pPort, pSocket->SID);

		if (fRet <= CBT_ERR_FATAL)
			return  fRet;

		pSocket = nextSocket;
	}
	return  CBT_ERR_NORMAL;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   VOID  Free_AllSocketAndBuffer(LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    VOID        
//
//  COMMENTS:   Free all sockets.
//
//------------------------------------------------------------------------------
VOID Free_AllSocketAndBuffer(LPPORT pPort)
{
	LPSOCKET    pSocket, nextSocket;
	HANDLE      hEvent;

	pSocket = pPort->lpSocketCtrl;
	nextSocket = NULL;

	while (pSocket != NULL) {

		nextSocket = (LPSOCKET)(pSocket->PostSocket);

		hEvent = pSocket->hBUF_Obj;         // Saving

											// Send data buffer
		if (pSocket->lpTxBuf != NULL)
			VirtualFree((LPVOID)(pSocket->lpTxBuf), 0, MEM_RELEASE);

		// Receive data buffer
		if (pSocket->lpRxBuf != NULL)
			VirtualFree((LPVOID)(pSocket->lpRxBuf), 0, MEM_RELEASE);

		// Socket management data
		LocalFree((HANDLE)pSocket);


		if (hEvent != NULL)
			CloseHandle(hEvent);

		pSocket = nextSocket;
	}
	return;
}


///////// Read Thread

///==============================================================================
//
//  FUNCTION:  DWORD ReadThread (LPVOID param)
//
//  INPUTS:     param       - Pointer to port management data
//
//  RETURNS:    none
//
//  COMMENTS:   Thread to receive packet.
//
///==============================================================================

void ReadThread(LPVOID param)
{


	LPPORT      pPort;          // ECBT handle = Pointer to port management data structure
	int         fRet;
	int         cnt;            // Size of read data
	WORD        length;         // Length of packet
	BYTE        HD_BUFFER[256];  // Buffer for packet header
	int         BD_size;        // Data size in buffer
	int         RD_size;        // Size to read
	int         i;
	byte        wk_Socket;      // SID of DataPacket
	byte        wk_Control;     // Packet+5 (A trailing end judgment)

	LPBYTE      pTemp;          // Address of the queue which read receive data temporarily
	int         Rx_Flag = 0;    // Data receive control flag  0:Initial state  1:Data receive completion
	int         RetryCnt;       // Frequency of Retry

	LPSOCKET    pSocket;

	// Thread start
	pPort = (LPPORT)param;

	pPort->fRTerror = CBT_ERR_NORMAL;

	BD_size = 0;                            // initialize data size in buffer

											// ********* It repeats itself till do Exit of CBT
	while (pPort->fRTstat != RT_Exit) {      // 0:Non-Active 1:Active 2:Exit

		if (pPort->fRTerror <= CBT_ERR_FATAL) { // Skip
			Sleep(100);
			continue;
		}

		WaitForSingleObject(pPort->hRX_Obj, Polling_Time);   // polling once in minimum 10 sec
		if (pPort->Wait_Reply != CBT_RPY_NONE)
			Sleep(0);

		if (pPort->Wait_Channel != 0) {     // the channel which waits for receive data
			pSocket = Get_SocketCtrl(pPort, pPort->Wait_Channel);
			if (pSocket == NULL)
				pPort->Wait_Channel = 0;
			else {
				// Receive data size of a socket > 0
				if ((pSocket->offsetRxBDEnd - pSocket->offsetRxBDTop) > 0) {
					pPort->Wait_Channel = 0;
					SetEvent(pPort->hDAT_Obj);      // Data receive event
				}
			}
		}

		RetryCnt = 0;                       // reset retry counter

											// ********
	Read_Start:
		if (pPort->fRTstat == RT_Exit) {
			break;
		}

		// this includes a case to diverge
		ResetEvent(pPort->hRX_Obj);

		pTemp = NULL;
		cnt = 0;

		// fix read size
		switch (pPort->Wait_Reply) {
		case  CBT_CMD_REPLY | CBT_CMD_INIT:
			RD_size = InitReply_Sz;         // 0x09
			break;
		case  CBT_CMD_REPLY | CBT_CMD_OPENCHANNEL:
			RD_size = OpenChannelReply_Sz;      // 0x10
			break;
		case  CBT_CMD_REPLY | CBT_CMD_CLOSECHANNEL:
			RD_size = CloseChannelReply_Sz;     // 0x0A
			break;
		case  CBT_CMD_REPLY | CBT_CMD_CREDIT:
			RD_size = CreditReply_Sz;       // 0x0A
			break;
		case  CBT_CMD_REPLY | CBT_CMD_CREDITREQUEST:
			RD_size = CreditRequestReply_Sz;    // 0x0C
			break;
		case  CBT_CMD_REPLY | CBT_CMD_EXIT:
			RD_size = ExitReply_Sz;         // 0x08
			break;
		case  CBT_CMD_REPLY | CBT_CMD_EPSONPACKING:
			RD_size = EpsonPackingReply_Sz;     // 0x08
			break;
		default:
			RD_size = sizeof(HD_BUFFER);
		}

		// Synchronization of Read/Write - start
		EnterCriticalSection(pPort->lpCsPort);

		// The packet which do not process stays.
		if (BD_size > 0) {
			if (BD_size >= RD_size) {
				fRet = BD_size;
				RD_size = 0;
			}
			else {      // BD_size < RD_size
				fRet = Read_Fnc(pPort, HD_BUFFER + BD_size, RD_size - BD_size, &cnt);
				BD_size += cnt;
				fRet = BD_size;
				RD_size = 0;
			}
		}
		else {		
			fRet = Read_Fnc(pPort, HD_BUFFER, RD_size, &cnt);
			BD_size = cnt;
		}

		if (fRet <= 0) {
			// Synchronization of Read/Write - end
			LeaveCriticalSection(pPort->lpCsPort);

			RetryCnt += 1;      // retry count
			if (pPort->Wait_Channel != 0)
				Rx_Flag = 1;
			goto Read_End;
		}
		else
			RetryCnt = 0;

		if (fRet < 6) {
			// Synchronization of Read/Write - end
			LeaveCriticalSection(pPort->lpCsPort);

			fRet = CBT_ERR_RXIMPERFECT;     // FATAL Error
			goto Read_End;
		}

		length = (HD_BUFFER[2] << 8) + HD_BUFFER[3];  // get length of packet

		if (HD_BUFFER[0] != HD_BUFFER[1]) {        // check PSID/SSID
												   // Synchronization of Read/Write - end
			LeaveCriticalSection(pPort->lpCsPort);

			fRet = CBT_ERR_MULFORMEDPACKET;     // FATAL Error
			goto Read_End;
		}

		// ***** Command Packet (CH0) *****
		if (HD_BUFFER[0] == 0x00) {

			if ((length < 7) || (length > 64)) {
				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

				fRet = CBT_ERR_MULFORMEDPACKET;         // Size error (FATAL)
				goto Read_End;
			}

			// Set up data to buffer for transaction
			for (cnt = 0; (cnt<length) && (cnt<BD_size); cnt++) {
				pPort->CH0BUFFER[cnt] = HD_BUFFER[cnt];
			}
			length -= cnt;

			if (cnt < BD_size) {
				for (i = 0; i<(BD_size - cnt); i++) {
					HD_BUFFER[i] = HD_BUFFER[cnt + i];
				}
				BD_size = i;
			}
			else
				BD_size = 0;

			// read of a remainder of packet
			if (length > 0) {          // (BD_size=0)
				i = cnt;
				cnt = 0;

				fRet = Read_Fnc2(pPort, pPort->CH0BUFFER + i, (int)length, &cnt);

				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

				if ((fRet < 0) || (cnt != length)) {
					fRet = CBT_ERR_MULFORMEDPACKET;
					goto Read_End;
				}
			}
			else
				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

			fRet = Check_CommandReply(pPort, pPort->CH0BUFFER);

			if (fRet < 0) 
				goto Read_End;

			if (fRet == CBT_RCV_REPLY) {        // (0x02)

				pPort->Wait_Reply = CBT_RPY_NONE;       // clear flag
				SetEvent(pPort->hRPL_Obj);
			}

			if (fRet == CBT_RCV_COMMAND) {      // (0x01)

				if (pPort->Wait_Reply != CBT_RPY_NONE)  // Wait reply?
					goto Read_Start;
			}

			if (pPort->Wait_Channel != 0)           // wait for receive data?
				goto Read_Start;

		}
		// ***** data packet (non CH0) *****
		else {

			if (length < 6) {
				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

				fRet = CBT_ERR_MULFORMEDPACKET;     // size error (FATAL)
				goto Read_End;
			}

			length -= 6;
			cnt = 0;
			pTemp = NULL;


			//  temporary buffer
			if (length > 0) {
				if (NULL == (pTemp = (LPBYTE)VirtualAlloc(NULL, (DWORD)length,
					MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))) {
					// Synchronization of Read/Write - end
					LeaveCriticalSection(pPort->lpCsPort);

					fRet = CBT_ERR_NOMEMORY;        // FATAL
					goto Read_End;
				}

				for (cnt = 0; (cnt<length) && (cnt<BD_size - 6); cnt++) {
					pTemp[cnt] = HD_BUFFER[cnt + 6];
				}
				length -= cnt;
			}

			// read the next of data packet
			if (length > 0) {          // (BD_size=0)
				i = cnt;
				cnt = 0;

				fRet = Read_Fnc2(pPort, pTemp + i, (int)length, &cnt);

				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

				if ((fRet < 0) || (cnt != length)) {
					fRet = CBT_ERR_MULFORMEDPACKET;         // FATAL

					VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
					goto Read_End;
				}
			}
			else
				// Synchronization of Read/Write - end
				LeaveCriticalSection(pPort->lpCsPort);

			// check of a socket
			pSocket = Get_SocketCtrl(pPort, HD_BUFFER[0]);
			if (pSocket == NULL) {
				fRet = CBT_ERR_UEDATA;
				if (pTemp != NULL) {
					VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
				}
				goto Read_End;
			}

			if ((pSocket->fMODE == 2) || (pSocket->fMODE == 1)) {
				if (pPort->Wait_Channel == HD_BUFFER[0])
					Rx_Flag = 1;                    // Dummy Read completion
				fRet = CBT_ERR_NORMAL;
				if (pTemp != NULL) {
					VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
				}
				goto Read_End;
			}

			// When it received data though Credit is 0, it is protocol violation
			if (pSocket->Credit_H == 0) {
				fRet = CBT_ERR_UEPACKET;
				if (pTemp != NULL) {
					VirtualFree(pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
				}
				goto Read_End;
			}

			length = length + BD_size - 6;

			if ((length + 6) > pSocket->StPsize) {
				fRet = CBT_ERR_BIGGERPACKET;        // FATAL
				if (pTemp != NULL) {
					VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
				}
				goto Read_End;
			}

			// reflect Piggy back Credit
			pSocket->Credit_P += (int)(HD_BUFFER[4]);
			if (pSocket->Credit_P > 0xffff) {
				fRet = CBT_ERR_PIGGYCREDIT;         // FATAL
				if (pTemp != NULL) {
					VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
					pTemp = NULL;
				}
				goto Read_End;
			}

			if (length > 0) {
				LPBYTE  pDestination;

				if (WAIT_TIMEOUT == WaitForSingleObject(pSocket->hBUF_Obj, Timeout_Time)) {  // 5sec
					fRet = CBT_ERR_BUFLOCKED;
					if (pTemp != NULL) {
						VirtualFree(pTemp, 0, MEM_RELEASE);
						pTemp = NULL;
					}
					goto Read_End;
				}

				pDestination = pSocket->lpRxBuf + pSocket->offsetRxBDEnd;
				memcpy(pDestination, pTemp, length);

				pSocket->offsetRxBDEnd += length;

				VirtualFree((LPVOID)pTemp, 0, MEM_RELEASE);
				pTemp = NULL;

				SetEvent(pSocket->hBUF_Obj);
			}

			pSocket->Credit_H -= 1;

			if (pPort->Wait_Channel != 0)
				Rx_Flag = 1;

			wk_Socket = HD_BUFFER[0];
			wk_Control = HD_BUFFER[5];          // Packet+5 (A trailing end judgment)

			if (length + 6 < BD_size) {
				for (i = 0; i<(BD_size - 6 - length); i++) {
					HD_BUFFER[i] = HD_BUFFER[length + 6 + i];
				}
				BD_size = i;
			}
			else
				BD_size = 0;


			// In case of trailing end (01)
			if (pPort->Wait_Channel != 0) {
				pSocket = Get_SocketCtrl(pPort, pPort->Wait_Channel);
				if (pSocket == NULL)
					pPort->Wait_Channel = 0;
				else {
					if (pPort->Wait_Channel != wk_Socket)
						goto Read_Start;
					else {                              // Data of an appropriate socket
						pPort->Wait_Channel = 0;
						Rx_Flag = 0;
						SetEvent(pPort->hDAT_Obj);
					}
				}
			}

			if (pPort->Wait_Reply != CBT_RPY_NONE)
				goto Read_Start;
		}


		// ******
	Read_End:

		if (pPort->fRTstat == RT_Exit) {
			break;
		}


		if (fRet <= CBT_ERR_FATAL) {
			pPort->fRTerror = fRet;                 // Error code in Read thread
			if (pPort->Wait_Reply != CBT_RPY_NONE) {        // wait Reply ?
				pPort->Wait_Reply = CBT_RPY_NONE;
				SetEvent(pPort->hRPL_Obj);
			}
			if (pPort->Wait_Channel != 0) {                 // wait receive data ?
				pPort->Wait_Channel = 0;
				SetEvent(pPort->hDAT_Obj);
			}
		}
		else {
			pPort->fRTerror = CBT_ERR_NORMAL;

			if (pPort->Wait_Channel != 0) { //Data receive waiting state
				pSocket = Get_SocketCtrl(pPort, pPort->Wait_Channel);
				if (pSocket == NULL)
					pPort->Wait_Channel = 0;
				else {
					if (Rx_Flag == 1) {                 // Receive data completion ?
						pPort->Wait_Channel = 0;
						Rx_Flag = 0;
						SetEvent(pPort->hDAT_Obj);
					}
					else
						goto Read_Start;        // Received reply first
				}
			}

			if (pPort->Wait_Reply != CBT_RPY_NONE) {
				if (RetryCnt > 29)
					Sleep(300);                     //  0-9     0msec Sleep
				else                                // 10-19  100msec Sleep
					Sleep(100 * (RetryCnt / 10));     // 20-29  200msec Sleep
													  // 30-    300msec Sleep
				goto Read_Start;
			}

		}

		// wait receive ?
		if ((pPort->Wait_Reply != CBT_RPY_NONE) ||
			(pPort->Wait_Channel != 0))
			SetEvent(pPort->hRX_Obj);

		// continue
	}


	if (pPort->hRThread_Obj != NULL)
		SetEvent(pPort->hRThread_Obj);          // An exiting notice

	ExitThread((DWORD)0);

}


//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Read_Fnc (LPPORT pPort, LPBYTE lpBuffer,
//                                              int Size, LPINT lpRead)
//
//  INPUTS:     pPort       - ECBT handle
//              lpBuffer    - Address of receive buffer
//              Size        - Size of receive buffer (size to expect)
//              lpRead      - Size of data which received
//
//  RETURNS:    Normal      - The data size that received
//              Error       - End code (negative number)
//
//  COMMENTS: receive packet to a printer
//
//------------------------------------------------------------------------------
int Read_Fnc(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpRead)
{
	int     fRet;
	int     cnt;
	DWORD   wk_cnt;

	cnt = 0;                            // Total Size

ReadFnc00:

	if (pPort->hOpen == NULL) {
		return CBT_ERR_CBTNOTOPEN;      // Error or Exit
	}

	if (pPort->Status != CBT_STS_PACKETMODE) {  // NON PACKET or Must EXIT
		return CBT_ERR_INTERRUPT;
	}

	wk_cnt = 0;

	fRet = ReadFile(pPort->hOpen, lpBuffer + cnt, (Size - cnt), &wk_cnt, NULL);

	cnt += wk_cnt;


	if (wk_cnt != 0)
		if (cnt < Size)
			goto    ReadFnc00;              // Retry

	*lpRead = cnt;

	if (fRet)
		fRet = *lpRead;
	else
		fRet = CBT_ERR_READERROR;           // Read API failure

	return fRet;
}

//------------------------------------------------------------------------------
//
//  FUNCTION:   int  Read_Fnc2 (LPPORT pPort, LPBYTE lpBuffer,
//                                              int Size, LPINT lpRead)
//
//  INPUTS:     pPort       - ECBT handle
//              lpBuffer    - Address of receive buffer
//              Size        - Size of receive buffer (size to expect)
//              lpRead      - Size of data which received
//
//  RETURNS:    Normal      - The data size that received
//              Error       - End code (negative number)
//
//  COMMENTS:   receive packet to a printer ( Version to do loop till timeout )
//
//------------------------------------------------------------------------------
int Read_Fnc2(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpRead)
{
	int     fRet;
	int     cnt;
	DWORD   wk_cnt;

	DWORD   Start_Count;


	Start_Count = GetTickCount();           // msec

	cnt = 0;                            // Total Size

ReadFnc200:

	if (pPort->hOpen == NULL)
	{
		return CBT_ERR_CBTNOTOPEN;      // Error or Exit
	}
	if (pPort->Status != CBT_STS_PACKETMODE)    // NON PACKET or Must EXIT
		return CBT_ERR_INTERRUPT;


	wk_cnt = 0;

	fRet = ReadFile(pPort->hOpen, lpBuffer + cnt, Size - cnt, &wk_cnt, NULL);

	cnt += wk_cnt;


	if (cnt == Size)
		goto    ReadFnc2End;                // Completion

	if ((GetTickCount() - Start_Count) < Timeout_Time)
		goto    ReadFnc200;                 // Retry

ReadFnc2End:

	*lpRead = cnt;

	if (fRet)
		fRet = *lpRead;
	else
		fRet = CBT_ERR_READERROR;           // Read API failure

	return fRet;
}


//------------------------------------------------------------------------------
//
//  FUNCTION:   VOID  DummyRead_Fnc (LPPORT pPort)
//
//  INPUTS:     pPort       - ECBT handle
//
//  RETURNS:    none
//
//  COMMENTS:   Flush of transmit data of a printer
//
//------------------------------------------------------------------------------
VOID DummyRead_Fnc(LPPORT pPort)
{
	int     fRet;
	DWORD    cnt;
	int     Bcnt = 1024;
	BYTE    BUFFER[1024];


	if (pPort->hOpen == NULL)
		return;                     // Error or Exit

	fRet = TRUE;

	// Synchronization of Read/Write - start
	EnterCriticalSection(pPort->lpCsPort);

	while (fRet) {

		cnt = 0;

		fRet = ReadFile(pPort->hOpen, BUFFER, Bcnt, &cnt, NULL);

		if (cnt < (DWORD)Bcnt)
			break;

	}

	// Synchronization of Read/Write - end
	LeaveCriticalSection(pPort->lpCsPort);

}
