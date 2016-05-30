#!/bin/sh

#
# Called from udev rule matching a serial radio.
#
# Uses DEVNAME, ACTION
#
if [ -z "$DEVNAME" ] ; then
    echo "no DEVNAME set, aborting"
    exit 1
fi
if [ -z "$ACTION" ] ; then
    echo "no ACTION set, aborting"
    exit 1
fi

PATH=/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin
export PATH

SHORTNAME=`basename $DEVNAME`
SERVICE=`dirname $DEVNAME`
ROUTER=/opt/netcontiki/sbin/border-router.native
WHERE=/var/db/border-router
PIDFILE=/var/run/br.${SHORTNAME}.pid
STATEFILE=/var/run/br.${SHORTNAME}.state
TUNFILE=/var/run/br.${SHORTNAME}.tundev
NEXT_TUNFILE=/var/run/br.nexttun
NEXT_TUNFILE_LOCK=/var/run/br.nexttun.lock
ADD=add
REMOVE=remove
SLEEPING=sleep
FAILED=failed
TUNDEV=tun${SHORTNAME#tty}

mkdir -p $WHERE/OSconfig
mkdir -p $WHERE/log

if [ -n "$BAUD" ] ; then
    USE_BAUD=$BAUD
else
    USE_BAUD=115200
fi
if [ -x /var/db/netcontiki/OSconfig/border-router.native ] ; then
    ROUTER=/var/db/netcontiki/OSconfig/border-router.native
fi

DATESTAMP=`date +%Y%m%d%H%M%S`
LOGFILE=${WHERE}/log/border-log-${SHORTNAME}-$DATESTAMP
exec >> $LOGFILE 2>&1

if [ "$ACTION" = "add" ] ; then
    echo $ADD > $STATEFILE

    cd $WHERE

    if [ -f $PIDFILE ] ; then

	if [ "$OTHER" = "$SLEEPING" ] ; then
	    # Already restarting, exit quietly
	    exit 0;
	fi
	OTHERPROC=`ps -ef | sed 's,^ *,,' | grep "^$OTHER"`
	if [ -n "$OTHERPROC" ] ; then
	    # Already running, exit quietly
	    exit 0;
	fi
    fi

    FAIL=0
    (while true ; do
	NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
	echo "------ $NOW Starting Border Router" >> $LOGFILE
	if [ -f "/var/db/netcontiki/OSconfig/braddress.${SHORTNAME#tty}" ]; then
	    BRADDR=`cat /var/db/netcontiki/OSconfig/braddress.${SHORTNAME#tty} 2> /dev/null`
	else
	    BRADDR="aaaa::1/64"
	fi
	STARTTIME=`date +%s`
	if [ "$SERVICE" = tcp ] ; then
	    REMOTEHOST=`echo $SHORTNAME | rev | cut -d: -f2- | rev | sed -e 's,\[,,' -e 's,\],,'`
	    REMOTEPORT=`echo $SHORTNAME | rev | cut -d: -f1 | rev`
	    if [ -f $TUNFILE ] ; then
		TUNDEV=`cat $TUNFILE`
	    else
		# mkdir is atomic, use it as synchronization
		while !mkdir $NEXT_TUNFILE_LOCK > /dev/null 2>&1; do
		    sleep 1
		done
		NT=`cat $NEXT_TUNFILE 2> /dev/null`
		if [ -z "$NT" ] ; then
		    NT=2
		fi
		TUNDEV=tun${NT}
		echo "$TUNDEV" > $TUNFILE
		NT=`expr $NT + 1`
		echo "$NT" > $NEXT_TUNFILE
		sleep 3
		rmdir $NEXT_TUNFILE_LOCK
	    fi
	    echo "Remote = \"${REMOTEHOST}\" port \"${REMOTEPORT}\"" >> $LOGFILE
	    echo "Starting with: $ROUTER -B${USE_BAUD} -s $DEVNAME -t $TUNDEV $BRADDR" >> $LOGFILE
	    ($ROUTER -a $REMOTEHOST -p $REMOTEPORT -t $TUNDEV $BRADDR >> $LOGFILE 2>&1) &
	else
	    echo "Starting with: $ROUTER -B${USE_BAUD} -s $DEVNAME -t $TUNDEV $BRADDR" >> $LOGFILE
	    ($ROUTER -B${USE_BAUD} -s $DEVNAME -t $TUNDEV $BRADDR >> $LOGFILE 2>&1) &
	fi
	echo $! > $PIDFILE
	NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
	echo "------ $NOW Border router started, pid `cat $PIDFILE`" >> $LOGFILE

	wait
	STATUS=$?

	NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
	echo "------ $NOW Border router exited with status  $STATUS" >> $LOGFILE
	STATE=`cat $STATEFILE`
	if [ "$STATE" = "$REMOVE" ] ; then
	    # Apparently terminated correctly, do not restart.
	    echo "------ $NOW Serial Radio removed, terminating." >> $LOGFILE
	    exit 0;
	fi
	echo $SLEEPING  > $PIDFILE
	NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
	echo "------ $NOW Router terminated, restarting soon." >> $LOGFILE
	ENDTIME=`date +%s`
	DURATION=`expr $ENDTIME - $STARTTIME`
	if [ $DURATION -lt 12 ] ; then
	    FAIL=`expr $FAIL + 1`
	else
	    FAIL=0
	fi
	#
	# If border router failed 20 times in a row, terminate.
	#  If it should run there will be an new "add" action.
	if [ $FAIL -gt 20 ] ; then
	    NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
	    echo "------ $NOW too many failures, terminating restart attempts." >> $LOGFILE
	    echo $REMOVE > $STATEFILE
	    if [ -r $PIDFILE ] ; then
		OLDPID=`cat $PIDFILE`
		if [ "$OLDPID" != "$SLEEPING" ] ; then
		    kill $OLDPID
		fi
		echo $FAILED > $PIDFILE
	    fi
	    break
	fi
	sleep 10
    done) &

elif [ "$ACTION" = "remove" ] ; then
    echo $REMOVE > $STATEFILE
    if [ -r $PIDFILE ] ; then
	OLDPID=`cat $PIDFILE`
	if [ "$OLDPID" != "$SLEEPING" ] ; then
	    kill $OLDPID
	fi
    fi

else
    NOW=`date "+%Y-%m-%d %H:%M:%S.000"`
    echo "------ $NOW Unknown action $ACTION , env:" >> $LOGFILE
    env >> $LOGFILE
    echo "------ " >> $LOGFILE
fi
