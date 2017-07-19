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


SUBDIRS = rc
#pkglib_PROGRAMS = ecbd

daemondir = /usr/lib/epson-backend
daemon_PROGRAMS = ecbd

ecbd_SOURCES = \
	cbtd.c \
	cbtd.h \
	cbtd_comserv.c \
	cbtd_datatrans.c \
	cbtd_def.h \
	cbtd_setup.c \
	cbtd_signal.c \
	cbtd_thread.c \
	cbtd_thread.h \
	cbtd_wrapper.c \
	cbtd_wrapper.h \
	ecbt_lcl.h \
	ecbteg.c \
	ecbteg.h \
	daemon_osdef.h \
	daemon_osfunc.c \
	daemon_osfunc.h \
	epmnt.c \
	libcbt.h \
	epson-typdefs.h

CLEANFILES = $(pkgconf_DATA)
