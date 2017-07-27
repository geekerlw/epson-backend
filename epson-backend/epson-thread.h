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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef __EPSON_THREAD_H__
#define __EPSON_THREAD_H__

#include "epson-def.h"
#include "epson-daemon.h"

/* information to do cleanup of a system */
typedef struct _CLEANUP_ARGS
{
	P_CBTD_INFO p_info;		/* daemon infomation */
	int* p_max;			/* high limit of file descriptor */
	fd_set* p_fds;		/* Set of file descriptor */
} CARGS, *P_CARGS;

typedef struct timespec {
	time_t tv_sec;
	long tv_nsec;
}TIMESPEC;

enum _SYS_FLAGS_WAIT_TYPES
{
	WAIT_SYS_OR = 0,  /* wait for even condition one if flags fill it */
	WAIT_SYS_AND		   /* wait if flags satisfy every condition */
};


HANDLE init_thread(int, void*, void*);
void delete_thread(HANDLE);
void cancel_thread(void*);

HANDLE init_critical(void);
void enter_critical(HANDLE);
void leave_critical(HANDLE);
void delete_critical(HANDLE);

void set_sysflags(P_CBTD_INFO, int);
void reset_sysflags(P_CBTD_INFO, int);
int is_sysflags(P_CBTD_INFO, int);
int wait_sysflags(P_CBTD_INFO, int, int, int, int);

#endif /* __EPSON_THREAD_H__ */
