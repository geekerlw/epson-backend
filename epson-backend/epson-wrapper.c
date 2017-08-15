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

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "epson.h"
#include "epson-hw.h"
#include "epson-wrapper.h"

/* Repeat number of times when communication error with printer
occurred. */
#define ECBT_ACCSESS_WAIT_MAX 20

/* Initialize ECBT */
int start_ecbt_engine(void)
{
	return DllMain(NULL, DLL_PROCESS_ATTACH, NULL);
}

/* Finish ECBT */
int end_ecbt_engine(void)
{
	return DllMain(NULL, DLL_PROCESS_DETACH, NULL);
}

/* Open CBT and open CTRL channel at the same time */
int open_port_driver(P_CBTD_INFO p_info)
{
	int err;

	enter_critical(p_info->ecbt_accsess_critical);

	err = (int)ECBT_Open((HANDLE)p_info->devfd, &p_info->ecbt_handle);
	if (err == 0) 
		err = (int)ECBT_OpenChannel(p_info->ecbt_handle, SID_CTRL);	

	leave_critical(p_info->ecbt_accsess_critical);

	if (err)
		return 1;
	else
		return 0;
}

/* Close CBT */
int close_port_driver(P_CBTD_INFO p_info)
{
	int err;

	if (p_info->ecbt_handle == NULL)
		return 0;

	enter_critical(p_info->ecbt_accsess_critical);

	ECBT_CloseChannel(p_info->ecbt_handle, SID_CTRL);
	ECBT_CloseChannel(p_info->ecbt_handle, SID_DATA);
	err = ECBT_Close(p_info->ecbt_handle);

	leave_critical(p_info->ecbt_accsess_critical);

	if (err)
		return 1;
	else
		return 0;
}

/* Open channel */
int open_port_channel(P_CBTD_INFO p_info, char sid)
{
	int err;

	enter_critical(p_info->ecbt_accsess_critical);
	err = ECBT_OpenChannel(p_info->ecbt_handle, sid);

	leave_critical(p_info->ecbt_accsess_critical);

	if (err)
		return 1;
	return 0;
}

/* Close channel */
int close_port_channel(P_CBTD_INFO p_info, char sid)
{	
	if (p_info->ecbt_handle)
	{
		enter_critical(p_info->ecbt_accsess_critical);
		ECBT_CloseChannel(p_info->ecbt_handle, sid);
		leave_critical(p_info->ecbt_accsess_critical);
	}
	return 0;
}

/* write to ECBT */
int write_to_prt(P_CBTD_INFO p_info, char sid,
	char* buffer, int* p_size)
{
	int count = 0;
	int err = CBT_ERR_NORMAL;

	//_DEBUG_MESSAGE("In Write");
	for (count = 0; count < ECBT_ACCSESS_WAIT_MAX; count++)
	{
		err = ECBT_Write(p_info->ecbt_handle, sid, (LPBYTE)buffer, p_size);

		if ((err == CBT_ERR_FNCDISABLE
			|| err == CBT_ERR_WRITETIMEOUT
			|| err == CBT_ERR_WRITEERROR) && *p_size == 0)
		{
			/* todo: usleep(10000) */
			usleep(10000);
		}

		else
		{
			break;
		}

	}

	/* when channel was closed */
	if (err == CBT_ERR_CHNOTOPEN)
		ECBT_OpenChannel(p_info->ecbt_handle, sid);

	//_DEBUG_MESSAGE_VAL("ECBT_Write size =", *p_size);
	if (err < 0 || *p_size == 0)
	{
		//_DEBUG_MESSAGE_VAL("ECBT_Write Error code =", err);
		return 1;
	}

	//_DEBUG_MESSAGE("Out Write");
	return 0;
}

/* read from ECBT */
int read_from_prt(P_CBTD_INFO p_info, char sid,
	char* buffer, int* p_size)
{
	int count = 0;
	int err = CBT_ERR_NORMAL;

	//_DEBUG_MESSAGE("In Read");

	for (count = 0; count < ECBT_ACCSESS_WAIT_MAX; count++)
	{
		err = ECBT_Read(p_info->ecbt_handle, sid, (LPBYTE)buffer, p_size);

		if (err == CBT_ERR_FNCDISABLE
			|| err == CBT_ERR_READERROR || *p_size == 0)
		{
			/* todo: unsleep(10000) */
			usleep(10000);
		}
		else
		{
			break;
		}
	}

	/* when channel was closed */
	if (err == CBT_ERR_CHNOTOPEN)
		ECBT_OpenChannel(p_info->ecbt_handle, sid);

	//_DEBUG_MESSAGE_VAL("ECBT_Read size =", *p_size);
	if (err < 0 || *p_size == 0)
	{
		//_DEBUG_MESSAGE_VAL("ECBT_Read Error code =", err);
		return 1;
	}

	//_DEBUG_MESSAGE("Out Read");
	return 0;
}
