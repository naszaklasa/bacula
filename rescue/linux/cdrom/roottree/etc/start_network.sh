#! /bin/sh
#
# This script is not used by Bacula. You should try the script
#  in bacula-hostname/start_network.
# We leave this here because it could be useful if you are 
#  getting an IP address with DHCP.  You might need to change
#  dhcpcd below to dhclient if you are running RedHat
#

if ifconfig eth0 >/dev/null 2>&1 ; then

    echo "Setting up network..."
    /bin/dhcpcd $eth0

    echo "done"

    ifconfig
    exit

fi

echo "No network card present, cannot configure network"
sleep 3
