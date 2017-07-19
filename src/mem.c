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

#include <stdlib.h>
#include <mem.h>
#include <err.h>


void *
mem_malloc (unsigned int size, bool_t crit)
{
	void *m;

	m = malloc (size);
	if (crit && m == NULL)
		err_system ("mem_malloc");

	return m;
}

void *
mem_calloc (unsigned int num, unsigned int size, bool_t crit)
{
	void *m;

	m = calloc (num, size);
	if (crit && m == NULL)
		err_system ("mem_calloc");

	return m;
}

void *
mem_realloc (void *m, u_int size, bool_t crit)
{
	m = realloc (m, size);
	if (crit && m == NULL)
		err_system ("mem_realloc");

	return m;
}

void
mem_free (void *m)
{
	if (m) free (m);

	return;
}
