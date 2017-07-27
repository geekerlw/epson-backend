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
#ifndef __WINFUNC_H__
#define __WINFUNC_H__

#include "daemon_osdef.h"

HLOCAL LocalAlloc(UINT uFlags, UINT uBytes);
HLOCAL LocalFree(HLOCAL hMem);
LPVOID VirtualAlloc(LPVOID lpAddress, DWORD dwSize,
	DWORD flAllocationType, DWORD flProtect);
BOOL VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType);
VOID Sleep(DWORD dmMilliseconds);
BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToResd,
	LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
DWORD SetFilePointer(HANDLE hFile, LONG lDistenceToMove,
	PLONG lpDistenceToMoveHigh, DWORD dwMoveMethod);
DWORD GetTickCount(VOID);
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,
	BOOL bInitialState, LPCTSTR lpName);
BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpTreadAttributes, DWORD dwStackSize,
	LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
	DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD ResumeThread(HANDLE hTread);
BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode);
BOOL SetThreadPriority(HANDLE hTreadPriority, int nPriority);
int GetThreadPriority(HANDLE hThread);
VOID InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
VOID EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
VOID LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
VOID DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
BOOL CloseHandle(HANDLE handle);
VOID ExitThread(DWORD dwExitCode);
HANDLE GetCurrentThread(VOID);

#endif /* __WINFUNC_H__ */

