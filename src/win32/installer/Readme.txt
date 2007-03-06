Bacula - Windows Version Notes
==============================

These notes highlight how the Windows version of Bacula differs from the 
other versions.  It also provides any notes additional to the documentation.

For detailed documentation on using, configuring and troubleshooting Bacula,
please consult the installed documentation or the online documentation at
http://www.bacula.org/?page=documentation.


Start Menu Items
----------------
A number of menu items have been created in the Start menu under All Programs
in the Bacula submenu.  They may be selected to edit the configuration files,
view the documentation or run one of the console or utility programs.  The 
choices available will vary depending on the options you chose to install.


File Locations
--------------
The programs and documentation are installed in the directory 
"C:\Program Files\Bacula" unless a different directory was selected during
installation.  The configuration and other application data files are in the
"C:\Documents and Settings\All Users\Application Data\Bacula" directory.


Storage and Director Services
-----------------------------
These services are still considered experimental in this release.  Use of 
them should be approached with caution since they haven't received the
same level of extensive usage and testing that the File service has 
received.

Storage Device Names
--------------------
There is a utility installed called scsilist.exe which displays the installed 
devices, their physical address and their device name.  A link to it is 
created in the Bacula menu when the Storage service is installed.

Changer and Tape device names in Windows are Changer0, Changer1, etc and 
Tape0, Tape1, etc.  If there isn't a device driver loaded for the Changer 
then you need to use the address <Port>:<Bus>:<Target>:<Lun>.  Port is the 
SCSI Adapter Number, Bus is the Bus Number on the adapter (usually 0 since 
most adapters only have one bus), Target is the device's Target Device ID, 
Lun is the Logical Unit Number.
 
You must specify DeviceType = tape in the Device resource in bacula-sd.conf 
since auto detection of device type doesn't work at the present time.
