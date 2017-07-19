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

epsonbackendscriptdir = $(pkglibdir)/scripts
epsonbackendscript_SCRIPTS = inst-cups-post.sh

epson_backend_scripts = \
	inst-cups-post.sh

pkgcachedir = $(localstatedir)/cache/$(PACKAGE_TARNAME)
install_prog = $(bindir)/$(PACKAGE_TARNAME)-install

$(epson_backend_scripts): Makefile
	rm -f $@ $@.tmp
	sed -e 's|[@]pkglibdir[@]|$(pkglibdir)|' \
	    -e 's|[@]pkgdatadir[@]|$(pkgdatadir)|' \
	    -e 's|[@]pkgcachedir[@]|$(pkgcachedir)|' \
	    -e 's|[@]epsonbackendscriptdir[@]|$(epsonbackendscriptdir)|' \
	    -e 's|[@]PACKAGE_TARNAME[@]|$(PACKAGE_TARNAME)|' \
	    -e 's|[@]PACKAGE_STRING[@]|$(PACKAGE_STRING)|' \
	    -e 's|[@]install_prog[@]|$(install_prog)|' \
	    '$(srcdir)/$@.in' > $@.tmp
	mv $@.tmp $@

inst-cups-post.sh: $(srcdir)/inst-cups-post.sh.in

CLEANFILES = \
	$(epson_backend_scripts)

EXTRA_DIST = \
	$(epsonbackendscript_DATA) \
	inst-cups-post.sh.in
