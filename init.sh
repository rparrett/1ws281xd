#!/bin/sh

# Quick start-stop-daemon example, derived from Debian /etc/init.d/ssh
set -e

# Must be a valid filename
NAME=led-daemon
PIDFILE=/var/run/$NAME.pid
#This is the command to be run, give the full pathname
DAEMON=/root/led-daemon
DAEMON_OPTS="--fifo=/root/led-fifo"

export PATH="${PATH:+$PATH:}/usr/sbin:/sbin"

case "$1" in
  start)
        echo -n "Starting daemon: "$NAME
	start-stop-daemon --start --background --quiet --make-pidfile --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
        echo "."
	;;
  stop)
        echo -n "Stopping daemon: "$NAME
	start-stop-daemon --stop --background --quiet --oknodo --make-pidfile --pidfile $PIDFILE
        echo "."
	;;
  restart)
        echo -n "Restarting daemon: "$NAME
	start-stop-daemon --stop --background --quiet --oknodo --retry 30 --make-pidfile --pidfile $PIDFILE
	start-stop-daemon --start --background --quiet --make-pidfile --pidfile $PIDFILE --exec $DAEMON -- $DAEMON_OPTS
	echo "."
	;;

  *)
	echo "Usage: "$1" {start|stop|restart}"
	exit 1
esac

exit 0
