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
#include <assert.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

#ifndef _CRT_NO_TIME_T
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#endif

#include <sys/types.h>

#include "epson-def.h"
#include "epson-typedefs.h"
#include "epson-thread.h"
#include "epson-daemon.h"

#pragma comment(lib,"x86/pthreadVSE2.lib")

/* structure for semaphore */
typedef struct _SEM_OBJECT
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int status_flag;
} SEM_OBJECT, *P_SEM_OBJECT;

/* Make thread and start it */
void* init_thread(int stack_size, void* function, void* param)
{
	pthread_t* p_thread;
	pthread_attr_t attr;
	int err = 0;

	p_thread = (pthread_t*)calloc(1, sizeof(pthread_t));
	if (!p_thread)
		return NULL;

	err = pthread_attr_init(&attr);
	err += pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	err += pthread_create(p_thread, &attr, function, param);

	assert(err == 0);
	return (void*)p_thread;
}

/* Thread stop and delete it */
void delete_thread(void* handle)
{
	pthread_t* p_thread = (pthread_t*)handle;

	if (p_thread)
	{
		pthread_join(*p_thread, NULL);
		pthread_detach(*p_thread);

		free(p_thread);
		p_thread = NULL;
	}
	return;
}

/* Cancel thread */
void cancel_thread(void* handle)
{
	pthread_cancel(*(pthread_t *)handle);
	return;
}

/* Initialize semaphore */
HANDLE init_critical(void)
{
	P_SEM_OBJECT p_semobj;
	int err = 0;

	p_semobj = (P_SEM_OBJECT)calloc(1, sizeof(SEM_OBJECT));
	if (!p_semobj)
		return NULL;

	p_semobj->status_flag = 0;

	err = pthread_mutex_init(&p_semobj->mutex,
		(pthread_mutexattr_t *)NULL);
	err += pthread_cond_init(&p_semobj->cond,
		(pthread_condattr_t *)NULL);
	assert(err == 0);
	return (HANDLE)p_semobj;
}

/* Critical section begins */
void enter_critical(HANDLE handle)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)handle;
	int err = 0;

	assert(p_semobj);

	err = pthread_mutex_lock(&p_semobj->mutex);
	while (err == 0 && p_semobj->status_flag)
	{
		err = pthread_cond_wait(&p_semobj->cond,
			&p_semobj->mutex);
	}
	p_semobj->status_flag = 1;
	err += pthread_mutex_unlock(&p_semobj->mutex);

	assert(err == 0);
	return;
}

/* Critical section ends */
void leave_critical(HANDLE handle)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)handle;
	int err = 0;

	assert(p_semobj);

	err = pthread_mutex_lock(&p_semobj->mutex);
	p_semobj->status_flag = 0;
	err += pthread_cond_signal(&p_semobj->cond);
	err += pthread_mutex_unlock(&p_semobj->mutex);

	assert(err == 0);
	return;
}


/* Delete semaphore */
void delete_critical(HANDLE handle)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)handle;
	int err = 0;

	assert(p_semobj);

	err = pthread_cond_destroy(&p_semobj->cond);
	err += pthread_mutex_destroy(&p_semobj->mutex);
	free(p_semobj);
	p_semobj = NULL;

	assert(err == 0);
	return;
}


/* Turn the flag of system state into ON */
void set_sysflags(P_CBTD_INFO p_info, int flags)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)p_info->sysflags_critical;
	int err = 0;

	assert(p_semobj);

	err = pthread_mutex_lock(&p_semobj->mutex);
	p_info->sysflags |= flags;

	err += pthread_cond_broadcast(&p_semobj->cond);
	err += pthread_mutex_unlock(&p_semobj->mutex);

	assert(err == 0);
	return;
}

/* Turn the flag of system state into OFF */
void reset_sysflags(P_CBTD_INFO p_info, int flags)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)p_info->sysflags_critical;
	int err = 0;

	assert(p_semobj);

	err = pthread_mutex_lock(&p_semobj->mutex);
	p_info->sysflags &= (flags ^ 0xff);

	err += pthread_cond_broadcast(&p_semobj->cond);
	err += pthread_mutex_unlock(&p_semobj->mutex);

	assert(err == 0);
	return;
}

/* compare the p_info's flags with given flags follow */
int is_sysflags(P_CBTD_INFO p_info, int flags)
{
	return p_info->sysflags & flags;
}

int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = (long)clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}

/* Wait till flag of system state is changed
* wait_type
*      WAIT_SYS_OR : wait for even condition one if flags fill it, set_flags may be 0
*					 p_info->sys_flags has any one of reset_flags.
*      WAIT_SYS_AND : wait if flags satisfy every condition, set_flags must exist
*					 p_info->sys_flags has all of reset_flags and no set_flags.					
*
* returns zero on success, or 1 if an timeout.
*/
int wait_sysflags(P_CBTD_INFO p_info, int set_flags,
	int reset_flags, int sec, int wait_type)
{
	P_SEM_OBJECT p_semobj = (P_SEM_OBJECT)p_info->sysflags_critical;
	int tflag = 0;
	int err = 0;

	assert(p_semobj);

	err += pthread_mutex_lock(&p_semobj->mutex);
	for (;;)
	{
		if (wait_type == WAIT_SYS_AND)
		{
			if (is_sysflags(p_info, set_flags) == 0
				&& is_sysflags(p_info, reset_flags) == reset_flags)
				break;
		}
		else if (wait_type == WAIT_SYS_OR)
		{
			if (is_sysflags(p_info, set_flags) != set_flags
				|| is_sysflags(p_info, reset_flags) != 0)
				break;
		}
		else
		{
			tflag = 1;
			break;
		}

		if (sec)
		{
			struct timeval now;
			struct timespec timeout;

			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + sec;
			timeout.tv_nsec = now.tv_usec * 1000;

			if (pthread_cond_timedwait(&p_semobj->cond,
				&p_semobj->mutex,
				&timeout))
			{
				tflag = 1;
				break;
			}
		}
		else
		{
			pthread_cond_wait(&p_semobj->cond,
				&p_semobj->mutex);
		}
	}

	err += pthread_mutex_unlock(&p_semobj->mutex);

	assert(err == 0);
	return tflag;
}
