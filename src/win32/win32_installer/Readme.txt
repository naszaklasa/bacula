Bacula - Windows Version Disclaimer
===================================

Please note, only the Win32 Client (File daemon) is supported.  All the
other components (Director, Storage daemon, their utilities) are provided
on an "as is" basis.  Unfortunately, they are neither properly tested,   
documented, or supported.  This means that we cannot ensure that bug reports
against the non-supported components will be fixed.  For them to be supported, 
we need three things from the Open Source community:

1. Full documentation of the Windows particularities of the Director,
   the Storage daemon, and their utilities in the Bacula manual.

2. Someone to periodically and on demand run the regressions tests.

3. One or more developers who are willing to accept and correct Windows
   related bugs as they occur.

4. A certain Win32 community that will respond to user support questions
   on the bacula-users list.  (This is probably already fullfilled). 
             

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

Code Page Problems
-------------------
Please note that Bacula expects the contents of the configuration files to be 
written in UTF-8 format. Some translations of "Application Data" have accented
characters, and apparently the installer writes this translated data in the
standard Windows code page coding.  This occurs for the Working Directory, and 
when it happens the daemon will not start since Bacula cannot find the directory.
The workaround is to manually edit the appropriate conf file and ensure that it
is written out in UTF-8 format.

The conf files can be edited with any UTF-8 compatible editor, or on most 
modern Win32 machines, you can edit them with notepad, then choose UTF-8
output encoding before saving them.


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
