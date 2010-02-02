Bacula - Windows Version Disclaimer
===================================

Please note, only the Win64 Client (File daemon) is supported, and
contrary to the Win32 Client, the other components (Director, Storage daemon,
their utilities) are not provided because they have not been ported.
Note: the Win64 Client can only be installed on 64 bit Windows Operating
systems.

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
Everything is installed in the directory 
"C:\Program Files\Bacula" unless a different directory was selected during
installation. Note: due to a bug in the NSIS installer we are using, on 
64 bit machines appears to install Bacula in "C:\Program Files (x86)\Bacula".

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
modern Windows machines, you can edit them with notepad, then choose UTF-8
output encoding before saving them.
