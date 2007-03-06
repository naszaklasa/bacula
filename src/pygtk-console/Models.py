# Models.py: Model classes
#
# Author: Lucas Di Pentima <lucas@lunix.com.ar>
#
# Version $Id: Models.py,v 1.48 2006/11/27 10:03:01 kerns Exp $
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

import gtk, gobject, pango
import bacula, parsers
import ConfigParser
from gtkmvc.model import Model
import sre

class Login (Model):
    application = None
    servers = []
    
    __properties__ = {
        'name' : '',
        'hostname' : '',
        'password' : '',
        'port' : '',
        'servers_loaded' : False,
        }

    def __init__(self, application):
        Model.__init__(self)
        self.application = application
        return

    def load_config(self):
        '''
        Load the saved data
        '''
        try:
            # Load last connected data
            last = self.application.config.get('global', 'last')
            self.name = self.application.config.get(last, 'name')
            self.hostname = self.application.config.get(last, 'hostname')
            self.password = self.application.config.get(last, 'password')
            self.port = self.application.config.get(last, 'port')
            # Populate the combobox
            for section in self.application.config.sections():
                if sre.search('server_', section):
                    self.servers.append(self.application.config.get(section,
                                                                    'name'))
            self.servers.sort()
            self.servers_loaded = True
        except ConfigParser.NoSectionError:
            pass
        return

    def select_server(self, server):
        try:
            section = 'server_' + server
            self.name = self.application.config.get(section, 'name')
            self.hostname = self.application.config.get(section, 'hostname')
            self.password = self.application.config.get(section, 'password')
            self.port = self.application.config.get(section, 'port')
        except ConfigParser.NoSectionError:
            pass
        return

    def authenticate(self):
        '''Try to connect to a bacula director daemon, and pass the
        connection to the application'''
        
        name = "*UserAgent*"
        password = bacula.get_md5(self.password)
        conn = bacula.bnet_connect (None, 5, 15, "Director daemon", self.hostname, None, self.port, 0)

        # Connection attempt
        bacula.bnet_fsend(conn, "Hello *UserAgent* calling\n")
        # Note: the last args on both next function calls are the new
        # 'compatible' argument on Bacula MD5 functions
        bacula.wrap_cram_md5_respond(conn, password, bacula.BNET_TLS_NONE, 1)
        bacula.cram_md5_challenge(conn, password, bacula.BNET_TLS_NONE, 1)

        if bacula.bnet_recv(conn) <= 0:
            return False
        else:
            self.application.login_connected(conn,
                                             self.name,
                                             self.hostname,
                                             self.password,
                                             self.port)
            return True
        pass

    def quit(self):
        '''Tell the application is going bye-bye'''
        self.application.quit()
        return
        
    pass # End of Login class

class Console (Model):
    __properties__ = {
        'output' : '',
        'messages' : '',
        'host' : '',
        'input' : '',
        'connected' : False,
        'running' : False,
        'auto_update' : False,
        'show_console': False,
        'show_stats' : False,
        'show_messages' : False,
        'show_log' : False,
        'at_prompt' : False,
        'history_index' : 0, # Pointer to cmd_history
        }
    cmd_history = [] # Saves all typed commands
    application = None
    director = None
    status_updater = None
    console_updater = None
    messages_updater = None
    
    def __init__(self, application, connection):
        Model.__init__(self)
        self.application = application
        self.director = Director(connection)
        self.director.gui_mode(True)
        self.status_updater = gobject.timeout_add(1000, self._update_connection_status)
        return

    def update(self):
        '''
        Update all data
        '''
        if self.connected and self.auto_update:
            self.director.update()
        return True

    def cmd_history_up(self):
        '''
        Places the history_index up position up
        '''
        if abs(self.history_index) < len(self.cmd_history):
            self.history_index -= 1
        return

    def cmd_history_down(self):
        '''
        Places the history_index up position up
        '''
        if abs(self.history_index) > 0:
            self.history_index += 1
        return

    def update_messages(self):
        '''
        Ask for pending messages
        '''
        if self.connected and self.auto_update:
            self.director.get_messages(self._cb_update_messages)
        return True # Keep being scheduled

    def _cb_update_messages(self, reply, data=None, at_prompt=False):
        '''
        Get the new messages
        '''
        if reply.strip() != '':
            self.messages = reply
        return

    def post_init(self):
        '''
        Initializations post-controller-creation
        '''
        self.host = self.director.hostname
        self.connected = self.director.is_connected()
        self.auto_update = True
        self.console_updater = gobject.timeout_add(20000, self.update)
        self.messages_updater = gobject.timeout_add(10000, self.update_messages)
        return

    def disconnect(self):
        '''Disconnect from Director and let know the application'''
        gobject.source_remove(self.status_updater)
        gobject.source_remove(self.console_updater)
        del self.director
        self.connected = False
        self.application.console_disconnected()
        return

    def _update_connection_status(self):
        self.connected = self.director.is_connected()
        self.running = self.director.is_running()
        return True
        
    def execute(self, command):
        '''Send a command to the director'''
        if self.director.is_connected():
            if (not self.at_prompt) and (command.strip() != ''):
                self.cmd_history.append(command)
                self.history_index = 0
            self.director.exec_cmd(command, self._cb_execute,
                                   priority=True, answer=self.at_prompt)
            self.at_prompt = False
        return

    def _cb_execute(self, reply, data=None, at_prompt=False):
        self.at_prompt = at_prompt
        self.output = reply
        return at_prompt

    def quit(self):
        self.application.quit()
        return

    pass # end of Console class


class Director(Model):
    '''
    This class represents the Bacula Director
    '''
    __properties__ = {
        'name' : '',
        'hostname' : '',
        'time' : '',
        'clients_nr' : 0,
        'running_jobs' : 0, # Number of jobs running
        'prompt_data' : None,
        'clientlist_updated' : False,
        'storagelist_updated': False,
        'joblist_updated' : False,
        'poollist_updated' : False,
        'last_command' : None,
        'last_reply' : None,
        'show_info' : False, # True when observers should show show_info_data
        }
    
    _connection = None
    _client_list = None
    _storage_list = None
    _job_list = None
    _pool_list = None
    prompt_callback = None
    prompt_cb_data = None
    running_jobs_updater = None # task id for elimination
    jobs_done = []
    show_info_data = (None, None) # Data to show to the user:(Summary, Details)
    
    def __init__(self, connection):
        Model.__init__(self)
        self._connection = DirectorConnection(connection, self)
        # Set some data
        self.update_jobs_list()
        # Set unchanging data
        self._client_list = ClientCollection(self._connection, self)
        self._storage_list = StorageCollection(self._connection, self)
        self._job_list = JobCollection(self._connection, self)
        self._pool_list = PoolCollection(self._connection, self)
        self.name = self._connection.name()
        self.hostname = self._connection.hostname()
        return

    def get_jobs_for_client(self, client):
        '''
        Return a list jobs for a client (wrapper)
        '''
        return self._job_list.get_jobs_for_client(client)

    def get_client(self, client_name):
        '''
        Retrieves the named client
        '''
        return self._client_list[client_name]

    def get_storage(self, storage_name):
        '''
        Retrieves the named storage
        '''
        return self._storage_list[storage_name]

    def update(self):
        '''
        Updates all its state data. This method is ment to be called
        periodically.
        '''
        self._client_list.update()
        self._storage_list.update()
        self.client_nr = len(self._client_list)
        self.name = self._connection.name()
        self.hostname = self._connection.hostname()
        self.clients_nr = len(self._client_list)
        self._connection.do_async('time', self._time_update_cb, priority=True)
        self.update_jobs_list()
        return

    def update_jobs_list(self):
        '''
        Updates the done jobs list
        '''
        self._connection.do_async('list jobs',
                                  self._cb_update_jobs_list,
                                  parsers.TableParser())
        return

    def _cb_update_jobs_list(self, reply, data, at_prompt):
        '''
        Updates the done jobs list (callback)
        '''
        self.jobs_done = reply
        self._running_jobs_updater()
        return

    def _running_jobs_updater(self):
        '''
        Updates the number of running jobs
        '''
        running = 0
        for job in self.jobs_done:
            # Running status
            # Waiting new volume to be mounted
            # Waiting for a mount
            # Waiting for maximum jobs
            # Waiting for higher priority job to finish
            if job['JobStatus'] == 'R' or job['JobStatus'] == 'm' or job['JobStatus'] == 'M' or job['JobStatus'] == 'd' or job['JobStatus'] == 'p':  
                running += 1
        self.running_jobs = running
        return

    def prompt(self, data, callback=None, cb_data=None, cb_parser=None):
        self.prompt_callback = callback
        self.prompt_cb_data = cb_data
        self.prompt_cb_parser = cb_parser
        # Must be the last
        self.prompt_data = parsers.DirectorPromptParser().parse(data)
        return

    def _time_update_cb(self, reply, data=None, at_prompt=False):
        self.time = reply.strip()
        return

    def get_clients(self):
        '''
        Returns the client collection
        '''
        return self._client_list

    def get_storages(self):
        '''
        Returns the storage collection
        '''
        return self._storage_list

    def get_jobs(self):
        '''
        Returns the job collection
        '''
        return self._job_list

    def get_pools(self):
        '''
        Returns the pool collection
        '''
        return self._pool_list

    def gui_mode(self, gui_on):
        '''
        Enables/disable gui mode
        '''
        if gui_on:
            self._execute('gui on', None)
        else:
            self._execute('gui off', None)
        return

    def get_messages(self, messages_cb):
        '''
        Ask for new messages and passes them to the callback
        '''
        self._execute('.messages', messages_cb)
        return

    def exec_cmd(self, command, callback, data=None,
                 priority=False, answer=False):
        '''
        A wrapper for the _execute method, meant to be used by the
        text console.
        '''
        self._execute(command, callback, None, data, priority, answer)
        return

    def _execute(self, command, callback, parser=None, data=None,
                 priority=False, answer=False):
        '''
        Execute some command on the connection and call some callback
        when the connection replies
        '''
        self._connection.do_async(command, callback, parser=parser, data=data,
                                  priority=priority, answer=answer)
        return

    def prompt_answer(self, answer):
        self._execute(answer,
                      callback=self.prompt_callback,
                      data=self.prompt_cb_data,
                      parser=self.prompt_cb_parser,
                      priority=True, answer=True)
        return

    def prompt_cancel(self):
        self._execute('.', callback=None, priority=True, answer=True)
        return

    def is_connected(self):
        '''
        Returns True if the connection is active
        '''
        return self._connection.is_connected()

    def is_running(self):
        '''
        Returns True if the director is running some command
        '''
        if self.is_connected():
            return self._connection.is_running()
        else:
            return False

    def do_full_restore(self, client):
        '''
        Do a full restore job on client
        '''
        self._connection.do_async("restore client=%s select current all done yes" % client.name,
                                  self._cb_do_full_restore,
                                  parsers.FullRestoreParser())
        return

    def _cb_do_full_restore(self, reply, data=None, at_prompt=False):
        # Do something...
        if reply:
            self.running_jobs += 1
        return

    def do_partial_restore(self, client, selector):
        '''
        Do a partial restore job on client
        '''
        self._connection.do_async("restore client=%s select current yes" % client.name,
                                  self._cb_do_partial_restore,
                                  parser=parsers.PartialRestoreParser(),
                                  data=selector)
        return

    def _cb_do_partial_restore(self, reply, data=None, at_prompt=False):
        selector = data
        
        if reply and at_prompt:
            self.prompt(reply,
                        callback=self._cb_do_partial_restore,
                        cb_parser=parsers.PartialRestoreParser(),
                        cb_data=selector)
        elif reply:
            # Setup info to be shown to the user
            self.show_info_data = ('Backup cannot be restored', reply)
            self.show_info = True
        else:
            selector.active = True
        return at_prompt

    def __del__(self):
        '''
        Destructor method: close the connection
        '''
        self._connection.close()
        return

    pass # End of Director class

class DirectorConnection:
    '''
    This class represents the connection with a Bacula Director daemon
    '''
    _connection = None
    _director = None
    _command_queue = []
    _running_command = False
    _at_prompt = False
    reply = ''

    def __init__(self, connection, director):
        self._connection = connection
        self._director = director
        bacula.bnet_set_nonblocking(self._connection)
        return

    def _cmd_sig(self, cmd_package):
        '''
        Generates a textual signature from a command package, that is,
        a group consisting of: command, callback, parser, and data.
        '''
        signature = ''
        [command, callback, parser, data] = cmd_package
        signature += command + ' '
        try:
            signature += callback.__name__ + ' '
        except AttributeError:
            signature += str(None) + ' '
        try:
            signature += parser.__class__.__name__ + ' '
        except AttributeError:
            signature += str(None) + ' '
        try:
            signature += str(data) + ' '
        except:
            signature += str(None) + ' '
        return signature        

    def do_async(self, command, callback, parser=None, data=None,
                 priority=False, answer=False):
        '''
        Enqueue a new command to the command list
        '''
        if self._at_prompt and not answer:
            print "Discarded command:", command
            return False # Command discarded

        cmd_package = [command, callback, parser, data]

        # Check for duplicated command
        for package in self._command_queue:
            if self._cmd_sig(cmd_package) == self._cmd_sig(package):
                print "Discarded duplicate command:", command
                return False # Command discarded
        
        if priority or answer:
            self._command_queue.insert(0, cmd_package)
            print "Enqueueing priority command:", command
        else:
            self._command_queue.append(cmd_package)
            print "Enqueueing command:", command, len(self._command_queue)
        self._process_pending_command(answer)
        return True # Command queued
        
    def _process_pending_command(self, answer=False):
        '''
        Execute a pending command if available
        '''
        if (not self._running_command or answer) and len(self._command_queue) > 0:
            print "Sending next pending command..."
            self._running_command = True
            [command, callback, parser, data] = self._command_queue.pop(0)
            gobject.io_add_watch(self._connection.fd, gobject.IO_IN,
                                 self._cb_async_read, callback, parser, data)
            # Execute the real command
            bacula.bnet_fsend(self._connection, command)
            self._director.last_command = command
            print "Command sent:", command
        return

    def _cb_async_read(self, fd, condition, callback, parser=None, data=None):
        '''
        Callback called to gather director response and call the user
        callback.
        '''
        stat = bacula.bnet_recv(self._connection)
        if stat >= 0:
            if self._connection.msg.strip() != 'You have messages.':
                self.reply += self._connection.msg
            return True # keep handling the event
        # All reply collected, let's do something with it...
        elif stat == bacula.BNET_SIGNAL:
            print "DATA ARRIVED!!"
            self._director.last_reply = self.reply

            if bacula.get_msglen(self._connection) == bacula.BNET_PROMPT:
                self._at_prompt = True
            else:
                self._at_prompt = False

            # A callback will do something with this reply
            if callback:
                print "Calling callback:", callback.func_name
                if parser:
                    will_answer = callback(parser.parse(self.reply),
                                           data, self._at_prompt)
                else:
                    will_answer = callback(self.reply, data, self._at_prompt)

                # If callback don't take care of answering, cancel with "."
                if self._at_prompt and not will_answer:
                    self._director.prompt(self.reply)
            
            # If there's no callback, cancel any prompt automatically
            elif self._at_prompt:
                print "Cancelling command..."
                self.do_async(".", None, answer=True)

            self.reply = ''
            if not self._at_prompt:
                self._running_command = False
                self._process_pending_command()
            return False # Don't keep handling this event
        else:
            print "NO SIGNAL!!!"
            return False
        pass

    def is_running(self):
        '''
        Returns true if a command is being run
        '''
        return self._running_command

    def close(self):
        '''
        Closes the connection
        '''
        bacula.bnet_fsend(self._connection, "quit")
        return

    def is_connected(self):
        '''
        Returns True if the connection is active
        '''
        return not bacula.is_bnet_stop(self._connection)

    def hostname(self):
        '''
        Returns the connections hostname
        '''
        return self._connection.host

    def name(self):
        '''
        Returns the directors name
        '''
        return self._connection.who

    pass # End of DirectorConnection class

class Client(Model):
    '''
    This class represents a bacula client (FD)
    '''
    __properties__ = {
        'online' : False,
        'jobs_done' : [],
        'jobs_running' : [],
        'version' : '',
        'address' : '',
        'port' : '',
        }

    name = ''
    _connection = None
    _director = None
    
    def __init__(self, name, connection, director):
        Model.__init__(self)
        self._connection = connection
        self._director = director
        self.name = name
        self.get_defaults()
        self.update()
        return

    def get_jobs_done(self):
        '''
        Return a list of hashes with data about jobs done on client
        '''
        jobs_done = []
        my_jobs = self._director.get_jobs_for_client(self)
        for my_job in my_jobs:
            for job_done in self._director.jobs_done:
                if job_done['Name'] == my_job.name:
                    jobs_done.append(job_done)
        return jobs_done

    def get_defaults(self):
        '''
        Ask for default data to the director
        '''
        self._connection.do_async('.defaults client=%s' % self.name,
                                  self._cb_get_defaults,
                                  parsers.ClientDefaultsParser())
        return

    def _cb_get_defaults(self, reply, data=None, at_prompt=False):
        '''
        Set unchanging data to client
        '''
        if reply:
            self.address = reply['address']
            self.port = reply['fdport']
        return

    def update(self, notice_cb=None):
        '''
        Updates client status
        '''
        self._update_jobs_done()
        self._connection.do_async('status client=%s' % self.name,
                                  self._cb_update,
                                  parsers.ClientStatusParser(),
                                  notice_cb)
        return

    def _update_jobs_done(self):
        '''
        Updates jobs done list
        '''
        self.jobs_done = self.get_jobs_done()
        return

    def _cb_update(self, reply, callback=None, at_prompt=False):
        self.online = reply['online']
        self.version = reply['version']
        # Running jobs
        self.jobs_running = reply['running_jobs']
        # Notice callback call
        if callback:
            callback()
        return

    pass # End of Client class


class ClientCollection(dict):
    '''
    This class represents a group of clients (FD) on a given director
    '''
    _connection = None
    _director = None

    def __init__(self, connection, director):
        dict.__init__(self)
        self._connection = connection
        self._director = director
        self._connection.do_async('.clients', self._cb_get_clients, None)
        return

    def update(self, notice_cb=None):
        '''
        Updates all clients, and passes the notice callback to update
        whatever was calling this method
        '''
        for client in self.itervalues():
            client.update(notice_cb)
            # Trigger director's observers
            self._director.clientlist_updated = True
        return
    
    def _cb_get_clients(self, reply, data=None, at_prompt=False):
        for c in reply.split("\n"):
            if c != '':
                self[c] = Client(c, self._connection, self._director)
                pass
            pass
        # Trigger director's observers
        self._director.clientlist_updated = True
        return

    def status(self):
        '''
        Return a matrix with the clients status data
        '''
        matrix = []
        for client in self.itervalues():
            matrix.append([client.name, client.online])
        return matrix
    
    pass # End of ClientCollection class

class StorageCollection(dict):
    '''
    This class represents a group of storage (SD) on a given director
    '''
    _connection = None
    _director = None

    def __init__(self, connection, director):
        dict.__init__(self)
        self._connection = connection
        self._director = director
        self._connection.do_async('.storage', self._cb_get_storage, None)
        return

    def update(self, notice_cb=None):
        '''
        Updates all storage, and passes the notice callback to update
        whatever was calling this method
        '''
        for storage in self.itervalues():
            storage.update(notice_cb)
        return
    
    def _cb_get_storage(self, reply, data=None, at_prompt=False):
        for s in reply.split("\n"):
            if s != '':
                self[s] = Storage(s, self._connection)
                pass
            pass
        # Trigger director's observers
        self._director.storagelist_updated = True
        return

    def status(self):
        '''
        Return a matrix with the storage status data
        '''
        matrix = []
        for storage in self.itervalues():
            matrix.append([storage.name, storage.online])
        return matrix
    
    pass # End of StorageCollection class

class Storage(Model):
    '''
    This class represents a bacula storage daemon (SD)
    '''
    __properties__ = {
        'online' : False,
        'enabled' : False,
        'name' : ''
        }

    version = ''
    address = ''
    media_type = ''
    port = ''
    device = ''
    _connection = None
    
    def __init__(self, name, connection):
        Model.__init__(self)
        self._connection = connection
        self.name = name
        self.get_defaults()
        return

    def get_defaults(self):
        '''
        Set its unchanging data from the director
        '''
        self._connection.do_async('.defaults storage=%s' % self.name,
                                  self._cb_get_defaults,
                                  parsers.StorageDefaultsParser())
        return

    def _cb_get_defaults(self, reply, data=None, at_prompt=False):
        '''
        Set unchanging properties
        '''
        if reply:
            self.address = reply['address']
            self.media_type = reply['media_type']
            self.port = reply['sdport']
            self.device = reply['device']
            self.enabled = (int(reply['enabled']) == 1)
        return

    def update(self, notice_cb=None):
        '''
        Updates storage status
        '''
        self._connection.do_async('status storage=%s' % self.name,
                                  self._cb_update,
                                  parsers.StorageStatusParser(),
                                  notice_cb)
        return

    def _cb_update(self, reply, callback=None, at_prompt=False):
        self.online = reply['online']
        self.version = reply['version']
        if callback:
            callback()
        return

    pass # End of Storage class

class JobCollection(dict):
    '''
    A collection of backup jobs on a given director
    '''
    _connection = None
    _director = None

    def __init__(self, connection, director):
        dict.__init__(self)
        self._connection = connection
        self._director = director
        self._connection.do_async('.jobs', self._cb_get_jobs, None)
        return

    def _cb_get_jobs(self, reply, data=None, at_prompt=False):
        for job in reply.split("\n"):
            if job != '':
                self[job] = Job(job, self._connection, self._director)
                pass
            pass
        # Trigger director's observers
        self._director.joblist_updated = True
        return

    def get_jobs_for_client(self, client):
        '''
        Return a list of jobs related to the passed client
        '''
        jobs = []
        for job in self.values():
            if job.client == client:
                jobs.append(job)
        return jobs
    
    pass # End of JobCollection class

class Job(Model):
    '''
    This class represent a bacula backup job
    '''
    _connection = None
    name = ''
    pool = None
    client = None
    storage = None
    job_type = None
    level = None
    fileset = None
    enabled = False
    
    def __init__(self, name, connection, director):
        Model.__init__(self)
        self._connection = connection
        self._director = director
        self.name = name
        self.get_defaults()
        return

    def get_defaults(self):
        '''
        Set its unchanging data from the director
        '''
        self._connection.do_async('.defaults job=%s' % self.name,
                                  self._cb_get_defaults,
                                  parsers.JobDefaultsParser())
        return

    def _cb_get_defaults(self, reply, data=None, at_prompt=False):
        '''
        Set unchanging properties
        '''
        if reply:
            self.pool = reply['pool']
            self.client = self._director.get_client(reply['client'])
            self.storage = self._director.get_storage(reply['storage'])
            self.job_type = reply['job_type']
            self.level = reply['level']
            self.fileset = reply['fileset']
            self.enabled = (int(reply['enabled']) == 1)
        return
    
    pass # End of Job class

class Pool(Model):
    '''
    This class represent a bacula pool
    '''
    _connection = None
    name = ''
    pool_type = ''
    label_format = ''
    use_volume_once = 0
    accept_any_volume = 0
    purge_oldest_volume = 0
    recycle_oldest_volume = 0
    recycle_current_volume = 0
    max_volumes = 0
    vol_retention = 0
    vol_use_duration = 0
    max_vol_jobs = 0
    max_vol_files = 0
    max_vol_bytes = 0
    auto_prune = 0
    recycle = 0

    def __init__(self, name, connection):
        Model.__init__(self)
        self._connection = connection
        self.name = name
        self.get_defaults()
        return

    def get_defaults(self):
        '''
        Set its unchanging data from the director
        '''
        self._connection.do_async('.defaults pool=%s' % self.name,
                                  self._cb_get_defaults,
                                  parsers.PoolDefaultsParser())
        return

    def _cb_get_defaults(self, reply, data=None, at_prompt=False):
        '''
        Set unchanging properties
        '''
        if reply:
            self.pool_type = reply['pool_type']
            self.label_format = reply['label_format']
            self.use_volume_once = reply['use_volume_once']
            self.accept_any_volume = reply['accept_any_volume']
            self.purge_oldest_volume = reply['purge_oldest_volume']
            self.recycle_oldest_volume = reply['recycle_oldest_volume']
            self.recycle_current_volume = reply['recycle_current_volume']
            self.max_volumes = reply['max_volumes']
            self.vol_retention = reply['vol_retention']
            self.vol_use_duration = reply['vol_use_duration']
            self.max_vol_jobs = reply['max_vol_jobs']
            self.max_vol_files = reply['max_vol_files']
            self.max_vol_bytes = reply['max_vol_bytes']
            self.auto_prune = reply['auto_prune']
            self.recycle = reply['recycle']
        return

    pass # End of Pool class

class PoolCollection(dict):
    '''
    A collection of pools on a given director
    '''
    _connection = None
    _director = None

    def __init__(self, connection, director):
        dict.__init__(self)
        self._connection = connection
        self._director = director
        self._connection.do_async('.pools', self._cb_get_pools, None)
        return

    def _cb_get_pools(self, reply, data=None, at_prompt=False):
        for pool in reply.split("\n"):
            if pool != '':
                self[pool] = Pool(pool, self._connection)
                pass
            pass
        # Trigger director's observers
        self._director.poollist_updated = True
        return
    
    pass # End of PoolCollection class

class FileSelector(Model):
    '''
    File selector class, to be used in partial restore operations
    '''
    __properties__ = {
        'active' : False,
        'cwd_path' : None,
        'cwd_contents' : None,
        'marked_message' : None,
        'last_estimation' : None,
        'allow_input' : True,
        'search_result' : None,
        }
    
    _director = None
    _visited = {} # Paths already visited
    
    def __init__(self, director):
        Model.__init__(self)
        self._director = director
        return

    def get_list(self):
        '''
        Ask the director the file list on the current working dir
        '''
        self.allow_input = False
        self._director._execute('.dir',
                                callback=self._cb_get_list,
                                parser=parsers.GetDirContentParser(),
                                data=None, priority=True, answer=True)
        return

    def _cb_get_list(self, reply, data=None, at_prompt=False):
        '''
        Callback to update the current working dir contents
        '''
        self.allow_input = True
        contents = reply
        if isinstance(reply, list):
            self.cwd_contents = contents
            self._visited[self.cwd_path] = self.cwd_contents
        return at_prompt

    def cached(self, path):
        '''
        Return true if given path is cached
        '''
        return path in self._visited.keys()

    def change_dir(self, directory):
        '''
        Ask the director to change to some directory
        '''
        self.allow_input = False
        self._director._execute('cd %s' % directory,
                                callback=self._cb_change_dir,
                                parser=parsers.ChangeDirParser(),
                                data=None, priority=True, answer=True)
        return

    def _cb_change_dir(self, reply, data=None, at_prompt=False):
        self.allow_input = True
        current_dir = reply
        if current_dir:
            self.cwd_path = current_dir
        return at_prompt

    def reset(self):
        self._visited = {}
        return

    def cancel_operation(self):
        '''
        Send to the director the cancel command
        '''
        self.active = False
        self.reset()
        self._director._execute('quit',
                                callback=None, parser=None, data=None,
                                priority=True, answer=True)
        return

    def confirm_operation(self):
        '''
        Send to the director the done command
        '''
        self.active = False
        self.reset()
        self._director._execute('done',
                                callback=None, parser=None, data=None,
                                priority=True, answer=True)
        return
    
    def mark(self, filename, restore):
        '''
        Tell the director to mark the specified file
        '''
        self.allow_input = False
        if filename[-1:] == '/':
            filename = filename[:-1]
        if restore:
            command = 'mark'
        else:
            command = 'unmark'
        self._director._execute(command + ' ' + filename,
                                callback=self._cb_mark,
                                parser=parsers.MarkFileParser(), data=None,
                                priority=True, answer=True)
        return

    def _cb_mark(self, reply, data=None, at_prompt=False):
        '''
        Updates marked files count
        '''
        self.marked_message = reply
        self._director._execute('estimate',
                                callback=self._cb_estimate,
                                parser=parsers.EstimateParser(),
                                data=None, priority=True, answer=True)
        return at_prompt

    def _cb_estimate(self, reply, data=None, at_prompt=False):
        '''
        Updates estimation
        '''
        self.allow_input = True
        self.last_estimation = reply
        return at_prompt

    def search(self, search_term):
        '''
        Execute the find command on the director
        '''
        self.allow_input = False
        self._director._execute('find %s' % search_term,
                                callback=self._cb_search,
                                parser=parsers.SearchResultParser(),
                                data=None, priority=True, answer=True)
        return

    def _cb_search(self, reply, data=None, at_prompt=False):
        '''
        Update the search_result attribute
        '''
        self.allow_input = True
        self.search_result = reply
        return at_prompt

    pass # End of FileSelector class
