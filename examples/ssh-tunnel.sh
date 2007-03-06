#!/bin/sh
# script for creating / stopping a ssh-tunnel to a backupclient
# Stephan Holl<sholl@gmx.net>
#
#

# variables
USER=xxxx
CLIENT=domain.com
CLIENT_PORT=9112
LOCAL=192.168.2.4
LOCAL_PORT=$CLIENT_PORT
SSH=/usr/bin/ssh


case "$1" in
 start)
    # create ssh-tunnel 

    echo "Start SSH-tunnel to $CLIENT..."
    $SSH -vfnCNg2 -o PreferredAuthentications=publickey -i /var/lib/bacula/.ssh/id_dsa -l $USER -L $CLIENT_PORT:$CLIENT:$LOCAL_PORT -R 9101:$LOCAL:9101 -R 9103:$LOCAL:9103 $CLIENT  
    exit 0
    ;;

 stop)
    # remove tunnel 
    echo "Stop SSH-tunnel to $CLIENT..."

    # find PID killem
    PID=`ps ax|grep "/usr/bin/ssh -vfnCNg2 -o PreferredAuthentications=publickey -i /var/lib/bacula/.ssh/id_dsa -l $USER -L $CLIENT_PORT:$CLIENT:$LOCAL_PORT -R 9101:$LOCAL:9101 -R 9103:$LOCAL:9103 $CLIENT &"|cut -d" " -f1`
    kill $PID
    exit 0
    ;;
 *)
    #  usage:
    echo "             "
    echo "      Start SSH-tunnel to client-host"
    echo "      to bacula-director and storage-daemon"
    echo "            "
    echo "      USAGE:"
    echo "      ssh-tunnel.sh {start|stop}"
    echo "                            "
    exit 1
    ;;
esac

        
