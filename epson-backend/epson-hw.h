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
*                                 ECBTEG.H
*
\******************************************************************************/
#ifndef __EPSON_HW_H__
#define __EPSON_HW_H__

#include <Windows.h>

/******************************************************************************\
*                              SYMBOLIC CONSTANTS
\******************************************************************************/

// Error code
#define	CBT_ERR_NORMAL			00

#define	CBT_ERR_INITDENIED		-1	//A printer cannot start CBT
// Result=0x01 from Printer
#define CBT_ERR_VERSION			-2	//Version does not fit
// Result=0x02
#define	CBT_ERR_CLOSEDENIED		-3	//Because it is CH0, CloseChannel is impossible
// Result=0x03
#define	CBT_ERR_RESOURCE		-4	//Resource of a printer is insufficient
// Result=0x04
#define	CBT_ERR_OPENCHANNEL		-5	//(unused)
// Result=0x05
#define	CBT_ERR_CHOPENED		-6	//Channel is done Open of already
// Result=0x06
#define	CBT_ERR_CREDITOVF		-7	//Credit did overflow
// Result=0x07
#define	CBT_ERR_CMDDENIED		-8	//A command for the channel which does not do Open
// Result=0x08
#define	CBT_ERR_CHNOTSUPPORT		-9	//Channel which is not supported
// Result=0x09
#define	CBT_ERR_NOSERVICE		-10	//Service is not offered
// Result=0x0A
#define	CBT_ERR_INITFAILED		-11	//(unused)
// Result=0x0B
#define	CBT_ERR_PACKETSIZE		-12	//Appoint invalid packet size (0001-0005)
// Result=0x0C
#define	CBT_ERR_NULLPACKETSZ		-13	//Packet size was 0x0000 to Host/Printer
// Result=0x0D

#define	CBT_ERR_PARAM			-20	//Parameter error
#define	CBT_ERR_MEMORY			-21	//Memory/Resource lack
#define	CBT_ERR_CBTNOTOPEN		-22	//Port does not open it
#define	CBT_ERR_CBT2NDOPEN		-23	//Port does open already
#define	CBT_ERR_CHNOTOPEN		-24	//Channel does not open it
#define	CBT_ERR_CH2NDOPEN		-25	//Channel does open already
#define	CBT_ERR_EXITED			-26	//Because it did Exit from CBT, it do not handle a command
#define	CBT_ERR_PORTOVER		-27	//Number of Port exceeded Max
#define	CBT_ERR_RPLYPSIZE		-28	//Packet size returned over with OpenChannel is unjust
#define	CBT_ERR_CREDITOVER		-29	//Credit did overflow

#define CBT_ERR_WRITETIMEOUT		-30	//Timeout occurred with Write
#define CBT_ERR_WRITEERROR		-31	//Write API failure
#define CBT_ERR_READERROR		-32	//Read API failure
#define	CBT_ERR_FNCDISABLE		-33	//Sending or receive is impossible now
#define	CBT_ERR_EVTIMEOUT		-34	//Timeout with Event

//	The following is fatal error ---> Exit from packet mode
#define	CBT_ERR_FATAL			-50	//Fatal error
#define	CBT_ERR_GETERRORPACKET		-51	//Received error packet

#define	CBT_ERR_CBTDISABLE		-52	//EpsonPackingCommand failed
//Not able to shift to packet mode
#define CBT_ERR_INTERRUPT		-53	//Interrupted Read/Write

#define	CBT_ERR_NOMEMORY		-54	//Cannot get receive queue between communication
#define	CBT_ERR_NOREPLY			-55	//Reply does not return (5sec)
#define	CBT_ERR_BUFLOCKED		-56	//A receive buffer was locked more than 5sec

#define	CBT_ERR_TXIMPERFECT		-61	//Sending size is unjust (communication error)
#define	CBT_ERR_RXIMPERFECT		-62	//Receive size is unjust (communication error)

// send error packet
#define	CBT_ERR_MULFORMEDPACKET		-80	//Receive packet of unjust format
//  Error=0x80 to printer
#define	CBT_ERR_UEPACKET		-81	//Though did not issue Credit, it received packet
//  Error=0x81
#define	CBT_ERR_UEREPLY			-82	//Received reply of command that it did not issue
//  Error=0x82
#define	CBT_ERR_BIGGERPACKET		-83	//The data packet which received exceeds maximum size
//  Error=0x83
#define	CBT_ERR_UEDATA			-84	//The channel which received data packet is not open
//  Error=0x84
#define	CBT_ERR_UERESULT		-85	//Result which received was not defined
//  Error=0x85
#define	CBT_ERR_PIGGYCREDIT		-86	//Credit overflow by PiggyBackCredit
//  Error=0x86
#define	CBT_ERR_UNKNOWNREPLY		-87	//Received unknown Reply packet
//  Error=0x87




// ****************************************************************************
//                              STRUCTURES
// ****************************************************************************


/******************************************************************************\
*                              FUNCTION PROTOTYPES
\******************************************************************************/

BOOL APIENTRY DllMain(HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved);
int ECBT_Open(HANDLE hOpen, LPHANDLE lpHECBT);
int ECBT_Close(HANDLE hECBT);
int ECBT_OpenChannel(HANDLE hECBT, BYTE SID);
int ECBT_CloseChannel(HANDLE hECBT, BYTE SID);
int ECBT_Write(HANDLE hECBT, BYTE SID, LPBYTE lpBuffer, LPINT lpSize);
int ECBT_Read(HANDLE hECBT, BYTE SID, LPBYTE lpBuffer, LPINT lpSize);


#endif /* __EPSON_HW_H__ */
