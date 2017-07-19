## Makefile.am -- an automake template for a Makefile.in file
## Copyright (C) Seiko Epson Corporation 2014.
##  This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


cupsbackenddir = $(prefix)$(CUPS_SERVER_DIR)/backend

cupsbackend_PROGRAMS =ecblp

noinst_LTLIBRARIES = libcutils.la

MODEL_FULL = LITE
MODEL = lite

libcutils_la_SOURCES = \
	gettext.h \
	str.c str.h \
	err.c err.h \
	mem.c mem.h \
	def.h


ecblp_LDFLAGS = \
	@CUPS_LIBS@ \
	libcutils.la
ecblp_SOURCES = \
	backend.c
