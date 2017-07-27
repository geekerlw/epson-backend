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

/******************************************************************************\
*
*                                 ECBT_LCL.H (Local.h)
*
\******************************************************************************/
#ifndef __ECBTLCL_H
#define __ECBTLCL_H

#define	ECBTEG__

/******************************************************************************\
*                              SYMBOLIC CONSTANTS
\******************************************************************************/

#define		BUFFSIZE	64	// CH0(transaction) buffer size
#define		Timeout_Time	31000 	// Event waiting time
#define		Polling_Time	24*60*60*1000	// polling in minimum 24h once

#define		Def_DataCH	0x40	// JOB Data Channel (40H)
#define		Def_ScanCH	0x50	// Scanner Data Channel (50H)

#define		PortNumber	16	// Number of the printer which can manage

#define		Lpt_Credit	0x0080	// Credit which a printer can provide
#define		Def_Credit	0x0008	// Credit which a host gives

#define		TxPSIZE		4102	// Packet size of JOB data (4096+6)
#define		RxPSIZE		512	// Packet size of others
#define		ScanSIZE	65535	// Packet size of SCAN data (65529+6)

#define		Size128K	131072	// 128Kbytes
#define		Size64K		65536	// 64Kbytes
#define		Size32K		32768	// 32Kbytes

// Status of ReadThread
#define 	RT_NotAct	0	// not active
#define 	RT_Active	1	// active
#define		RT_Exit		2	// Exit

// Status of port
#define CBT_STS_NONPACKET	0x00	// Non packet mode
#define CBT_STS_PACKETMODE	0x01	// Packet mode
#define CBT_STS_MUSTCLOSE	0x02	// The status that must do Exit promptly
#define	CBT_STS_AFTERERROR	0x03	// After error (Error Packet)...Non Packet

// Status of receive function
#define	CBT_RCV_COMMAND		0x01	// Command receive of CH0
#define	CBT_RCV_REPLY		0x02	// receive Reply which expected

// Size of packet
#define	Init_Sz			0x08	// Init Command
#define	OpenChannel_Sz		0x11	// OpenChannel Command
#define	CloseChannel_Sz		0x0A	// CloseChannel Command
#define	Credit_Sz		0x0B	// Credit Command
#define	CreditRequest_Sz	0x0D	// CreditRequest Command
#define	Exit_Sz			0x07	// Exit Command
#define	GetSocketID_Sz		0x07	// GetSocketID Command(7-47)
#define	GetServiceName_Sz	0x08	// GetServiceName Command
#define	Error_Sz		0x0A	// Error Packet
#define	EpsonPacking_Sz		0x1B	// EpsonPackingCommand

#define	InitReply_Sz		0x09	// InitReply
#define	OpenChannelReply_Sz	0x10	// OpenChannelReply
#define	CloseChannelReply_Sz	0x0A	// CloseChannelReply
#define	CreditReply_Sz		0x0A	// CreditReply
#define	CreditRequestReply_Sz	0x0C	// CreditRequestReply
#define	ExitReply_Sz		0x08	// ExitReply
#define	GetSocketIDReply_Sz	0x09	// GetSocketIDReply(9-49)
#define	GetServiceNameReply_Sz	0x09	// GetServiceNameReply(9-49)
#define	EpsonPackingReply_Sz	0x08	// EpsonPackingCommandReply

// CBT command
#define	CBT_CMD_INIT		0x00
#define	CBT_CMD_OPENCHANNEL	0x01
#define	CBT_CMD_CLOSECHANNEL	0x02
#define	CBT_CMD_CREDIT		0x03
#define	CBT_CMD_CREDITREQUEST	0x04
#define	CBT_CMD_DEBIT		0x05	// only MLC
#define	CBT_CMD_DEBITREQUEST	0x06	// only MLC
#define	CBT_CMD_CONFIGSOCKET	0x07
#define	CBT_CMD_EXIT		0x08
#define CBT_CMD_GETSOCKETID	0x09
#define CBT_CMD_GETSERVICENAME	0x0A
#define	CBT_CMD_EPSONPACKING	0x45
#define	CBT_CMD_ERROR		0x7F
#define	CBT_CMD_REPLY		0x80	// MSB on
#define	CBT_CMD_NONE		0xFF

// Reply of command
#define	CBT_RPY_INIT		0x80
#define	CBT_RPY_OPENCHANNEL	0x81
#define	CBT_RPY_CLOSECHANNEL	0x82
#define	CBT_RPY_CREDIT		0x83
#define	CBT_RPY_CREDITREQUEST	0x84
#define	CBT_RPY_DEBIT		0x85
#define	CBT_RPY_DEBITREQUEST	0x86
#define	CBT_RPY_CONFIGSOCKET	0x87
#define	CBT_RPY_EXIT		0x88
#define CBT_RPY_GETSOCKETID	0x89
#define CBT_RPY_GETSERVICENAME	0x8A
#define	CBT_RPY_EPSONPACKING	0xC5
#define	CBT_RPY_NONE		0x00

// ****************************************************************************
//                              STRUCTURES
// ****************************************************************************

// CBT port management data structure
typedef struct	tag_PORT {
	HANDLE		hOpen; 			// Open handle (hFile or hPort)
	HANDLE		hFunc_Obj;		// Function synchronization in port
	CRITICAL_SECTION	CsPort;		// Read/Write synchronization in port
	LPCRITICAL_SECTION	lpCsPort;

	HANDLE		hRThread;		// Handle of ReadThread
	int		fRTstat;		// Status flag of ReadThread
							// Active(1)
							// Exit(2)
	int		fRTerror;		// Error code of ReadThread
	HANDLE		hRThread_Obj;		// ReadThread exiting judgment event object

	HANDLE		hRX_Obj;		// RxThread execution event object
	HANDLE		hCMD_Obj;		// CH0 Command synchronization event object
	HANDLE		hRPL_Obj;		// CH0 Reply receive completion event object
	HANDLE		hDAT_Obj;		// Data receive completion event object

	LPVOID		lpSocketCtrl;		// ->Socket management data
	int		Counter;		// Counter of function in execution
	int		Status;			// CBT Status
							// 00: Non Packet Mode
							// 01: Packet Mode (Initialized)
							// 02: Must Close CBT (Find/Receive Error)
							// 03: After error...Non Packet
	BYTE		Pre_Reply;		// Reply to wait for next (Wait_Reply)
	BYTE		Wait_Reply;		// wait for receive
	BYTE		Wait_Channel;		// The channel which waits for receive data

	BYTE		Dummy1;

	BYTE		DATA_Channel;		// SocketID of JOB data channel
	BYTE		SCAN_Channel;		// SocketID of Scanner data channel
	BYTE		Dummy2;
	BYTE		Dummy3;

	BYTE		CH0BUFFER[BUFFSIZE];
	// Reply receive buffer (CH0)
} PORT, FAR* LPPORT;


// CBT socket management data structure
typedef	struct	tag_SOCKET {
	LPVOID		PreSocket;		// ->prev
	LPVOID		PostSocket;		// ->next
	BYTE		fOpen;			// 1:finish open
	BYTE		SID;			// socket ID
	BYTE		fMODE;			// 0:Nomal
								// 1:CloseChannel receive status
								// 2:CloseChannel execution
	BYTE		Counter;		// Execution counter of channel function
	WORD		PtSsize;		// Packet size (PC->Printer)
	WORD		StPsize;		// Packet size (Printer->PC)
	int		Credit_P;		// The credit that received from a printer
	int		Credit_H;		// The credit that handed from a host
	LPBYTE		lpTxBuf;		// Tx buffer address
	DWORD		szTxBuf;		//           size
	HANDLE		hBUF_Obj;		// Synchronization event object of Rx buffer update
	LPBYTE		lpRxBuf;		// Rx buffer address
	DWORD		szRxBuf;		//           size
	DWORD		offsetRxBDTop;		// top of Rx data
	DWORD		offsetRxBDEnd;		// end of Rx data +1
} CBTSOCKET, FAR* LPSOCKET;


/******************************************************************************\
*                              FUNCTION PROTOTYPES
\******************************************************************************/

int	Terminate_Fnc(LPPORT pPort, int RetCode);
int	Init_Command(LPPORT pPort);
int	OpenChannel_Command(LPPORT pPort, BYTE SID, LPWORD pPtSsz, LPWORD pStPsz,
	WORD CreditReq, WORD MaxCredit);
int	CloseChannel_Command(LPPORT pPort, BYTE SID);
int	Tx_CloseChannelReply(LPPORT pPort, BYTE SID, int Error);
int	Credit_Command(LPPORT pPort, BYTE SID, WORD Credit);
int	Tx_CreditReply(LPPORT pPort, BYTE SID, int Error);
int	CreditRequest_Command(LPPORT pPort, BYTE SID,
	WORD CreditReq, WORD MaxCredit);
int	Tx_CreditRequestReply(LPPORT pPort, BYTE SID, WORD wCredit, int Error);
int	Exit_Command(LPPORT pPort);
int	Tx_ExitReply(LPPORT pPort);
int	GetSocketID_Command(LPPORT pPort, LPBYTE lpName, int Size,
	LPBYTE lpSocketID);
int	GetServiceName_Command(LPPORT pPort, BYTE SID, LPBYTE lpNameBuf, int Size);
int	Tx_EpsonPacking(LPPORT pPort);
int	Tx_ErrorPacket(LPPORT pPort, BYTE SID, byte Error);

// *****************************************************************************

int	Write_Fnc(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpWritten);
int	DataWrite_Fnc(LPPORT pPort, LPSOCKET pSocket, LPBYTE lpSource, int DataSize);

// *****************************************************************************

int	Check_CommandReply(LPPORT pPort, BYTE *BUFFER);
int	HeaderCheck(BYTE *BUFFER);
int	PortCheck(HANDLE hOpen);
int	GetPortNumber(HANDLE hOpen);
int	HandleCheck(HANDLE hECBT);
LPSOCKET Get_SocketCtrl(LPPORT pPort, BYTE SID);
VOID	Link_SocketCtrl(LPPORT pPort, LPSOCKET lpSocket);
VOID	Eject_SocketCtrl(LPPORT pPort, LPSOCKET lpSocket);
int	Close_AllSocket(LPPORT pPort);
VOID	Free_AllSocketAndBuffer(LPPORT pPort);

// *****************************************************************************

int	Read_Fnc(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpRead);
int	Read_Fnc2(LPPORT pPort, LPBYTE lpBuffer, int Size, LPINT lpRead);
VOID	DummyRead_Fnc(LPPORT pPort);


#endif			//  __ECBTLCL_H

