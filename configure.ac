dnl configure.in -- Process this file with autoconf to produce a configure script.
dnl Copyright (C) Seiko Epson Corporation 2014.
dnl  This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

AC_PREREQ(2.57)
AC_INIT([Epson Custom Backend],
	[1.0.1], [SEIKO EPSON <linux-printer@epson.jp>], [epson-backend])
AC_DEFINE(LAST_UPDATE, "2017/02/06", "")
AC_SUBST(PACKAGE_RELEASE, 1)            dnl  keep ALL versioning info in
                                        dnl  one location
AC_SUBST(LSB_VER, [3.2]) 
AM_INIT_AUTOMAKE([gnu 1.7])

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/str.c)


dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL

AC_LIBLTDL_INSTALLABLE
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_LIB_LTDL
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)

AC_CHECK_PROG(have_cups_config, cups-config, yes)
if test $have_cups_config = yes; then
   CUPS_CFLAGS=`cups-config --cflags`
   CUPS_LDFLAGS=`cups-config --ldflags`
dnl    CUPS_LIBS=`cups-config --libs | sed -e 's,-lssl,,;s,-lcrypto,,;s,-lcrypt,,'`
   CUPS_LIBS='-lcups -lm'
dnl    CUPS_IMAGE_LIBS=`cups-config --image --libs | sed -e 's,-lssl,,;s,-lcrypto,,;s,-lcrypt,,;s,-ltiff,,;s,-ljpeg,,;s,-lpng,,'`
   CUPS_IMAGE_LIBS='-lcupsimage -lcups -ljpeg -lm'
dnl `@<:@' , `@:>@' are replaced with  `[' , `]'
   CUPS_SERVER_DIR=`cups-config --serverbin | sed -e 's,^/@<:@^/@:>@@<:@^/@:>@*,,'`
   CUPS_DATA_DIR=/share/cups
else
   AC_MSG_ERROR([*** 'cups-config' missing, please install CUPS or fix your \$PATH ***])
fi

AC_SUBST(CUPS_CFLAGS)
AC_SUBST(CUPS_LDFLAGS)
AC_SUBST(CUPS_LIBS)
AC_SUBST(CUPS_IMAGE_LIBS)
AC_SUBST(CUPS_SERVER_DIR)
AC_SUBST(CUPS_DATA_DIR)

dnl Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(fcntl.h limits.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h png.h)
AC_ISC_POSIX

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(alarm gettimeofday memset mkfifo select socket strcspn strstr)


dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}", "")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}", "")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}", "")
fi

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

dnl  appendix

AC_DEFINE(CONV_VERSION2,1,CONV_VERSION2)
AC_DEFINE(_V1_SUPPORT,1,_V1_SUPPORT)
AC_DEFINE(DEBUG,0,DEBUG)
AC_DEFINE(ROLL_PAPER,1,ROLL_PAPER)
AC_DEFINE(USE_INKBOX,1,USE_INKBOX)
AC_DEFINE(R_RESOLUTION,1,R_RESOLUTION)
AC_DEFINE(SPC_MODEL,1,SPC_MODEL)
AC_DEFINE(CD_DVD_OPEN,1,CD_DVD_OPEN)
AC_DEFINE(NO_ACTION_PAPERJAM,1,NO_ACTION_PAPERJAM)


AM_CONDITIONAL(WITH_MODULES, true)
AM_CONDITIONAL(WIN32_NATIVE_BUILD, false)
AM_CONDITIONAL(CYGWIN_BUILD, false)
AC_DEFINE_UNQUOTED(MagickShareConfigSubDir,".",dummy)
AC_DEFINE_UNQUOTED(MagickLibConfigSubDir,".",dummy)
AC_DEFINE_UNQUOTED(QuantumDepth,8,[Number of bits in a pixel Quantum (8/16/32)])
AC_SUBST(QuantumDepth)
AC_CHECK_FUNCS([atoll _exit ftime getexecname getc_unlocked getpagesize \
  getpid lltostr madvise mkstemp _NSGetExecutablePath _pclose pclose poll \
  _popen popen pread pwrite putc_unlocked raise rand_r readlink realpath \
  select seekdir sigemptyset sigaction strerror strlcat strlcpy strtoll \
  sysconf tempnam times telldir ulltostr vsprintf vsnprintf])
AC_CHECK_HEADERS(sys/times.h sys/types.h)

AC_CHECK_DECLS([pread, pwrite],[],[],[
#include <unistd.h>])

AC_CHECK_DECLS([strlcpy],[],[],[
#include <strings.h>])

AC_CHECK_DECLS([vsnprintf],[],[],[
#include <stdio.h>
#include <stdarg.h>])


AC_CONFIG_FILES( \
Makefile \
src/Makefile \
daemon/Makefile \
daemon/rc/Makefile \
setup/Makefile \
epson-backend.spec
)
AC_OUTPUT
