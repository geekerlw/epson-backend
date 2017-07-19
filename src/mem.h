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
#include <config.h>
#endif

#ifndef MEM_H
#define MEM_H

#include "def.h"

/* If it is a success, an address is returned, and if it is failure, a message is outputted and it ends. */
#define mem_new(type, num) \
((type *) mem_malloc (sizeof (type) * (num), 1))
#define mem_new0(type, num) \
((type *) mem_calloc ((num), sizeof (type), 1))
#define mem_renew(type, pointer, num) \
((type *) mem_realloc (pointer, sizeof (type) * (num), 1))

/*
* bool value true Critical
* false Normal operation
*/
BEGIN_C

void * mem_malloc (unsigned int, bool_t);
void * mem_calloc (unsigned int, unsigned int, bool_t);
void * mem_realloc (void *, unsigned int, bool_t);
void mem_free (void *);

END_C

#endif /* MEM_H */
