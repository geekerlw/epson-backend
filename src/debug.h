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
#ifndef DEBUG_H
#define DEBUG_H
#include <stdarg.h>

#include <def.h>

#if !DEBUG   /* not DEBUG */
#define DEBUG_START
#define DEDBUG_END

#define DEBUG_QUALITY_STRUCT(str)
#define DEBUG_JOB_STRUCT(str)

#else /* DEBUG */
#define DEBUG_START debug_init ()
#define DEDBUG_END debug_end ()

#define DEBUG_QUALITY_STRUCT(str) \
debug_dump ("--- ESCPR_PRINT_QUALITY ---\n\n" \
	    "mediaTypeID  = %d\n" \
	    "quality      = %d\n" \
	    "colormode    = %d\n" \
	    "brightness   = %d\n" \
	    "contrast     = %d\n" \
	    "saturation   = %d\n" \
	    "colorPlane   = %d\n" \
	    "paltetteSize = %d\n" \
	    "--------------------------\n\n" \
	    ,str.MediaTypeID \
	    ,str.PrintQuality \
            ,str.ColorMono \
	    ,str.Brightness \
	    ,str.Contrast \
	    ,str.Saturation \
	    ,str.ColorPlane \
	    ,str.PaletteSize)

#define DEBUG_JOB_STRUCT(str) \
debug_dump ("--- ESCPR_PRINT_JOB  ---\n\n" \
	    "paperWidth     = %ld\n" \
	    "paperLength    = %ld\n" \
	    "topMargin      = %d\n" \
	    "leftMargin     = %d\n" \
	    "areaWidth      = %ld\n" \
	    "areaLength     = %ld\n" \
	    "inResolution   = %d\n" \
	    "printDirection = %d\n" \
	    "--------------------------\n\n" \
	    ,str.PaperWidth \
	    ,str.PaperLength \
	    ,str.TopMargin \
	    ,str.LeftMargin \
	    ,str.PrintableAreaWidth \
	    ,str.PrintableAreaLength \
	    ,str.InResolution \
	    ,str.PrintDirection)

BEGIN_C

void debug_init (void);
void debug_dump (const char *, ...);
void debug_end (void);

END_C

#endif /* DEBUG */

#endif /* DEBUG_H */
