#!/usr/bin/python

# pygtk-console.py: Python GTK Console
#
# Author: Lucas Di Pentima <lucas@lunix.com.ar>
#
# Version $Id: pygtk-console.py,v 1.10 2006/11/27 10:03:01 kerns Exp $
#
#
# This file was contributed to the Bacula project by Lucas Di
# Pentima and LUNIX S.R.L.
#
# LUNIX S.R.L. has been granted a perpetual, worldwide,
# non-exclusive, no-charge, royalty-free, irrevocable copyright
# license to reproduce, prepare derivative works of, publicly
# display, publicly perform, sublicense, and distribute the original
# work contributed by LUNIX S.R.L. and its employees to the Bacula
# project in source or object form.
#
#
#  Bacula® - The Network Backup Solution
#
#  Copyright (C) 2005-2006 Free Software Foundation Europe e.V.
#
#  The main author of Bacula is Kern Sibbald, with contributions from
#  many others, a complete list can be found in the file AUTHORS.
#  This program is Free Software; you can redistribute it and/or
#  modify it under the terms of version two of the GNU General Public
#  License as published by the Free Software Foundation plus additions
#  that are listed in the file LICENSE.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#  02110-1301, USA.
#
#  Bacula® is a registered trademark of John Walker.
#  The licensor of Bacula is the Free Software Foundation Europe
#  (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
#  Switzerland, email:ftf@fsfeurope.org.


import sys, os
import gtk
import bacula, parsers
import ConfigParser
from Models import *
from Controllers import *
from Views import *

class Singleton:
    '''
    Abstract class to implement Singleton pattern
    '''
    __single = None
    def __init__(self):
        if Singleton.__single:
            raise Singleton.__single
        Singleton.__single = self
    pass # End of Singleton class
    
class Application(Singleton):
    login_model = None
    login_view = None
    login_controller = None
    console_model = None
    console_view = None
    console_controller = None
    config_file = None
    config = None
    
    def __init__(self):
        '''Application started: show a login screen'''
        Singleton.__init__(self)
        self.config_file = os.path.expanduser('~/.bacula-pygtk-console.cfg')
        self._load_config()
        self._create_login()
        gtk.main()
        return

    def _load_config(self):
        '''
        Loads the configuration
        '''
        self.config = ConfigParser.ConfigParser()
        self.config.read(self.config_file)
        return

    def _write_config(self):
        '''
        Write the config data
        '''
        self.config.write(open(self.config_file, 'w'))
        os.chmod(self.config_file, 0600)
        return

    def _create_login(self):
        '''Set-up and show the login dialog'''
        self.login_model = Login(self)
        self.login_controller = LoginController(self.login_model)
        self.login_view = LoginView(self.login_controller)
        return

    def _destroy_login(self):
        '''Eliminate the login dialog'''
        self.login_controller.destroy()
        self.login_model = None
        self.login_controller = None
        self.login_view = None
        return

    def login_connected(self, connection, name, hostname, password, port):
        '''Application connected, close the login screen and
        show the console window'''
        host_tag = "server_%s" % name
        if not self.config.has_section(host_tag):
            self.config.add_section(host_tag)
        if not self.config.has_section('global'):
            self.config.add_section('global')
        self.config.set(host_tag, 'name', name)
        self.config.set(host_tag, 'hostname', hostname)
        self.config.set(host_tag, 'password', password)
        self.config.set(host_tag, 'port', port)
        self.config.set('global', 'last', host_tag)
        self._destroy_login()
        self._create_console(connection)
        return

    def console_disconnected(self):
        '''Console disconnected: re-open the login screen'''
        self._destroy_console()
        self._create_login()
        return

    def _create_console(self, connection):
        '''Set-up and show the console window'''
        self.console_model = Console(self, connection)
        self.console_controller = ConsoleController(self.console_model)
        self.console_view = ConsoleView(self.console_controller)
        return

    def _destroy_console(self):
        '''Eliminate the console window'''
        self.console_controller.destroy()
        self.console_model = None
        self.console_controller = None
        self.console_view = None
        return

    def quit(self):
        '''
        Time to go, quit the application...goodbye!
        '''
        self._write_config()
        gtk.main_quit()
        sys.exit(0)        
        pass
    
    pass # end of Application class


# If called like an application, load it!
if __name__ == '__main__':
    a = Application()
