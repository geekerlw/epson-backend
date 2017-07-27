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

#include <signal.h>
#include "epson-daemon.h"
#include "epson-thread.h"

static void kill_cbtd(int);
extern int pid_fd;

/* Setting of signal handler */
void
sig_set(void)
{
	//signal(SIGHUP, kill_cbtd);
	signal(SIGINT, kill_cbtd);
	//signal(SIGQUIT, kill_cbtd);
	signal(SIGTERM, kill_cbtd);

	return;
}


/* Signal handler */
static void
kill_cbtd(int sig)
{
	_DEBUG_MESSAGE_VAL("Signal handler called : ", sig);
	if (-1 != pid_fd)
	{
		close(pid_fd);
	}
	unlink(PID_PATH);
	_exit(0);
	return;
}
