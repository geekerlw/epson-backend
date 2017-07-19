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

#if DEBUG
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include <err.h>
#include <debug.h>

static char *debug_path = "/tmp/epson-backend-wrapper-dump";
static FILE *debug_fp = NULL;

void
debug_init (void)
{
	char path[255];
	pid_t pid;
	
	pid = getpid ();
	sprintf (path, "%s.%d", debug_path, pid);
	
	if (!debug_fp)
	{
		debug_fp = fopen (path, "w");
		if (!debug_fp)
			err_msg (MSGTYPE_WARNING, "Can't open the dump file \"%s\".", path);
	}

	fchmod (fileno (debug_fp), 0644);
	return;
}


void
debug_dump (const char *fmt, ...)
{
	va_list ap;

	if (!debug_fp)
		debug_init ();

	va_start (ap, fmt);
	vfprintf (debug_fp, fmt, ap);

	va_end (ap);
	return;
}


void
debug_end (void)
{
	if (debug_fp != NULL)
		fclose (debug_fp);

	return;
}

#endif /* DEBUG */
