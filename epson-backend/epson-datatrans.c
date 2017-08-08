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
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "epson-daemon.h"
#include "epson-wrapper.h"
#include "epson-thread.h"

#ifndef _CRT_NO_TIME_T
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#endif

#define F_OK 0 /* Test for existence. just for windows*/

#define DATA_BUF_SIZE 0x1000	/* size of data which can read at a time */
#define INPUT_TIMEOUT 60	/* timeout of input */

static int _datatrans_lpr_flag;

/* restart lpd */
static void lpd_start(P_CBTD_INFO p_info)
{
	if (_datatrans_lpr_flag)
	{
		char* com_buf;

		/* command + printer name + \0 */
		com_buf = malloc(10 + strlen(p_info->printer_name) + 1);

		/* printing start */
		strcpy(com_buf, "lpc start ");
		strcat(com_buf, p_info->printer_name);
		system(com_buf);

		free(com_buf);
		_datatrans_lpr_flag = 0;
	}

	return;
}

/* eliminate print job in process with lpd and turn a printer into
temporary halt */
static void lpd_stop(P_CBTD_INFO p_info)
{
/*
	/* In case of printer that a D4 command is not supported, wait
	till a printer reset it */
/*	if (is_sysflags(p_info, ST_JOB_CANCEL_NO_D4))
	{
		reset_sysflags(p_info, ST_JOB_PRINTING);
		wait_sysflags(p_info, ST_JOB_CANCEL, 0, 0, WAIT_SYS_AND);
	}
/* todo: windows can't write file*/
/*	if (!access("/var/run/lpr_lock", F_OK))
	{
		char* com_buf;

		/* command + printer name + \0 */
/*		com_buf = malloc(10 + strlen(p_info->printer_name) + 1);

		/* printing stop */
/*		strcpy(com_buf, "lpc stop ");
		strcat(com_buf, p_info->printer_name);
		system(com_buf);

		/* delete Job */
/*		strcpy(com_buf, "lprm -P");
		strcat(com_buf, p_info->printer_name);
		system(com_buf);

		free(com_buf);
		/* lpr stops */
/*		_datatrans_lpr_flag = 1;
	}

	return;
*/
}



/* open the named pipe */
static int fifo_open(P_CBTD_INFO p_info, char* path)
{
/*	int fd;
	int old_mask;

	if (access(path, F_OK) == 0)
	{
		if (remove(path))
		{
			return -1;
		}
	}

	old_mask = umask(0);

	if (mkfifo(path, 0666))
	{
		umask(old_mask);
		return -1;
	}

	umask(old_mask);

	if (is_sysflags(p_info, ST_JOB_CANCEL | ST_JOB_CANCEL_NO_D4))
	{
		wait_sysflags(p_info, ST_JOB_CANCEL | ST_JOB_CANCEL_NO_D4, 0, 0, WAIT_SYS_AND);
	}
	/* wait till there is input */
/*	lpd_start(p_info);
	fd = open(path, O_RDONLY);

	if (fd < 0)
	{
		return -1;
	}
	return fd;
*/
	return 0;
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
	_DEBUG_MESSAGE("Data transfer thread ...down");
	return;
}

/* Send and receive of printing data */
static void datatrans_work(P_CBTD_INFO p_info, int fd)
{
	char *data_buf;
	int read_size, write_size;
	int i;

	data_buf = calloc(DATA_BUF_SIZE, sizeof(char));

	/* in this moment, system can communicate with a printer */
	for (;;)
	{
		read_size = 0;
		/* receive printing data */
		for (i = 0; i < INPUT_TIMEOUT && read_size <= 0; i++)
		{
			if (is_sysflags(p_info, ST_JOB_CANCEL)
				|| !is_sysflags(p_info, ST_PRT_CONNECT)
				|| is_sysflags(p_info, ST_SYS_DOWN))
				goto CANCEL;

			//read_size = read(fd, data_buf, DATA_BUF_SIZE);
			read_size = 0;


			if (read_size <= 0)
				Sleep(1);
		}

		if (read_size <= 0)
		{
			free(data_buf);
			_DEBUG_MESSAGE("Job end");
			return;
		}

		/* send printing data to a printer */
		write_size = read_size;
		do
		{
			int tmp_size;

			if (is_sysflags(p_info, ST_JOB_CANCEL)
				|| !is_sysflags(p_info, ST_PRT_CONNECT))
				goto CANCEL;

			enter_critical(p_info->ecbt_accsess_critical);

			tmp_size = write_size;
			write_to_prt(p_info, SID_DATA,
				data_buf + (read_size - write_size),
				&write_size);
			write_size = tmp_size - write_size;

			leave_critical(p_info->ecbt_accsess_critical);

			Sleep(10000);
		} while (write_size > 0);
	}

	/* printing was canceled */
CANCEL:
	free(data_buf);

	set_sysflags(p_info, ST_JOB_CANCEL);
	lpd_stop(p_info);

	_DEBUG_MESSAGE_VAL("Job cancel : system status =", p_info->sysflags);
	return;
}

/* Thread to send printing data */
void datatrans_thread(P_CBTD_INFO p_info)
{
	int fifo_fd;
	char* fifo_path = p_info->infifo_path;
	CARGS cargs;
	//int set_flags, reset_flags;

	//_DEBUG_MESSAGE_VAL("datatrans thread ID : ", (int)pthread_self());
	_datatrans_lpr_flag = 0;

	cargs.p_info = p_info;
	cargs.p_max = &fifo_fd;
	pthread_cleanup_push(datatrans_cleanup, (void *)&cargs);

	for (;;)
	{
		fifo_fd = fifo_open(p_info, fifo_path);
		if (fifo_fd < 0)
		{
			perror(fifo_path);
			break;
		}

		/* Is daemon in the middle of process for end ? */
		if (is_sysflags(p_info, ST_SYS_DOWN))
			break;

		p_info->datatrans_thread_status = THREAD_RUN;
		//set_sysflags(p_info, ST_JOB_PRINTING);

		//set_flags = 0;
		//reset_flags = ST_PRT_CONNECT | ST_SYS_DOWN | ST_JOB_CANCEL;

		//wait_sysflags(p_info, set_flags, reset_flags, 0, WAIT_SYS_AND);

		//if (is_sysflags(p_info, ST_PRT_CONNECT))
		//{
			/* send and receive of printing data */
		/*	open_port_channel(p_info, SID_DATA);
			datatrans_work(p_info, fifo_fd);
			close_port_channel(p_info, SID_DATA);
			*/
		//}
		//reset_sysflags(p_info, ST_JOB_PRINTING);

		//if (fifo_fd)
			//close(fifo_fd);

		if (is_sysflags(p_info, ST_SYS_DOWN))
			break;
	}

	pthread_cleanup_pop(1);
	return;
}