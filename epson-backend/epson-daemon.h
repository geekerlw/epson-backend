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
#ifndef __EPSON_DAEMON_H__
#define __EPSON_DAEMON_H__

#ifndef _DEBUG
#define NOEBUG
#endif

#define CBTD_THREAD_STACK_SIZE 0x4000 /* unused */

#define PRT_STATUS_MAX 256	/* maximum size of printer status strings */
#define CONF_BUF_MAX 100	/* maximum size of configuration strings */
#define PID_PATH "/var/run/ecbd.pid"

typedef struct _CBTD_INFO
{
	char printer_name[CONF_BUF_MAX]; /* the printer title that is set with lpr */
	char devprt_path[CONF_BUF_MAX]; /* open device path */
	char infifo_path[CONF_BUF_MAX]; /* fifo path */
	unsigned int comsock_port; /* INET socket port */
	int devfd;		/* device file descriptor */

	char prt_status[PRT_STATUS_MAX]; /* printer status */
	int prt_status_len;	/* size of printer status strings */
	long status_post_time; /* the time that updated prt_status in the last */

	int sysflags;		/* CBTD status flags */
	void* sysflags_critical;

	/* Parent thread uses status of each thread for authentication.
	Not need to manage it with semaphore. */
	void* datatrans_thread;
	int datatrans_thread_status;
	void* comserv_thread;
	int comserv_thread_status;

	/* CBT control */
	void* ecbt_handle;
	void* ecbt_accsess_critical;
} CBTD_INFO, *P_CBTD_INFO;


/* Status of thread */
enum _CBTD_THREAD_STATUS_NUMBERS {
	THREAD_RUN = 0,
	THREAD_SLEEP,
	THREAD_DOWN
};


/* Status of a system */
enum _CBTD_SYSTEM_FLAGS {
	ST_SYS_DOWN = 1,	 /* 00000001  system is in the middle of end process */

	ST_DAEMON_WAKEUP = 2,    /* 00000010  system works */
	ST_PRT_CONNECT = 4,      /* 00000100  connected to a printer */

	ST_CLIENT_CONNECT = 8,   /* 00001000  presence of connection from a client */
	ST_JOB_PRINTING = 16,	 /* 00010000  print it */
	ST_JOB_CANCEL = 32,      /* 00100000  canceling print job */
	ST_JOB_CANCEL_NO_D4 = 64 /* 01000000  wait for reset of a printer */
};


void end_cbtd(P_CBTD_INFO);
int end_epson_cbt(P_CBTD_INFO);
void sig_set(void);
int parameter_setup(P_CBTD_INFO);
int parameter_update(P_CBTD_INFO);
void datatrans_thread(P_CBTD_INFO);
void comserv_thread(P_CBTD_INFO);

#endif /* __EPSON_DAEMON_H__ */
