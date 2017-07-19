#!/bin/sh
# 
# Copyright (C) Seiko Epson Corporation 2014.
# 
#  This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 

pkglibdir=/usr/lib/epson-backend
pkgdatadir=/usr/share/epson-backend
pkgcachedir=/var/cache/epson-backend

install_prog=/usr/bin/epson-backend-install
pkg_tarname=epson-backend

PRINTER_MODEL_LOW=lite
PRINTER_MODEL=`echo $PRINTER_MODEL_LOW | tr [:lower:] [:upper:]`

daemon=ecbd
default_option="--default"

MKDIR="mkdir -p"
RM="rm -f"

RC_LOCAL=/etc/rc.d/rc.local
ECBD_SERVICE='service ecbd start'

case "$1" in
    install)

        if [ -e $RC_LOCAL ]; then
            ECBD=`grep ecbd $RC_LOCAL`	
            if [ -z "$ECBD" ]; then
    	         echo $ECBD_SERVICE>> $RC_LOCAL
            fi
        fi
	KEY=`grep 35587 < /etc/services`
	if [ -z "$KEY" ]; then
	    if [ -s /etc/services ]; then
		cp /etc/services /etc/services.bak
	    fi
	    cat <<EOF >>/etc/services
# written for $pkg_tarname
cbtd     35587/tcp
# $pkg_tarname  end
EOF
	    
	    echo "Install Message > Described entry of $PRINTER_MODEL in services."
	    echo "Install Message > Backup file is /etc/services.bak."
	fi

#
# make cache dir
#
	if test ! -d "$pkgcachedir" ; then
	    $MKDIR "$pkgcachedir"
	fi

# delete daemon process
#
	daemon_pslist=`ps -C $daemon -o pid,args | grep -e "$pkglibdir/$daemon"`
	if test "0" = "$?" ; then
	    # daemon process exist
	    while read line ; do
		daemon_pid=`echo $line | awk '{ print $1 }'`
		daemon_option=`echo $line | awk '{ print $3 }'`
		daemon_pidfile=`echo $line | awk '{ print $4 }'`
		if test "x-p" = "x$daemon_option" -a "x" != "x$daemon_pidfile" ; then
		    # pid file exist
		    daemon_savedpid=`cat $daemon_pidfile`
		    if test "x$daemon_pid" != "x$daemon_savedpid" ; then
			echo "fatal error! pid in $daemon_pidfile unmatched." >&2
			exit 1
		    fi
		else
		    # pid file does not exist
		    if test -r "/var/run/$daemon.pid" ; then
			daemon_savedpid=`cat /var/run/$daemon.pid`
			if test "x$daemon_pid" != "x$daemon_savedpid" ; then
			    kill -TERM $daemon_pid
			fi
		    else
			kill -TERM $daemon_pid
		    fi
		fi
	    done <<EOF
$daemon_pslist
EOF
	fi
#
# rcfile install
#
#	$pkglibdir/rc.d/inst-rc_d.sh install
	/usr/lib/epson-backend/rc.d/inst-rc_d.sh install
	
	echo "Install Message > Start $pkglibdir/setup to change setup."
	
# printer start
	$pkglibdir/rc.d/$daemon start 2>&1 1>/dev/null
	;;

    deinstall)
        if [ -e $RC_LOCAL ];then
            cp $RC_LOCAL $RC_LOCAL.bak
            sed -e "/\<$ECBD_SERVICE\>/d" < $RC_LOCAL.bak > $RC_LOCAL
            rm $RC_LOCAL.bak
	fi

	if [ -s /etc/services ]; then
	    KEY=`grep "\# written for $pkg_tarname" /etc/services`
	    if [ -n "$KEY" ]; then
		mv /etc/services /etc/services.bak
		sed -e "/# written for $pkg_tarname/,/# $pkg_tarname  end/d" < /etc/services.bak > /etc/services
	    fi
	fi

# printer stop
	$pkglibdir/rc.d/$daemon stop 2>&1 1>/dev/null

#	$pkglibdir/rc.d/inst-rc_d.sh deinstall
       /usr/lib/epson-backend/rc.d/inst-rc_d.sh deinstall

	;;
    *)
	echo "Usage: install_post.sh { install | deinstall }" >&2
	exit 1
	;;
esac

