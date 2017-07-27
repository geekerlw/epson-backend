/*
* Copyright (C) Seiko Epson Corporation 2014.
*
* This file is part of the `cbtd' program.
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

#ifndef __CBTD_DEF_H__
#define __CBTD_DEF_H__

/* printer access wait time */
#define PRINTER_ACCESS_WAIT 120000

#define DAEMON_PORT        35587
#define DEVICE_PATH        "/dev/usb/lp0"
#define FIFO_PATH	      "/var/run/ecblp0"

#define EPS_MNT_NOZZLE     0
#define EPS_MNT_CLEANING   1

#define EPS_INK_NUM		   20

/* Debug massage */

#ifdef _DEBUG
#include <stdio.h>
#define _DEBUG_FUNC(x) x
#define _DEBUG_MESSAGE(x) fprintf (stderr, "Debug Message >>> %s\n", x)
#define _DEBUG_MESSAGE_VAL(x, y) fprintf (stderr, "Debug Message >>> %s %d\n", x, y)
#else
#define _DEBUG_FUNC(x)
#define _DEBUG_MESSAGE(x)
#define _DEBUG_MESSAGE_VAL(x, y)
#endif

#endif /* __CBTD_DEF_H__ */
