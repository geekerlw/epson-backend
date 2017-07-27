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
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>
//#include <sys/time.h>
#include <time.h>
//#include <sys/ioctl.h>
//#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "daemon-osfunc.h"

#define RETRY_MAX 2
#define ACCESS_TIMEOUT 1

enum _OBJECT_ID
{
	OID_EVENT = 0,
	OID_THREAD,
	OID_CRITICAL
};

static void access_alarm(int);


HLOCAL
LocalAlloc(UINT uFlags, /* ignore */
	UINT uBytes)
{
	HLOCAL buf;

	buf = (HLOCAL)calloc(uBytes, 1);
	assert(buf);
	return buf;
}


HLOCAL
LocalFree(HLOCAL hMem)
{
	free((void *)hMem);
	return NULL;
}


LPVOID
VirtualAlloc(LPVOID lpAddress, /* ignore */
	DWORD dwSize,
	DWORD flAllocationType, /* ignore */
	DWORD flProtect /* ignore */)
{
	return (LPVOID)LocalAlloc(0, (UINT)dwSize);
}


BOOL
VirtualFree(LPVOID lpAddress,
	DWORD dwSize, /* ignore */
	DWORD dwFreeType /* ignore */)
{
	free(lpAddress);
	return 1;
}


VOID
Sleep(DWORD dmMilliseconds)
{
	unsigned long stime;

	stime = dmMilliseconds * 1000;
	usleep(stime);

	return;
}


BOOL
ReadFile(HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToResd,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	int* lp_fd = (int *)hFile;
	void* sigfunc;
	long rsize = 0;
	int i;

	_DEBUG_MESSAGE_VAL("Sys Read Size =", (int)nNumberOfBytesToResd);

	for (i = 0; i < RETRY_MAX; i++)
	{
		sigfunc = signal(SIGALRM, access_alarm);
		alarm(ACCESS_TIMEOUT);

		rsize = read(*lp_fd, lpBuffer, nNumberOfBytesToResd);

		alarm(0);
		signal(SIGALRM, sigfunc);

		if (rsize > 0)
			break;

		usleep(50000);
	}

	if (lpNumberOfBytesRead)
	{
		if (rsize > 0)
			*lpNumberOfBytesRead = rsize;
		else
			*lpNumberOfBytesRead = 0;
	}

	_DEBUG_MESSAGE_VAL("System Read =", (int)rsize);

	if (rsize >= 0)
		return 1;

	_DEBUG_MESSAGE("Read Error !!");

	return 0;
}


/* handler for timeout */
static void
access_alarm(int sig)
{
	_DEBUG_MESSAGE("Device time out");
	return;
}


DWORD
SetFilePointer(HANDLE hFile,
	LONG lDistenceToMove,
	PLONG lpDistenceToMoveHigh,
	DWORD dwMoveMethod)
{
	int* lp_fd = (int *)hFile;
	int ofs, flag;

	assert(!lpDistenceToMoveHigh);

	switch (dwMoveMethod)
	{
	case FILE_BEGIN:
		flag = SEEK_SET;
		break;

	case FILE_CURRENT:
		flag = SEEK_CUR;
		break;

	case FILE_END:
		flag = SEEK_END;
		break;

	default:
		assert(0);
	}

	ofs = lseek(*lp_fd, lDistenceToMove, flag);
	if (ofs < 0)
		return 0xffffffff;
	else
		return (DWORD)ofs;
}


DWORD
GetTickCount(VOID)
{
	struct timeval tv;
	struct timezone tz;

	if ((gettimeofday(&tv, &tz)) != 0)
		return 0;

	return tv.tv_sec * 1000 + tv.tv_usec;
}


HANDLE
CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, /* NULL */
	BOOL bManualReset,
	BOOL bInitialState,
	LPCTSTR lpName)	/* NULL */
{
	LP_WIN_EVENT_OBJECT eventobj;
	int err_code = 0;

	eventobj = (LP_WIN_EVENT_OBJECT)calloc(1, sizeof(WIN_EVENT_OBJECT));
	if (!eventobj)
		return NULL;

	eventobj->id = OID_EVENT;
	eventobj->manual_reset = bManualReset;
	eventobj->state_flag = bInitialState;

	/* realize one part of facility of Event object of Windows
	with Mutex&Cond */
	err_code = pthread_mutex_init(&eventobj->mutex,
		(pthread_mutexattr_t *)NULL);
	if (err_code)
	{
		free(eventobj);
		return NULL;
	}

	/* because the Event object that attribute is set does not
	exist in ECBT, attribute object does not use it. */
	err_code = pthread_cond_init(&eventobj->cond, (pthread_condattr_t *)NULL);
	if (err_code)
	{
		free(eventobj);
		pthread_mutex_destroy(&eventobj->mutex);
		return NULL;
	}

	return (HANDLE)eventobj;
}


BOOL
SetEvent(HANDLE hEvent)
{
	LP_WIN_EVENT_OBJECT eventobj = (LP_WIN_EVENT_OBJECT)hEvent;
	int err_code = 0;

	if (!eventobj)
		return 1;

	err_code = pthread_mutex_lock(&eventobj->mutex);
	if (err_code)
		return 1;

	eventobj->state_flag = 1;

	pthread_cond_signal(&eventobj->cond);
	pthread_mutex_unlock(&eventobj->mutex);

	return 0;

}

BOOL
ResetEvent(HANDLE hEvent)
{
	LP_WIN_EVENT_OBJECT eventobj = (LP_WIN_EVENT_OBJECT)hEvent;
	int err_code = 0;

	err_code = pthread_mutex_lock(&eventobj->mutex);

	eventobj->state_flag = 0;

	pthread_cond_signal(&eventobj->cond);
	pthread_mutex_unlock(&eventobj->mutex);

	return 0;
}

DWORD
WaitForSingleObject(HANDLE hHandle,
	DWORD dwMilliseconds)
{
	char oid = *((char *)hHandle);
	int err_code = 0;

	if (!hHandle)
		return WAIT_FAILED;


	if (oid == OID_EVENT)
	{
		LP_WIN_EVENT_OBJECT eventobj = (LP_WIN_EVENT_OBJECT)hHandle;
		struct timeval now;
		struct timespec timeout;

		err_code = pthread_mutex_lock(&eventobj->mutex);

		while (!eventobj->state_flag && err_code == 0)
		{
			if (dwMilliseconds)
			{
				gettimeofday(&now, NULL);
				timeout.tv_sec = now.tv_sec + (dwMilliseconds / 1000);
				timeout.tv_nsec = now.tv_usec;

				err_code = pthread_cond_timedwait(&eventobj->cond,
					&eventobj->mutex,
					&timeout);
			}
			else
				err_code = pthread_cond_wait(&eventobj->cond,
					&eventobj->mutex);
		}

		if (err_code)
		{
			if (err_code == ETIMEDOUT)
				return WAIT_TIMEOUT;
			return WAIT_FAILED;
		}

		if (!eventobj->manual_reset)
			eventobj->state_flag = 0;

		pthread_mutex_unlock(&eventobj->mutex);

		return WAIT_OBJECT_0;
	}

	else if (oid == OID_THREAD)
	{
		return WAIT_OBJECT_0;
	}
	return WAIT_FAILED;
}


HANDLE
CreateThread(LPSECURITY_ATTRIBUTES lpTreadAttributes, /* NULL */
	DWORD dwStackSize, /* 0x4000 */
	LPTHREAD_START_ROUTINE lpStartAddress,
	LPVOID lpParameter,
	DWORD dwCreationFlags,
	LPDWORD lpThreadId)
{
	LP_WIN_THREAD_OBJECT threadobj;
	pthread_attr_t attr;

	threadobj = (LP_WIN_THREAD_OBJECT)calloc(1, sizeof(WIN_THREAD_OBJECT));
	if (!threadobj)
		return NULL;

	threadobj->id = OID_THREAD;

	/* In LinuxThread, there is not suspend */
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, (size_t)dwStackSize);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&threadobj->thread, &attr,
		(void *)lpStartAddress, lpParameter);
	threadobj->count = 0;

	return (HANDLE)threadobj;
}


DWORD
ResumeThread(HANDLE hTread)
{
	return 0;
}


BOOL
TerminateThread(HANDLE hThread,
	DWORD dwExitCode)
{
	LP_WIN_THREAD_OBJECT threadobj = (LP_WIN_THREAD_OBJECT)hThread;

	if (threadobj == NULL)
		return 1;

	pthread_cancel(threadobj->thread);
	free(threadobj);
	return 1;
}


/* modification of priority does not support it */
BOOL
SetThreadPriority(HANDLE hTreadPriority,
	int nPriority)
{
	return 1;
}

int
GetThreadPriority(HANDLE hThread)
{
	return THREAD_PRIORITY_NORMAL;
}


VOID
InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	int err_code = 0;

	lpCriticalSection->id = OID_CRITICAL;
	lpCriticalSection->state_flag = 0;

	err_code += pthread_mutex_init(&lpCriticalSection->mutex, (pthread_mutexattr_t *)NULL);
	if (err_code)
		return;

	err_code = pthread_cond_init(&lpCriticalSection->cond, (pthread_condattr_t *)NULL);
	if (err_code)
	{
		err_code += pthread_mutex_destroy(&lpCriticalSection->mutex);
	}
	return;
}


VOID
EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	int err_code;

	err_code = pthread_mutex_lock(&lpCriticalSection->mutex);

	while (lpCriticalSection->state_flag)
	{
		pthread_cond_wait(&lpCriticalSection->cond,
			&lpCriticalSection->mutex);
	}

	lpCriticalSection->state_flag = 1;

	pthread_mutex_unlock(&lpCriticalSection->mutex);

	return;
}


VOID
LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	int err_code;

	err_code = pthread_mutex_lock(&lpCriticalSection->mutex);

	lpCriticalSection->state_flag = 0;
	pthread_cond_signal(&lpCriticalSection->cond);
	pthread_mutex_unlock(&lpCriticalSection->mutex);

	return;
}

VOID
DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
	if (pthread_cond_destroy(&lpCriticalSection->cond))
		return;

	if (pthread_mutex_lock(&lpCriticalSection->mutex))
		return;

	if (pthread_mutex_destroy(&lpCriticalSection->mutex))
		return;

	lpCriticalSection->id = -1;
	lpCriticalSection->state_flag = -1;

	return;
}


BOOL
CloseHandle(HANDLE handle)
{
	char oid = *((char *)handle);

	if (!handle)
		return 0;

	if (oid == OID_EVENT)
	{
		LP_WIN_EVENT_OBJECT eventobj = (LP_WIN_EVENT_OBJECT)handle;

		pthread_cond_destroy(&eventobj->cond);
		pthread_mutex_destroy(&eventobj->mutex);
		free(eventobj);

		return 1;
	}

	else if (oid == OID_THREAD)
	{
		LP_WIN_THREAD_OBJECT threadobj = (LP_WIN_THREAD_OBJECT)handle;

		pthread_cancel(threadobj->thread);
		pthread_join(threadobj->thread, NULL);
		pthread_detach(threadobj->thread);
		free(threadobj);

		return 1;
	}

	return 0;
}



VOID
ExitThread(DWORD dwExitCode)
{
	int status = 0;

	pthread_exit((void *)&status);
	return;

}

HANDLE
GetCurrentThread(VOID)
{
	return NULL;
}


BOOL
WriteFile(HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	int fd = *((int *)hFile);
	long wsize = 0;
	fd_set fds;
	int i;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	for (i = 0; i < RETRY_MAX; i++)
	{
		struct timeval tv;
		fd_set watch_fds = fds;

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		if (select(fd + 1, NULL, &watch_fds, NULL, &tv) < 0)
		{
			_DEBUG_FUNC(perror("select() : lp write"));
			break;
		}

		if (FD_ISSET(fd, &watch_fds))
		{
			void* sigfunc;

			sigfunc = signal(SIGALRM, access_alarm);
			alarm(ACCESS_TIMEOUT);

			wsize = write(fd, lpBuffer, nNumberOfBytesToWrite);

			alarm(0);
			signal(SIGALRM, sigfunc);

			if (wsize > 0)
				break;
		}
	}

	if (wsize > 0)
	{
		fsync(fd);

		if (lpNumberOfBytesWritten)
			*lpNumberOfBytesWritten = wsize;
		return 1;
	}

	_DEBUG_MESSAGE_VAL("Write Error !!", fd);
	return 0;
}

