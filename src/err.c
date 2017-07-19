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
 * As a special exception, AVASYS CORPORATION gives permission to
 * link the code of this program with libraries which are covered by
 * the AVASYS Public License and distribute their linked
 * combinations.  You must obey the GNU General Public License in all
 * respects for all of the code used other than the libraries which
 * are covered by AVASYS Public License.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"
#define  _(msg_id)	gettext (msg_id)

#include "err.h"

/* global  */
static char err_pname[256] = "";

/* static functions */
static void err_doit (enum msgtype, int, const char *, va_list);

void
err_init (const char *name)
{
	if (name && strlen (name) < 256)
		strcpy (err_pname, name);
	return;
}

void
err_msg (enum msgtype type, const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	err_doit (type, 0, fmt, ap);

	va_end (ap);
	return;
}

void
err_fatal (const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	err_doit (MSGTYPE_ERROR, 0, fmt, ap);

	va_end (ap);
	exit (1);
}

void
err_system (const char *fmt,...)
{
	int e;
	va_list ap;

	e = errno;
	va_start (ap, fmt);
	err_doit (MSGTYPE_ERROR, e, fmt, ap);

	va_end (ap);
	exit (1);
}

static void
err_doit (enum msgtype type, int e, const char *fmt, va_list ap)
{
	if (err_pname[0] != '\0')
		fprintf (stderr, "%s : ", err_pname);

	if (type == MSGTYPE_ERROR)
		fprintf (stderr, _("**** ERROR **** : "));
	else if (type == MSGTYPE_WARNING)
		fprintf (stderr, _("**** WARNING **** : "));
	else if (type == MSGTYPE_INFO)
		fprintf (stderr, _("**** INFO **** : "));
		
	vfprintf (stderr, fmt, ap);

	if (e)
		fprintf (stderr, " : %s", strerror (e));
	
	fprintf (stderr, "\n");
	fflush (stderr);
	return;
}
