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

rcddir = $(pkglibdir)/rc.d

rcd_SCRIPTS = \
	$(distro_init_scripts) \
	init-functions

distro_init_scripts = \
	inst-rc_d.sh \
	ecbd

pkgrcddir = $(rcddir)

initdstartno = 11
initdstopno = 89

$(distro_init_scripts): Makefile
	rm -f $@ $@.tmp
	sed -e 's|[@]pkglibdir[@]|$(pkglibdir)|' \
	    -e 's|[@]sbindir[@]|$(sbindir)|' \
	    -e 's|[@]pkgdatadir[@]|$(pkgdatadir)|' \
	    -e 's|[@]pkgrcddir[@]|$(pkgrcddir)|' \
	    -e 's|[@]sysconfdir[@]|$(sysconfdir)|' \
	    -e 's|[@]initdstartno[@]|$(initdstartno)|' \
	    -e 's|[@]initdstopno[@]|$(initdstopno)|' \
	    '$(srcdir)/$@.in' > $@.tmp
	mv $@.tmp $@

inst-rc_d.sh: $(srcdir)/inst-rc_d.sh.in
ecbd: $(srcdir)/ecbd.in

CLEANFILES = \
	$(distro_init_scripts)

EXTRA_DIST = \
	inst-rc_d.sh.in \
	ecbd.in \
	init-functions

