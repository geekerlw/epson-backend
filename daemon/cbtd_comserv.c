/*
 * Copyright (C) Seiko Epson Corporation 2014.
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "cbtd.h"
#include "libcbt.h"
#include "cbtd_thread.h"
#include "epson-typdefs.h"
#include "daemon_osdef.h"

#define SOCK_ACCSESS_WAIT_MAX 20
/* command packet key word */
static const char COMMAND_PACKET_KEY[] = "pcp";
/* length of packet key word */
#define PACKET_KEY_LEN 3

#define COM_BUF_SIZE  256	/* size of command buffer */
#define PAC_HEADER_SIZE 5	/* size of packet header */
#define REP_BUF_SIZE  256	/* maximum size of reply buffer */

#define STATUS_POST_TIME 1 /* number of seconds which updates status */

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x40
#endif

enum _ERROR_PACKET_NUMBERS {
	ERRPKT_NOREPLY = 0,	/* no error (nothing reply) */
	ERRPKT_UNKNOWN_PACKET,	/* receive indistinct packet */
	ERRPKT_PRINTER_NO_CONNECT, /* cannot communicate with a printer */
	ERRPKT_MEMORY		/* memory shortage */
};

typedef enum {
	STBIN_ST = 0x01,
	STBIN_ER = 0x02,
	STBIN_WR = 0x04,	/* 2004.03.25 add for setting warning */ 
	STBIN_CS = 0x0a,
	STBIN_IC = 0x0f,
	STBIN_PC = 0x36
} StField;

static int comserv_work (P_CBTD_INFO, int);
static int command_recv (int, char*, int*);
static void reply_send (int, char*, int);
static int default_recept (P_CBTD_INFO, int, char*, int);
static int status_recept (P_CBTD_INFO, int);
static int post_prt_status (P_CBTD_INFO);
static int cancel_recept (P_CBTD_INFO, int);
static int nozzlecheck_recept (P_CBTD_INFO, int);
static int getdeviceid_recept (P_CBTD_INFO, int);
static int headcleaning_recept (P_CBTD_INFO, int);
static int cancel_nd4_recept (P_CBTD_INFO, int);
static int error_recept (int, int);
static int write_prt_command (P_CBTD_INFO, char*, int, char*, int*);

static int servsock_open (int);
static int sock_read (int, char*, int);
static int sock_write (int, char*, int);
static void comserv_cleanup (void*);
static void clear_replay_buffer (P_CBTD_INFO);

static size_t change_status_format(P_CBTD_INFO, unsigned char *, size_t, char *, size_t);
static unsigned char * scan_bin_status(StField, unsigned char *, size_t);

static int SendCommand(char *pCmd, int nCmdSize);

/* When flag stands, get status from a printer without qualification */
static int _comserv_first_flag;


/* communication thread */
void
comserv_thread (P_CBTD_INFO p_info)
{
	int server_fd, client_fd;
	struct sockaddr_in client_addr;
	fd_set sock_fds;
	struct timeval tv;
	int maxval, nclient;
	CARGS cargs;

	_DEBUG_MESSAGE_VAL("Comserv thread ID : ", (int)pthread_self());
	_comserv_first_flag = 1;

	FD_ZERO (&sock_fds);
	maxval = nclient = 0;

	cargs.p_info = p_info;
	cargs.p_fds = &sock_fds;
	cargs.p_max = &maxval;
	pthread_cleanup_push (comserv_cleanup, (void *)&cargs);

	server_fd = servsock_open (p_info->comsock_port);
	if (server_fd < 0)
	{
		char sock_num[10];

		sprintf (sock_num, "%d", p_info->comsock_port);
		p_info->comserv_thread_status = THREAD_DOWN;
		perror (sock_num);
		return;
	}

	FD_SET (server_fd, &sock_fds);
	maxval = server_fd;

	for (;;)
	{
		int fd;
		int addr_len;
		fd_set watch_fds = sock_fds;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if (select (maxval + 1, &watch_fds, NULL, NULL, &tv) < 0)
		{
			_DEBUG_FUNC(perror ("cs select ()"));
			continue;
		}

		/* Is daemon in the middle of process for end ? */
		if (is_sysflags (p_info, ST_SYS_DOWN))
			break;

		/* Is a printer connected ? */
		if (!is_sysflags (p_info, ST_PRT_CONNECT))
		{
			_comserv_first_flag = 1;
		}
		else
		{
			if (post_prt_status (p_info))
				reset_sysflags (p_info, ST_PRT_CONNECT);
		}

		for (fd = 0; fd < maxval + 1; fd++)
		{
			if ( FD_ISSET(fd, &watch_fds))
			{
				if (fd == server_fd)
				{
					/* connecting */
					client_fd = accept (server_fd,
							    (struct sockaddr *)&client_addr,
							    (socklen_t *)&addr_len);
					if (client_fd < 0)
						continue;

					_DEBUG_MESSAGE_VAL("connect client fd = ", client_fd);
					FD_SET (client_fd, &sock_fds);

					if (maxval < client_fd)
						maxval = client_fd;

					nclient ++;
					set_sysflags (p_info, ST_CLIENT_CONNECT);
					if (!is_sysflags (p_info, ST_PRT_CONNECT))
						sleep (1);
				}

				else
				{
					int nread = 0;
		  
					ioctl (fd, FIONREAD, &nread);
					if (nread == 0)
					{
						/* disconnecting */
						_DEBUG_MESSAGE_VAL("deconnect client fd = ", fd);
						shutdown (fd, 2);
						FD_CLR (fd, &sock_fds);
						close(fd);

						nclient--;
						if (nclient == 0)
							reset_sysflags (p_info, ST_CLIENT_CONNECT);
					}
					else
					{
						/* receive a message */
						if (comserv_work (p_info, fd))
							reset_sysflags (p_info, ST_PRT_CONNECT);
					}
				}
			}
		}
	}
  
	pthread_cleanup_pop (1);
	return;
}


/* handle the data which received */
static int
comserv_work (P_CBTD_INFO p_info, int fd)
{
	const char status_command[] = { 's', 't', 0x01, 0x00, 0x01 };
	const char cancel_command[] = { 'c', 'a', 'n', 'c', 'e', 'l' };
	const char cancel_nd4_command[] = { 'c', 'a', 'n', 'c', 'e', 'l', '_', 'n', 'd', '4' };
	const char nozzlecheck_command[] = { 'n', 'o', 'z', 'z', 'l', 'e', 'c', 'h', 'e', 'c', 'k' };
	const char headcleaning_command[] = { 'h', 'e', 'a', 'd', 'c', 'l', 'e', 'a', 'n', 'i', 'n', 'g' };
	const char getdeviceid_command[] = { 'g', 'e', 't', 'd', 'e', 'v', 'i', 'c', 'e', 'i', 'd'};
	char cbuf[COM_BUF_SIZE];
	unsigned int csize, err = 0;

	assert (p_info);

	/* wait till it is connected to a printer */
	if (!is_sysflags (p_info, ST_PRT_CONNECT))
		return error_recept (fd, ERRPKT_PRINTER_NO_CONNECT);

	if (command_recv (fd, cbuf, &csize))
	{
		/* received indistinct packet */
		return error_recept (fd, ERRPKT_UNKNOWN_PACKET);
	}


	/* acquire status */
	if (memcmp (cbuf, status_command, sizeof (status_command)) == 0)
	{
		if (status_recept (p_info, fd))
			err = 1;
	}

	/* stop printing (not D4) */
	else if (memcmp (cbuf, cancel_nd4_command, sizeof (cancel_nd4_command)) == 0)
	{
		if (cancel_nd4_recept (p_info, fd))
			err = 1;
	}

	/* stop printing */
	else if (memcmp (cbuf, cancel_command, sizeof (cancel_command)) == 0)
	{
		if (cancel_recept (p_info, fd))
			err = 1;
	}

	/* nozzle check */
	else if (memcmp (cbuf, nozzlecheck_command, sizeof (nozzlecheck_command)) == 0)
	{
		if (nozzlecheck_recept (p_info, fd))
			err = 1;
	}

	/* head cleaning */
	else if (memcmp (cbuf, headcleaning_command, sizeof (headcleaning_command)) == 0)
	{
		if (headcleaning_recept (p_info, fd))
			err = 1;
	}

	/* nozzle check */
	else if (memcmp (cbuf, getdeviceid_command, sizeof (getdeviceid_command)) == 0)
	{
		if (getdeviceid_recept (p_info, fd))
			err = 1;
	}

	/* others */
	else
	{

		if (default_recept (p_info, fd, cbuf, csize))
			err = 1;
	}
  
	if (err)
	{
		error_recept (fd, ERRPKT_PRINTER_NO_CONNECT);
		return 1;
	}
	return 0;
}


/* take out data division minute from packet */ 
static int
command_recv (int fd, char* cbuf, int* csize)
{

	if (sock_read (fd, cbuf, PAC_HEADER_SIZE))
		return 1;

	if (strncmp (cbuf, COMMAND_PACKET_KEY, PACKET_KEY_LEN))
		return 1;
  
	*csize = ((int)cbuf[3] << 8) + (int)cbuf[4];

	if (sock_read (fd, cbuf, *csize))
		return 1;
	return 0;
}


/* transmit packet */
static void
reply_send (int fd, char* buf, int size)
{
	char packet[REP_BUF_SIZE + PAC_HEADER_SIZE];
	char header[] = { 'p', 'c', 'p', 0x00, 0x00 };

	header[3] = (char)(size >> 8);
	header[4] = (char)(size & 0xff);
  
	memcpy (packet, header, PAC_HEADER_SIZE);
	memcpy (packet + PAC_HEADER_SIZE, buf, size);
	sock_write (fd, packet, size + PAC_HEADER_SIZE);
	return;
}


/* usual command reception */
static int
default_recept (P_CBTD_INFO p_info, int fd, char* cbuf, int csize)
{
	char rbuf[REP_BUF_SIZE];
	int rsize;

	rsize = REP_BUF_SIZE;
	if (write_prt_command (p_info, cbuf, csize,
			       rbuf, &rsize))
	{
		return 1;
	}


	reply_send (fd, rbuf, rsize);
	return 0;
}


/* receive a status acquisition command */
static int
status_recept (P_CBTD_INFO p_info, int fd)
{
	if (p_info->prt_status_len == 0)
		return 1;

	reply_send (fd, p_info->prt_status, p_info->prt_status_len);
	return 0;
}


/* get status of a printer */
static int
post_prt_status (P_CBTD_INFO p_info)
{
	char get_status_command[] = { 's', 't', 0x01, 0x00, 0x01 };
	char timer_set_command[] = { 't', 'i', 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	char rbuf[PRT_STATUS_MAX];  
	int csize;
	int rsize = PRT_STATUS_MAX;
	long now_time;
	struct timeval tv;

	if (gettimeofday (&tv, NULL) < 0)
	{
		_DEBUG_FUNC(perror ("gettimeofday"));
	}

	now_time = tv.tv_sec;
  
//	if (p_info->status_post_time == 0
//	    || (now_time - p_info->status_post_time) > STATUS_POST_TIME)
	{
		char* point;

		csize = sizeof (get_status_command);
		if (write_prt_command (p_info, get_status_command, csize, rbuf, &rsize))
		{
			memset (p_info->prt_status, 0, PRT_STATUS_MAX);
			p_info->prt_status_len = 0;
			return 1;
		}

		/* ステータス情報変換 */
		if (rbuf[7] == '2') {
			p_info->prt_status_len = change_status_format(p_info,(unsigned char *) rbuf, rsize,
								      p_info->prt_status,
								      PRT_STATUS_MAX);
		} else {
			memcpy (p_info->prt_status, rbuf, rsize);
			p_info->prt_status_len = rsize;
		}


		point = strstr (p_info->prt_status, "ST:");
		if (point == NULL)
		{		/* retry */
			usleep (100000);
			clear_replay_buffer (p_info);
			return post_prt_status (p_info);
		}

		/* cancel of a printer was completed */
		if (is_sysflags (p_info, ST_JOB_CANCEL)
		    && !is_sysflags (p_info, ST_JOB_PRINTING))
		{
			if (point[4] == '4')
			{
				reset_sysflags (p_info, ST_JOB_CANCEL);
				reset_sysflags (p_info, ST_JOB_CANCEL_NO_D4);
			}
		}

		/* wait for re-starting of a printer */
		if (is_sysflags (p_info, ST_JOB_CANCEL_NO_D4))
		{
			char* epoint;
			epoint = strstr (p_info->prt_status, "ER:");

			if (point[4] == '1' || point[4] == '2'
			    || point[4] == '3' || point[4] == '7')
			{
				memcpy (point + 3, "CN", 2); /* overwrite */
			}
			else if (epoint != NULL)
			{
				if (!strncmp (epoint + 3, "04", 2)
				    || !strncmp (epoint + 3, "06", 2))
					memcpy (point + 3, "CN", 2); /* overwrite */
			}
	  
		}

		p_info->status_post_time = now_time;
#if 0      
		/* set a clock */
		if (_comserv_first_flag)
		{
			if( point[4] == '4' || point[4] == '0' ){
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
				csize = sizeof (timer_set_command);
				rsize = PRT_STATUS_MAX;
				write_prt_command (p_info, timer_set_command, csize, rbuf, &rsize);
				_comserv_first_flag = 0;
			}
		}
#endif
	}

	return 0;
}


/* received a cancel command */
static int
cancel_recept (P_CBTD_INFO p_info, int fd)
{
/* In case of CL-700, need to transmit a "mo" command */
#ifdef USE_MOTOROFF
	char motoroff_command[] = { 'm', 'o', 0x01, 0x00, 0x01 };
#endif
	char reset_command[] = { 'r', 's', 0x01, 0x00, 0x01 };
	char rbuf[REP_BUF_SIZE];
	int rsize = REP_BUF_SIZE;
	char* point;
	int i;

	set_sysflags (p_info, ST_JOB_CANCEL);
	for (i = 0; i < 100; i++)
	{
		if (is_sysflags (p_info, ST_JOB_PRINTING))
			usleep (10000);
	}

	sleep (1);

	point = strstr (p_info->prt_status, "ER:");
	if (point && (memcmp (point, "ER:04", 5) == 0)) /* paper jam ? */
	{
		write_prt_command (p_info, reset_command,
				   sizeof (reset_command), rbuf, &rsize);
		rsize = REP_BUF_SIZE;

#ifdef USE_MOTOROFF /* "mo" command */
		write_prt_command (p_info, motoroff_command,
				   sizeof (motoroff_command), rbuf, &rsize);
#endif
	}
	else
	{
#ifdef USE_MOTOROFF /* "mo" command */
		write_prt_command (p_info, motoroff_command,
				   sizeof (motoroff_command), rbuf, &rsize);
#endif
		rsize = REP_BUF_SIZE;
		write_prt_command (p_info, reset_command,
				   sizeof (reset_command), rbuf, &rsize);
	}

//	Cancel CUPS Jobs
	system ("cancel -a");

	error_recept (fd, ERRPKT_NOREPLY);  

	return 0;
}


/* received a cancel command (not D4) */
static int
cancel_nd4_recept (P_CBTD_INFO p_info, int fd)
{
	set_sysflags (p_info, ST_JOB_CANCEL_NO_D4);
	set_sysflags (p_info, ST_JOB_CANCEL);
	error_recept (fd, ERRPKT_NOREPLY);
	return 0;
}

/* received a nozzle check command */
static int
nozzlecheck_recept (P_CBTD_INFO p_info, int fd)
{

	char buffer[256];
	int bufsize;	
	epsMakeMainteCmd(EPS_MNT_NOZZLE, buffer, &bufsize);

	SendCommand (buffer, bufsize);

	error_recept (fd, ERRPKT_NOREPLY);  

	return 0;
}

/* received a head cleaning command */
static int
headcleaning_recept (P_CBTD_INFO p_info, int fd)
{

	char buffer[256];
	int bufsize;	
	epsMakeMainteCmd(EPS_MNT_CLEANING, buffer, &bufsize);

	SendCommand (buffer, bufsize);

	error_recept (fd, ERRPKT_NOREPLY);  

	return 0;
}

/* received a get device_id command */
static int
getdeviceid_recept (P_CBTD_INFO p_info, int fd)
{
	char device_id[256];
	get_device_id (device_id);

	reply_send (fd, device_id, strlen(device_id));

	return 0;
}

/* transmit error packet */
static int
error_recept (int fd, int err_code)
{
	char err_packet[] = { 'p', 'c', 'p', 0x00, 0x00, 0x00 };
	char buf[100];

	err_packet[5] = (char)err_code;

	sock_read (fd, buf, sizeof (buf));
	sock_write (fd, err_packet, sizeof (err_packet));
	return 0;
}


/* transmit a command to a printer and receive reply */
static int
write_prt_command (P_CBTD_INFO p_info, char* com_buf,
		   int com_size, char* rep_buf, int* rep_size)
{

	int read_possible_size = *rep_size;
	int read_size;
	int err = 0;
  
	if (!p_info || rep_buf == NULL || rep_size == 0)
		return 0;
  
	*rep_size = 0;
	enter_critical (p_info->ecbt_accsess_critical);
	err=	write_to_prt (p_info, SID_CTRL, com_buf, &com_size);

	usleep (100000);


	if (com_size)
	{

		/* get size of reply */
		read_from_prt (p_info, SID_CTRL, NULL, &read_size);
 
		if (read_size)
		{

			/* receive reply */
			char tmp_buf[REP_BUF_SIZE];

			assert (read_possible_size >= read_size);
			*rep_size = read_size;
			do
			{
				int tmp_size = read_size;
	      
				err = read_from_prt (p_info, SID_CTRL, tmp_buf, &read_size);
				if (read_size)
					memcpy (rep_buf + (*rep_size - tmp_size), tmp_buf, read_size);
				read_size = tmp_size - read_size;
			}
			while (err == 0 && read_size > 0);
		}
	}
	leave_critical (p_info->ecbt_accsess_critical);

	if (*rep_size <= 0)
		return 1;

	return err;
}


/* open a socket */
static int
servsock_open (int port)
{
  
	int fd;
	int opt = 1;

	struct sockaddr_in addr;

	fd = socket (AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (INADDR_ANY);
	addr.sin_port = htons (port);
	bind (fd, (struct sockaddr *)&addr, sizeof (addr));
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int));
	listen (fd, 5);

	return fd;
}


/* read it from a socket */
static int
sock_read (int fd, char* buf, int read_size)
{
	int i;
	int size;

	for (i = 0; i < SOCK_ACCSESS_WAIT_MAX; i++)
	{
		size = recv (fd, buf, read_size, MSG_NOSIGNAL | MSG_DONTWAIT);
		if (size == read_size)
		{
			return 0;
		}
		else if (size > 0)
		{
			read_size -= size;
			buf += size;
			usleep (1000);
		}
		else
			break;
	}
	return 1;
}


/* write it to a socket */
static int
sock_write (int fd, char* buf, int write_size)
{
	int i;
	int size;

	if (fd < 0 || buf == NULL || write_size <= 0)
		return 1;

	for (i = 0; i < SOCK_ACCSESS_WAIT_MAX; i++)
	{
		size = send (fd, buf, write_size, MSG_NOSIGNAL | MSG_DONTWAIT);
		if (size == write_size)
		{
			fsync (fd);
			return 0;
		}
		else if (size > 0)
		{
			write_size -= size;
			buf += size;
			usleep (1000);
		}
		else
			break;
	}
	return 1;
}


/* end of thread */
static void
comserv_cleanup (void* data)
{
	P_CARGS p_cargs = (P_CARGS)data;
	int fd;

	for (fd = 0; fd < *(p_cargs->p_max) + 1; fd++)
	{
		if (FD_ISSET(fd, p_cargs->p_fds))
		{
			shutdown (fd, 2);
			FD_CLR (fd, p_cargs->p_fds);
			close(fd);
		}
	}

	if (!is_sysflags (p_cargs->p_info, ST_SYS_DOWN))
		set_sysflags (p_cargs->p_info, ST_SYS_DOWN);

	p_cargs->p_info->comserv_thread_status = THREAD_DOWN;
	_DEBUG_MESSAGE("Command server thread ...down");
	return;
}


/* clear data left with a printer */
static void
clear_replay_buffer (P_CBTD_INFO p_info)
{
	char tmp_buf[REP_BUF_SIZE];
	int read_size;
	int err = 0;

	enter_critical (p_info->ecbt_accsess_critical);
	for (;;)
	{
		err = read_from_prt (p_info, SID_CTRL, NULL, &read_size);

		if (err != 0 || read_size <= 0)
			break;

		assert (REP_BUF_SIZE >= read_size);

		do
		{
			int tmp_size = read_size;
	  
			err = read_from_prt (p_info, SID_CTRL, tmp_buf, &read_size);
			read_size = tmp_size - read_size;
		}
		while (err == 0 && read_size > 0);
	}
	leave_critical (p_info->ecbt_accsess_critical);

	return;
}



static size_t
change_status_format(P_CBTD_INFO p_info, unsigned char *bdata, size_t bsize,
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

	if (is_sysflags (p_info, ST_JOB_PRINTING) && (point[2] == 4))
		point[2] = 3;
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
 	if (point ) {
 		EPS_UINT8   ParameterByte = point[1];
		if( ParameterByte == 2){
			if(point[2] == 0x10 /*struct ver.*/){
				#define HIDE_INKINFO (0x01 << 0)
				#define HIDE_INKLOW  (0x01 << 1)
				EPS_BOOL showInkInfo = (point[3] & HIDE_INKINFO) ? FALSE : TRUE;
				EPS_BOOL showInkLow  = (point[3] & HIDE_INKLOW)  ? FALSE : TRUE;

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
 	if (point ) {
 		int i, n; 
		n = point[1]; 	/* no. of warning */
		for ( i = 0; i < n ; i++ ){
			/* check only ink low */
			if( point[i + 2 ] >= 0x10 && point[i+2] <= 0x1A ){
				sprintf(cdata + count, "WR:%02X;", (int)point[i+2]);
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
					}else if ( 6 <= n ){
						/* printer has 6 color ink or more. */
						strncpy(key, "A140", 4);
					} else {
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

 	if (point ) {
 		int i,n;
//		int cc=0;  /* no. of color printed */
//		int mc=0;  /* no. of monochrome printed */
//		int wc=0;  /* no. of blank page */
//		int ac=0;  /* no. of ADF scanned */
//		int cb=0;  /* no. of color borderless printed */ 
//		int mb=0;  /* no. of monochrome borderless printed */

 		n = point[1]; 	/* size of parameter (14h / 18h / 20h */

		if(n == 0x14 || n == 0x18 || n == 0x20) {

			sprintf(cdata + count, "CC:%02X%02X%02X%02X;", (int)point[10], (int)point[11], (int)point[12], (int)point[13]);
			count += 12;
			sprintf(cdata + count, "MC:%02X%02X%02X%02X;", (int)point[14], (int)point[15], (int)point[16], (int)point[17]);
			count += 12;
			sprintf(cdata + count, "WC:%02X%02X%02X%02X;", (int)point[18], (int)point[19], (int)point[20], (int)point[21]);
			count += 12;

			if (n == 0x18){
				sprintf(cdata + count, "AC:%02X%02X%02X%02X;", (int)point[22], (int)point[23], (int)point[24], (int)point[25]);
				count += 12;

			}else if (n == 0x20){		
				sprintf(cdata + count, "AC:%02X%02X%02X%02X;", (int)point[22], (int)point[23], (int)point[24], (int)point[25]);
				count += 12;
				sprintf(cdata + count, "CB:%02X%02X%02X%02X;", (int)point[26], (int)point[27], (int)point[28], (int)point[29]);
				count += 12;
				sprintf(cdata + count, "MB:%02X%02X%02X%02X;", (int)point[30], (int)point[31], (int)point[32], (int)point[33]);
				count += 12;
			}
			

			for ( i = 0; i < n ; i++ ){
				/* check only ink low */
				if( point[i + 2 ] >= 0x10 && point[i+2] <= 0x19 ){
					sprintf(cdata + count, "WR:%02X;", (int)point[i+2]);
					count += 6;
					/* not set all warning, so break */
					break;
				}
			}
		}
 	}



	*(cdata + count) = '\0';
	count ++;
	return count;
}

static unsigned char *
scan_bin_status(StField field, unsigned char *bdata, size_t bsize)
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

int SendCommand(char* pCmd, int nCmdSize)
{
	int res = STS_NO_ERROR;
	char *p = NULL;
    	int fd;			/* Parallel device */
   	 int wbytes;		/* Number of bytes written */
	
	do {
	    if ((fd = open (FIFO_PATH, O_WRONLY | O_EXCL)) == -1) {
	        if (errno == EBUSY) {
//	            err_msg (MSGTYPE_INFO, "Backend port busy; will retry in 10 seconds...\n");
	            sleep (10);
	        } else {
	            fprintf(stderr, "Unable to open Backend port device file. %s\n",
	            strerror (errno));
	            res = EPS_PRNERR_COMM;
	            break;
	        }
	    }
	} while (fd < 0 && res == STS_NO_ERROR);

	p = pCmd;
        while (nCmdSize > 0 && res == STS_NO_ERROR) {
        if ((wbytes = write (fd, p, nCmdSize)) < 0){
            if (errno == ENOTTY){
                wbytes = write (fd, p, nCmdSize);
			}
		}

        if (wbytes < 0) {
  //          err_msg (MSGTYPE_ERROR, "Unable to send print file to printer.");
            break;
        }
        nCmdSize -= wbytes;
        p += wbytes;
	}


	if (fd > 0)	close(fd);

	return res;
}


