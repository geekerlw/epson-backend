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
#ifndef _WIN_DEF_
#define _WIN_DEF_

#include "cbtd_def.h"
#include <stdint.h>

typedef uint32_t	ULONG;
typedef ULONG 		*PULONG;
typedef uint16_t	USHORT;
typedef USHORT 		*PUSHORT;
typedef unsigned char 	UCHAR;
typedef UCHAR 		*PUCHAR;

#define MAX_PATH        260

#ifndef NULL
#define NULL    	0
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

#define CALLBACK

#ifndef CONST
#define CONST           const
#endif

typedef int32_t         INT;
typedef int32_t         LONG;
typedef uint32_t        DWORD;
typedef int32_t         BOOL;
typedef unsigned char   BYTE;
typedef uint16_t        WORD;
typedef float           FLOAT;
typedef FLOAT           *PFLOAT;
typedef BOOL		*PBOOL;
typedef BOOL		*LPBOOL;
typedef BYTE		*PBYTE;
typedef BYTE		*LPBYTE;
typedef INT		*PINT;
typedef INT		*LPINT;
typedef WORD		*PWORD;
typedef WORD		*LPWORD;
typedef LONG		*LPLONG;
typedef DWORD		*PDWORD;
typedef DWORD		*LPDWORD;
typedef void		*LPVOID;
typedef CONST void	*LPCVOID;
typedef uint32_t        UINT;
typedef UINT            *PUINT;
typedef char		CHAR;
typedef int16_t		SHORT;
typedef CHAR		*PCHAR;
typedef CHAR		*LPCH, *PCH;
typedef CONST CHAR	*LPCCH, *PCCH;
typedef CHAR		*NPSTR;
typedef CHAR		*LPSTR, *PSTR;
typedef CONST CHAR	*LPCSTR, *PCSTR;
typedef CHAR		*LPTSTR, *PTSTR;
typedef CONST CHAR	*LPCTSTR, *PCTSTR;


#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

/*  typedef void 		*HANDLE; */
typedef HANDLE          *LPHANDLE, *PHANDLE;
typedef HANDLE          HWND;
typedef unsigned char   byte;
typedef void            VOID;

typedef DWORD (*PTHREAD_START_ROUTINE) (LPVOID);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

#define FAR

#define DLL_PROCESS_ATTACH            1
#define DLL_PROCESS_DETACH            0

#define PAGE_READWRITE                0x04
#define MEM_COMMIT                    0x1000
#define MEM_RESERVE                   0x2000
#define MEM_RELEASE                   0x8000
#define LMEM_FIXED                    0x0000
#define CREATE_SUSPENDED              0x00000004

#define STATUS_TIMEOUT                ((DWORD)0x00000102L)
#define WAIT_TIMEOUT                  STATUS_TIMEOUT

#define THREAD_BASE_PRIORITY_LOWRT    15
#define THREAD_BASE_PRIORITY_IDLE     -15
#define THREAD_BASE_PRIORITY_MIN      -2
#define THREAD_BASE_PRIORITY_MAX      2
#define THREAD_PRIORITY_LOWEST        THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL  (THREAD_PRIORITY_LOWEST + 1)
#define THREAD_PRIORITY_NORMAL        0
#define THREAD_PRIORITY_HIGHEST       THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL  (THREAD_PRIORITY_HIGHEST - 1)
#define THREAD_PRIORITY_ERROR_RETURN  (MAXLONG)
#define THREAD_PRIORITY_TIME_CRITICAL THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE          THREAD_BASE_PRIORITY_IDLE

typedef struct _COMMTIMEOUTS
{
  DWORD ReadIntervalTimeout;
  DWORD ReadTotalTimeoutMultiplier;
  DWORD ReadTotalTimeoutConstant;
  DWORD WriteTotalTimeoutMultiplier;
  DWORD WriteTotalTimeoutConstant;
} COMMTIMEOUTS, *LPCOMMTIMEOUTS;

#define DECLARE_HANDLE(name) typedef HANDLE name

DECLARE_HANDLE(HINSTANCE);

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, *PRECT, *LPRECT;

typedef const RECT 			*LPCRECT;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagPOINTS
{
    SHORT  x;
    SHORT  y;
} POINTS, *PPOINTS, *LPPOINTS;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

typedef struct _MEMORYSTATUS { // mst 
    DWORD dwLength;        // sizeof(MEMORYSTATUS) 
    DWORD dwMemoryLoad;    // percent of memory in use 
    DWORD dwTotalPhys;     // bytes of physical memory 
    DWORD dwAvailPhys;     // free physical memory bytes 
    DWORD dwTotalPageFile; // bytes of paging file 
    DWORD dwAvailPageFile; // free bytes of paging file 
    DWORD dwTotalVirtual;  // user bytes of address space 
    DWORD dwAvailVirtual;  // free user bytes 
} MEMORYSTATUS, *LPMEMORYSTATUS; 

#define HLOCAL HANDLE
typedef LONG *PLONG;

typedef struct _OVERLAPPED
{
  DWORD Internal;
  DWORD InternalHigh;
  DWORD Offset;
  DWORD OffsetHigh;
  HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES
{
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2

#define STATUS_WAIT_0 ((DWORD)0x00000000L)
#define WSABASEERR 10000
#define WSAETIMEDOUT (WSABASEERR + 60)
/*  #define ETIMEDOUT WSAETIMEDOUT */

#define WAIT_FAILED (DWORD)0xFFFFFFFF
#define WAIT_OBJECT_0 ((STATUS_WAIT_0) + 0)

#include <pthread.h>

typedef struct _WIN_EVENT_OBJECT
{
  char id;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int manual_reset;
  int state_flag;
} WIN_EVENT_OBJECT, *LP_WIN_EVENT_OBJECT;
typedef struct _CRITICAL_OBJECT
{
  char id;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int state_flag;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;


typedef struct _WIN_THREAD_OBJECT
{
  char id;
  pthread_t thread;
  int count;
} WIN_THREAD_OBJECT, *LP_WIN_THREAD_OBJECT;

#endif /* _WIN_DEF_ */

