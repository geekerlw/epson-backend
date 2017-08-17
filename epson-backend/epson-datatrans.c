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
#include "epson-wrapper.h"
#include "epson-thread.h"


static int _datatrans_lpr_flag;

#define REP_BUF_SIZE  256	/* maximum size of reply buffer */

typedef enum {
	STBIN_ST = 0x01,
	STBIN_ER = 0x02,
	STBIN_WR = 0x04,	/* 2004.03.25 add for setting warning */
	STBIN_CS = 0x0a,
	STBIN_IC = 0x0f,
	STBIN_PC = 0x36
} StField;

#define HIDE_INKINFO (0x01 << 0)
#define HIDE_INKLOW  (0x01 << 1)

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

/* transmit a command to a printer and receive reply */
int write_prt_command(P_CBTD_INFO p_info, char* com_buf,
	int com_size, char* rep_buf, int* rep_size)
{

	int read_possible_size = *rep_size;
	int read_size;
	int err = 0;

	if (!p_info || rep_buf == NULL || rep_size == 0)
		return 0;

	*rep_size = 0;
	enter_critical(p_info->ecbt_accsess_critical);
	err = write_to_prt(p_info, SID_CTRL, com_buf, &com_size);

	usleep(100000);


	if (com_size)
	{

		/* get size of reply */
		read_from_prt(p_info, SID_CTRL, NULL, &read_size);

		if (read_size)
		{

			/* receive reply */
			char tmp_buf[REP_BUF_SIZE];

			assert(read_possible_size >= read_size);
			*rep_size = read_size;
			do
			{
				int tmp_size = read_size;

				err = read_from_prt(p_info, SID_CTRL, tmp_buf, &read_size);
				if (read_size)
					memcpy(rep_buf + (*rep_size - tmp_size), tmp_buf, read_size);
				read_size = tmp_size - read_size;
			} while (err == 0 && read_size > 0);
		}
	}
	leave_critical(p_info->ecbt_accsess_critical);

	if (*rep_size <= 0)
		return 1;

	return err;
}


/* clear data left with a printer */
static void clear_replay_buffer(P_CBTD_INFO p_info)
{
	char tmp_buf[REP_BUF_SIZE];
	int read_size;
	int err = 0;

	enter_critical(p_info->ecbt_accsess_critical);
	for (;;)
	{
		err = read_from_prt(p_info, SID_CTRL, NULL, &read_size);

		if (err != 0 || read_size <= 0)
			break;

		assert(REP_BUF_SIZE >= read_size);

		do
		{
			int tmp_size = read_size;

			err = read_from_prt(p_info, SID_CTRL, tmp_buf, &read_size);
			read_size = tmp_size - read_size;
		} while (err == 0 && read_size > 0);
	}
	leave_critical(p_info->ecbt_accsess_critical);

	return;
}


static unsigned char* scan_bin_status(StField field, unsigned char *bdata, size_t bsize)
{
	unsigned char bheader[] = { '@', 'B', 'D', 'C', ' ', 'S', 'T', '2', 0x0d, 0x0a };
	unsigned char *point;
	size_t count;

	if (!bdata) {
		return NULL;
	}

	count = sizeof(bheader);
	point = NULL;
	while (count < bsize) {
		if (bdata[count] == (unsigned char)field) {
			point = bdata + count;
			break;
		}
		count += 2 + bdata[count + 1];
	}

	return point;
}

static size_t change_status_format(P_CBTD_INFO p_info, unsigned char *bdata, size_t bsize,
	char *cdata, size_t csize)
{
	unsigned char bheader[] = { '@', 'B', 'D', 'C', ' ', 'S', 'T', '2', 0x0d, 0x0a };
	char cheader[] = { '@', 'B', 'D', 'C', ' ', 'S', 'T', 0x0d, 0x0a };
	unsigned char *point;
	size_t count = 0;
	int nparam;

	/* ヘッダチェック */
	if (!bdata || memcmp(bdata, bheader, sizeof(bheader))) {
		return 0;
	}

	if (count + sizeof(cheader) < csize) {
		memcpy(cdata + count, cheader, sizeof(cheader));
	}

	/* Status Codes */
	point = scan_bin_status(STBIN_ST, bdata, bsize);

	if (point && count + 6 < csize) {
		/* 2004.03.02 update "%2x" -> "%2X" */
		sprintf(cdata + count, "ST:%02X;", (int)point[2]);
		count += 6;
	}

	/* Error Codes */
	point = scan_bin_status(STBIN_ER, bdata, bsize);
	if (point && count + 6 < csize) {
		/* 2004.03.02 update "%2x" -> "%2X" */
		sprintf(cdata + count, "ER:%02X;", (int)point[2]);
		count += 6;
	}

	/* Quantity of ink */
	point = scan_bin_status(STBIN_IC, bdata, bsize);
	if (point) {
		int i, n;

		nparam = point[2];
		n = point[1] / nparam;

		if (count + (3 + 2 * n + 1) < csize) {
			sprintf(cdata + count, "IQ:");
			count += 3;
			point += 3;

			for (i = 0; i < n; i++) {
				/* 2004.03.02 update "%2x" -> "%2X" */
				sprintf(cdata + count, "%02X", (int)point[i * nparam + 2]);
				count += 2;
			}
			sprintf(cdata + count, ";");
			count += 1;
		}
	}

#if 1
	/* Characteristic status code */
	point = scan_bin_status(STBIN_CS, bdata, bsize);
	if (point) {
		EPS_UINT8   ParameterByte = point[1];
		if (ParameterByte == 2) {
			if (point[2] == 0x10 /*struct ver.*/) {
				EPS_BOOL showInkInfo = (point[3] & HIDE_INKINFO) ? FALSE : TRUE;
				EPS_BOOL showInkLow = (point[3] & HIDE_INKLOW) ? FALSE : TRUE;

				if (point && count + 6 < csize) {
					sprintf(cdata + count, "CS:%X%X;", showInkInfo, showInkLow);
					count += 6;
				}

			}
		}

	}
#endif

	/* Warning Codes */
	point = scan_bin_status(STBIN_WR, bdata, bsize);
	if (point) {
		int i, n;
		n = point[1]; 	/* no. of warning */
		for (i = 0; i < n; i++) {
			/* check only ink low */
			if (point[i + 2] >= 0x10 && point[i + 2] <= 0x1A) {
				sprintf(cdata + count, "WR:%02X;", (int)point[i + 2]);
				count += 6;
				/* not set all warning, so break */
				break;
			}
		}
	}

	point = scan_bin_status(STBIN_IC, bdata, bsize);
	if (point) {
		int i, n;

		/* 2004.06.24 recommend to use point[2] instead of 3 */
		nparam = point[2];
		n = point[1] / nparam;
		if (count + (4 + 4 * n + 1) < csize) {
			sprintf(cdata + count, "INK:");
			count += 4;
			point += 3;

			for (i = 0; i < n; i++) {
				char key[5];

				key[0] = '\0';

				switch (point[i * nparam + 1]) {
				case 0x00:
					if (0x0b == point[i * nparam]) {
						strncpy(key, "B140", 4);
					}
					else if (6 <= n) {
						/* printer has 6 color ink or more. */
						strncpy(key, "A140", 4);
					}
					else {
						/* 4 color ink printer */
						strncpy(key, "1101", 4);
					}
					break;
				case 0x01:
					strncpy(key, "3202", 4);
					break;
				case 0x02:
					strncpy(key, "4304", 4);
					break;
				case 0x03:
					strncpy(key, "5408", 4);
					break;
				case 0x04:
					strncpy(key, "6210", 4);
					break;
				case 0x05:
					strncpy(key, "7320", 4);
					break;

				case 0x07:
					strncpy(key, "9440", 4);
					break;
				case 0x08:
					strncpy(key, "C140", 4);
					break;
				case 0x09:
					/* for PX-G900 -- New Ink i	2004.01.20	*/
					/* Red		*/
					strncpy(key, "9020", 4);
					break;
				case 0x0A:
					/* for PX-G900 -- New Ink i	2004.01.20	*/
					/* Viloet	*/
					strncpy(key, "A040", 4);
					break;
				case 0x0B:
					/* for PX-G900 -- New Ink i	2004.01.20	*/
					/* Clear	*/
					strncpy(key, "B080", 4);
					break;
				case 0x0C:
					strncpy(key, "C040", 4);
					break;
				case 0x0D:
					strncpy(key, "D010", 4);
					break;
				default:
					/* UNKNOWN COLOR */
					strncpy(key, "0000", 4);
					break;
				}

				key[4] = '\0';
				sprintf(cdata + count, "%s", key);
				count += 4;
			}
			sprintf(cdata + count, ";");
			count += 1;
		}
	}


	/* Printing Count */
	point = scan_bin_status(STBIN_PC, bdata, bsize);

	if (point) {
		int i, n;
		//		int cc=0;  /* no. of color printed */
		//		int mc=0;  /* no. of monochrome printed */
		//		int wc=0;  /* no. of blank page */
		//		int ac=0;  /* no. of ADF scanned */
		//		int cb=0;  /* no. of color borderless printed */ 
		//		int mb=0;  /* no. of monochrome borderless printed */

		n = point[1]; 	/* size of parameter (14h / 18h / 20h */

		if (n == 0x14 || n == 0x18 || n == 0x20) {

			sprintf(cdata + count, "CC:%02X%02X%02X%02X;", (int)point[10], (int)point[11], (int)point[12], (int)point[13]);
			count += 12;
			sprintf(cdata + count, "MC:%02X%02X%02X%02X;", (int)point[14], (int)point[15], (int)point[16], (int)point[17]);
			count += 12;
			sprintf(cdata + count, "WC:%02X%02X%02X%02X;", (int)point[18], (int)point[19], (int)point[20], (int)point[21]);
			count += 12;

			if (n == 0x18) {
				sprintf(cdata + count, "AC:%02X%02X%02X%02X;", (int)point[22], (int)point[23], (int)point[24], (int)point[25]);
				count += 12;

			}
			else if (n == 0x20) {
				sprintf(cdata + count, "AC:%02X%02X%02X%02X;", (int)point[22], (int)point[23], (int)point[24], (int)point[25]);
				count += 12;
				sprintf(cdata + count, "CB:%02X%02X%02X%02X;", (int)point[26], (int)point[27], (int)point[28], (int)point[29]);
				count += 12;
				sprintf(cdata + count, "MB:%02X%02X%02X%02X;", (int)point[30], (int)point[31], (int)point[32], (int)point[33]);
				count += 12;
			}


			for (i = 0; i < n; i++) {
				/* check only ink low */
				if (point[i + 2] >= 0x10 && point[i + 2] <= 0x19) {
					sprintf(cdata + count, "WR:%02X;", (int)point[i + 2]);
					count += 6;
					/* not set all warning, so break */
					break;
				}
			}
		}
	}



	*(cdata + count) = '\0';
	count++;
	return count;
}

/* get status of a printer */
int post_prt_status(P_CBTD_INFO p_info)
{
	char get_status_command[] = { 's', 't', 0x01, 0x00, 0x01 };
	char timer_set_command[] = { 't', 'i', 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	char rbuf[PRT_STATUS_MAX];
	int csize;
	int rsize = PRT_STATUS_MAX;
	long now_time;
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0)
	{
		perror("gettimeofday");
	}

	now_time = tv.tv_sec;

	//	if (p_info->status_post_time == 0
	//	    || (now_time - p_info->status_post_time) > STATUS_POST_TIME)
	{
		char* point;

		csize = sizeof(get_status_command);
		if (write_prt_command(p_info, get_status_command, csize, rbuf, &rsize))
		{
			memset(p_info->prt_status, 0, PRT_STATUS_MAX);
			p_info->prt_status_len = 0;
			return 1;
		}

		/* ステータス情報変換 */
		if (rbuf[7] == '2') {
			p_info->prt_status_len = change_status_format(p_info, (unsigned char *)rbuf, rsize,
				p_info->prt_status,
				PRT_STATUS_MAX);
		}
		else {
			memcpy(p_info->prt_status, rbuf, rsize);
			p_info->prt_status_len = rsize;
		}


		point = strstr(p_info->prt_status, "ST:");
		if (point == NULL)
		{		/* retry */
			usleep(100000);
			clear_replay_buffer(p_info);
			return post_prt_status(p_info);
		}

		/* cancel of a printer was completed */
		if (is_sysflags(p_info, ST_JOB_CANCEL)
			&& !is_sysflags(p_info, ST_JOB_PRINTING))
		{
			if (point[4] == '4')
			{
				reset_sysflags(p_info, ST_JOB_CANCEL);
			}
		}

#if 0
		/* wait for re-starting of a printer */
		if (is_sysflags(p_info, ST_JOB_CANCEL_NO_D4))
		{
			char* epoint;
			epoint = strstr(p_info->prt_status, "ER:");

			if (point[4] == '1' || point[4] == '2'
				|| point[4] == '3' || point[4] == '7')
			{
				memcpy(point + 3, "CN", 2); /* overwrite */
			}
			else if (epoint != NULL)
			{
				if (!strncmp(epoint + 3, "04", 2)
					|| !strncmp(epoint + 3, "06", 2))
					memcpy(point + 3, "CN", 2); /* overwrite */
			}

		}
#endif /* if 0 */
		p_info->status_post_time = now_time;
#if 0      
		/* set a clock */
		if (_comserv_first_flag)
		{
			if (point[4] == '4' || point[4] == '0') {
				/* statusが正常なときに設定するように修正      */
				/* statusがERRORでも可能                       */
				/* for PM-980C				       */
				time_t tp;
				struct tm *ltime;

				tp = time(&tp);
				ltime = localtime(&tp);
				timer_set_command[5] = (1900 + ltime->tm_year) >> 8;
				timer_set_command[6] = (1900 + ltime->tm_year) & 0xff;
				timer_set_command[7] = ltime->tm_mon + 1;
				timer_set_command[8] = ltime->tm_mday;
				timer_set_command[9] = ltime->tm_hour;
				timer_set_command[10] = ltime->tm_min;
				timer_set_command[11] = ltime->tm_sec;
				csize = sizeof(timer_set_command);
				rsize = PRT_STATUS_MAX;
				write_prt_command(p_info, timer_set_command, csize, rbuf, &rsize);
				_comserv_first_flag = 0;
			}
		}
#endif
	}

	return 0;
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