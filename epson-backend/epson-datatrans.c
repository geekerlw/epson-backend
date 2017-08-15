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
*
* As a special exception, AVASYS CORPORATION gives permission
* to link the code of this program with the `cbt' library and
* distribute linked combinations including the two.  You must obey
* the GNU General Public License in all respects for all of the
* code used other then `cbt'.
*/

#include <stdio.h>
#include <assert.h>
#include <tchar.h>
#include <Windows.h>

#ifndef _CRT_NO_TIME_T
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#endif

#include "epson.h"
#include "epson-thread.h"


static int _datatrans_lpr_flag;


/*
 * get printer job status, success return true 
 */
static BOOL GetJobs(HANDLE hPrinter, JOB_INFO_2 **ppJobInfo, int *pcJobs, DWORD *pStatus)
{
	DWORD			cByteNeeded, nReturned, cByteUsed;
	JOB_INFO_2		*pJobStorage = NULL;
	PRINTER_INFO_2	*pPrinterInfo = NULL;

	/* Get the buffer size needed. */
	if (!GetPrinter(hPrinter, 2, NULL, 0, &cByteNeeded)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return FALSE;
	}

	pPrinterInfo = (PRINTER_INFO_2 *)malloc(cByteNeeded);
	if (!(pPrinterInfo))
		/* Failure to allocate memory. */
		return FALSE;

	/* Get the printer information. */
	if (!GetPrinter(hPrinter, 2, (LPSTR)pPrinterInfo, cByteNeeded, &cByteUsed)) {
		/* Failure to access the printer. */
		free(pPrinterInfo);
		pPrinterInfo = NULL;
		return FALSE;
	}

	/* Get job storage space. */
	if (!EnumJobs(hPrinter, 0, pPrinterInfo->cJobs, 2, NULL, 0, (LPDWORD)&cByteNeeded, (LPDWORD)&nReturned)) {
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			free(pPrinterInfo);
			pPrinterInfo = NULL;
			return FALSE;
		}
	}

	pJobStorage = (JOB_INFO_2 *)malloc(cByteNeeded);
	if (!pJobStorage) {
		/* Failure to allocate Job storage space. */
		free(pPrinterInfo);
		pPrinterInfo = NULL;
		return FALSE;
	}

	ZeroMemory(pJobStorage, cByteNeeded);

	/* Get the list of jobs. */
	if (!EnumJobs(hPrinter, 0, pPrinterInfo->cJobs, 2, (LPBYTE)pJobStorage, cByteNeeded, (LPDWORD)&cByteUsed, (LPDWORD)&nReturned)) {
		free(pPrinterInfo);
		free(pJobStorage);
		pJobStorage = NULL;
		pPrinterInfo = NULL;
		return FALSE;
	}

	/*
	 *  Return the information.
	 */
	*pcJobs = nReturned;
	*pStatus = pPrinterInfo->Status;
	*ppJobInfo = pJobStorage;
	free(pPrinterInfo);

	return TRUE;
}

/*
 * return Printer error and job error,
 * any error of printer or job return ture
 */
static BOOL IsPrinterError(HANDLE hPrinter, DWORD *dwPrinterStatus, DWORD *jobStatus, DWORD *cJobs)
{
	JOB_INFO_2	*pJobs;
	/*
	 *  Get the state information for the Printer Queue and
	 *  the jobs in the Printer Queue.
	 */
	if (!GetJobs(hPrinter, &pJobs, cJobs, dwPrinterStatus))
		return FALSE;

	/*
	 *  If the Printer reports an error, believe it.
	 */
	if (*dwPrinterStatus & (PRINTER_STATUS_ERROR |
			PRINTER_STATUS_PAPER_JAM |
			PRINTER_STATUS_PAPER_OUT |
			PRINTER_STATUS_PAPER_PROBLEM |
			PRINTER_STATUS_OUTPUT_BIN_FULL |
			PRINTER_STATUS_NOT_AVAILABLE |
			PRINTER_STATUS_NO_TONER |
			PRINTER_STATUS_OUT_OF_MEMORY |
			PRINTER_STATUS_OFFLINE |
			PRINTER_STATUS_DOOR_OPEN)) {
		goto error;
	}

	/*
	 *  Find the Job in the Queue that is printing.
	 */
	for (int i = 0; i < (int)*cJobs; i++) {
		if (pJobs[i].Status & JOB_STATUS_PRINTING) {
			/*
			*  If the job is in an error state,
			*  report an error for the printer.
			*  Code could be inserted here to
			*  attempt an interpretation of the
			*  pStatus member as well.
			*/
			if (pJobs[i].Status & (JOB_STATUS_ERROR |
					JOB_STATUS_OFFLINE |
					JOB_STATUS_PAPEROUT |
					JOB_STATUS_BLOCKED_DEVQ)) {
				goto error;
			}
		}
	}


	/*
	*  No error condition.
	*/
	free(pJobs);
	return FALSE;

error:
	/* save job error status to p_info */
	for (int i = 0; i < (int)*cJobs; i++) {	
		*(jobStatus + i) = pJobs[i].Status;
	}

	free(pJobs);
	return TRUE;
}


/* Send and receive of printing data */
static void datatrans_work(P_CBTD_INFO p_info)
{
	DWORD jobNums = 0;

	/* loop break when get error or no jobs */
	for (;;) {

		/* if job has any error, break out */
		if (IsPrinterError((HANDLE)(p_info->printer_handle), &(DWORD)p_info->prt_state, p_info->prt_job_status, &jobNums)) {	
			p_info->need_update = 1;
			set_sysflags(p_info, ST_PRT_ERROR);
			//break;
		}

		/* no job in panel, break out */
		if (jobNums == 0) {
			set_sysflags(p_info, ST_PRT_IDLE);
			break;
		}
	}
}

/*  end of thread */
static void datatrans_cleanup(void* data)
{
	P_CARGS p_cargs = (P_CARGS)data;
	int fd = *(p_cargs->p_max);

	//close(fd);
	if (!is_sysflags(p_cargs->p_info, ST_SYS_DOWN))
		set_sysflags(p_cargs->p_info, ST_SYS_DOWN);

	p_cargs->p_info->datatrans_thread_status = THREAD_DOWN;
	printf("Data transfer thread ...down\n");
	return;
}


/* Thread to send printing data */
void datatrans_thread(P_CBTD_INFO p_info)
{
	CARGS cargs;
	int set_flags, reset_flags;
	int p_max = 0;

	HANDLE hPrinter = NULL;

	if (!OpenPrinter(_T("EPSON R330 Series"), &hPrinter, NULL)) {
		int err = GetLastError();
		printf("unable to open %s, last error: %d\n", "EPSON R330 Series", err);
		return;
	}
	if(hPrinter)
		p_info->printer_handle = hPrinter;

	//_DEBUG_MESSAGE_VAL("datatrans thread ID : ", (int)pthread_self());
	_datatrans_lpr_flag = 0;

	cargs.p_info = p_info;
	cargs.p_max = &p_max;
	pthread_cleanup_push(datatrans_cleanup, (void *)&cargs);

	for (;;)
	{
		/* Is daemon in the middle of process for end ? */
		if (is_sysflags(p_info, ST_SYS_DOWN))
			break;

		p_info->datatrans_thread_status = THREAD_RUN;

		/* 
		 * todo: print job printing state may set when a task is create
		 * we need to wait a ST_JOB_PRINTING here. The state may set by
		 * comserv thread when a print job create by upsteam, send a sock
		 * command to me.
		 */
		set_sysflags(p_info, ST_JOB_PRINTING);

		set_flags = 0;
		reset_flags = ST_PRT_CONNECT | ST_SYS_DOWN | ST_JOB_CANCEL;

		wait_sysflags(p_info, set_flags, reset_flags, 0, WAIT_SYS_OR);

		if (is_sysflags(p_info, ST_PRT_CONNECT))
		{
			datatrans_work(p_info);	
		}

		//reset_sysflags(p_info, ST_JOB_RECV);
		
		reset_sysflags(p_info, ST_JOB_PRINTING);

		if (is_sysflags(p_info, ST_SYS_DOWN))
			break;
	}

	pthread_cleanup_pop(1);
	return;
}