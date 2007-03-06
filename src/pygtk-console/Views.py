# Views.py: View classes
#
# Author: Lucas Di Pentima <lucas@lunix.com.ar>
#
# Version $Id: Views.py,v 1.47 2006/11/27 10:03:01 kerns Exp $
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
from gtkmvc.view import View

class LoginView (View):
    def __init__(self, controller):
        View.__init__(self, controller, 'pygtk-console.glade', 'login-dialog',
                      None, False)
        self._init_combobox()
        controller.registerView(self)
        return

    def _init_combobox(self):
        '''
        Server list combobox initialization
        '''
        cell = gtk.CellRendererText()
        self['combobox-servers'].set_model(gtk.ListStore(str))
        self['combobox-servers'].pack_start(cell, True)
        self['combobox-servers'].add_attribute(cell, 'text', 0)
        return

    def __getattr__(self, attr):
        '''get/set accesors abstraction'''

        if attr == 'name':
            return self['login-entry-name'].get_text()
        elif attr == 'hostname':
            return self['login-entry-hostname'].get_text()
        elif attr == 'password':
            return self['login-entry-password'].get_text()
        elif attr == 'port':
            return int(self['login-entry-port'].get_text())
        else:
            raise 'ERROR: \'%s\' not defined as attribute' % attr

    def update_name_entry(self, name):
        '''Updates director\'s name entry widget'''
        self['login-entry-name'].set_text(name)
        return

    def update_hostname_entry(self, hostname):
        '''Updates director\'s hostname entry widget'''
        self['login-entry-hostname'].set_text(hostname)
        return

    def update_password_entry(self, password):
        '''Update director\'s password entry widget'''
        self['login-entry-password'].set_text(password)
        return

    def update_port_entry(self, port):
        '''Updates director\'s port number entry widget'''
        self['login-entry-port'].set_text(str(port))
        return

    def populate_combobox(self, servers, default):
        '''
        Add server names to combobox and select default
        '''
        for server in servers:
            self['combobox-servers'].append_text(server)
        self['combobox-servers'].set_active(default)
        return

    def show(self):
        '''Show the UI'''
        self['login-dialog'].show_all()
        return

    pass

class ConsoleView (View):
    tooltips = None
    command_tag = None
    reply_tag = None
    
    def __init__(self, controller):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'console-window',
                      None, True)
        self.tooltips = gtk.Tooltips()
        self.tooltips.enable()
        # Setup font type
        font_desc = pango.FontDescription('monospace 10')
        self['output-textview'].modify_font(font_desc)
        self['messages-textview'].modify_font(font_desc)
        # Setup color tags for text
        tag_table = self['output-textview'].get_buffer().get_tag_table()
        self.command_tag = gtk.TextTag("command")
        self.command_tag.set_property("foreground", "firebrick")
        tag_table.add(self.command_tag)
        self.reply_tag = gtk.TextTag("reply")
        self.reply_tag.set_property("foreground", "SteelBlue4")
        tag_table.add(self.reply_tag)
        # Static pages popup entries
        menu_lbl = gtk.Label('Console')
        menu_lbl.set_alignment(0,0)
        self['notebook'].set_menu_label(self['console-vbox'], menu_lbl)
        menu_lbl = gtk.Label('Status')
        menu_lbl.set_alignment(0,0)
        self['notebook'].set_menu_label(self['status-vbox'], menu_lbl)
        menu_lbl = gtk.Label('Messages')
        menu_lbl.set_alignment(0,0)
        self['notebook'].set_menu_label(self['messages-vbox'], menu_lbl)
        return

    def __getattr__(self, attr):
        if attr == 'input':
            return self['input-entry'].get_text().strip()
        else:
            raise "ERROR: '%s' is not an attribute" % attr
        pass

    def retrieve_command(self, command):
        '''
        Show command on console input widget
        '''
        self['input-entry'].set_text(command)
        return

    def reset_prompt(self):
        '''Empty the prompt widget'''
        self['input-entry'].set_text('')
        return

    def _insert_text(self, text, tag):
        buffer = self['output-textview'].get_buffer()
        iter = buffer.get_end_iter()
        buffer.insert_with_tags(iter, text, tag)
        gobject.idle_add(self._scroll_to_end, 'output-textview')
        return

    def update_output_command(self, output):
        '''
        Update command text with color tag
        '''
        self._insert_text(output, self.command_tag)
        return

    def update_output_reply(self, output):
        '''
        Update reply text with color tag
        '''
        self._insert_text(output, self.reply_tag)
        return

    def update_messages(self, output):
        '''
        Fill the messages widget with the new content
        '''
        buffer = self['messages-textview'].get_buffer()
        iter = buffer.get_end_iter()
        buffer.insert(iter, output)
        gobject.idle_add(self._scroll_to_end, 'messages-textview')
        # Mark label to notify user if not active
        if self['notebook'].get_current_page() != 2: # Message log
            self['messages-tab-label'].set_markup('<span foreground="red"><b>Messages</b></span>')
        return

    def update_director_name(self, dir_hostname):
        '''
        Update console window title with the connected director hostname
        '''
        self['console-window'].set_title("%s - Bacula Administrator" % dir_hostname)
        return

    def set_focus_commandline(self):
        '''
        Set the focus on the input-entry widget
        '''
        gobject.idle_add(self['input-entry'].grab_focus)
        return

    def reset_messages_tab(self):
        '''
        Reset messages tab label to normal
        '''
        self['messages-tab-label'].set_markup('Messages')
        return

    def update_connection_status(self, connected):
        if connected:
            self['connected-label'].set_text("Online")
        else:
            self['connected-label'].set_text("Offline")
        self['input-entry'].set_sensitive(connected)
        return

    def update_running_status(self, running=False, at_prompt=False):
        if running and at_prompt:
            self['running-label'].set_text("Waiting")
            self['command-label'].set_text(">>>")
        elif running:
            self['running-label'].set_text("Busy")
            self['command-label'].set_text("Command:")
        else:
            self['running-label'].set_text("Idle")
            self['command-label'].set_text("Command:")
    
    def _scroll_to_end (self, widget_name):
        text = self[widget_name]
        iter = text.get_buffer().get_end_iter()
        text.scroll_to_iter(iter, 0, 1)
        return False  # don't requeue this handler

    def remove_page(self, widget, child):
        self['notebook'].remove_page(self['notebook'].page_num(child))
        return

    def clean_messages(self):
        '''
        Clean the message log widget
        '''
        buffer = self['messages-textview'].get_buffer()
        buffer.set_text('', 0)
        return

    def show_console(self, show):
        if show:
            self['console-vbox'].show_all()
            self['notebook'].set_current_page(self['notebook'].page_num(self['console-vbox']))
        else:
            self['console-vbox'].hide()
        return

    def show_stats(self, show):
        if show:
            self['status-vbox'].show_all()
            self['notebook'].set_current_page(self['notebook'].page_num(self['status-vbox']))
        else:
            self['status-vbox'].hide()
        return

    def show_messages(self, show):
        if show:
            self['messages-vbox'].show_all()
            self['notebook'].set_current_page(self['notebook'].page_num(self['messages-vbox']))
        else:
            self['messages-vbox'].hide()
        return

    def show_log(self, show):
        if show:
            self['log-vbox'].show_all()
            self['notebook'].set_current_page(self['notebook'].page_num(self['log-vbox']))
        else:
            self['log-vbox'].hide()
        return

    def add_page(self, objview, label, icon=None):
        '''
        Manages the notebook tab pages
        '''
        child = objview.get_top_widget()
        exist = False

        # Search for an existing tab page
        for view in self['notebook'].get_children():
            lbl = self['notebook'].get_tab_label(view)
            if lbl.get_data('tab-label') == label:
                self['notebook'].set_current_page(self['notebook'].page_num(view))
                exist = True
        # Create a new one
        if not exist:
            lbl = gtk.Label(label)
            lbl.set_ellipsize(pango.ELLIPSIZE_END)
            lbl.set_width_chars(10)
            lbl.set_alignment(0,0.5)
            img = gtk.Image()
            img.set_from_stock(gtk.STOCK_CLOSE, gtk.ICON_SIZE_MENU)
            icon_img = gtk.Image()
            icon_img.set_padding(2,0)
            if icon == 'Client':
                icon_img.set_from_stock(gtk.STOCK_NETWORK, gtk.ICON_SIZE_MENU)
            elif icon == 'Storage':
                icon_img.set_from_stock(gtk.STOCK_HARDDISK, gtk.ICON_SIZE_MENU)
            else:
                icon_img.set_from_stock(gtk.STOCK_JUMP_TO, gtk.ICON_SIZE_MENU)
            evt = gtk.EventBox()
            evt.add(img)
            evt.set_visible_window(False)
            btn = gtk.Button()
            btn.set_relief(gtk.RELIEF_NONE)
            btn.set_size_request(20, 20)
            btn.add(evt)
            evt_lbl = gtk.EventBox()
            evt_lbl.add(lbl)
            evt_lbl.set_visible_window(False)
            cnt = gtk.HBox()
            cnt.pack_start(icon_img, False, False)
            cnt.pack_start(evt_lbl, True, False)
            cnt.pack_start(btn, False, False)
            cnt.show_all()
            cnt.set_data('tab-label', label)
            child.show()
            nr = self['notebook'].append_page(child, cnt)

            menu_label = gtk.Label(icon + ': ' + label)
            menu_label.set_alignment(0,0)
            self['notebook'].set_menu_label(child, menu_label)

            self.tooltips.set_tip(evt_lbl, label)
            self['notebook'].set_current_page(nr)
            btn.connect('clicked', self.remove_page, child)
        return

    def show(self):
        '''Show the UI'''
        self['console-window'].show_all()
        return

    pass # End of ConsoleView class

class DirectorPromptView(View):
    def __init__(self, controller):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'director-prompt-dialog', None, True)
        self.controller = controller
        return

    def update_text(self, text):
        # Clear the vbox
        for child in self['text-vbox'].get_children():
            self['text-vbox'].remove(child)
        # Re-populate it
        for line in text:
            self['text-vbox'].add(gtk.Label(line))
        self['text-vbox'].show_all()
        return

    def update_prompt(self, message):
        self['prompt-label'].set_text(message)
        return

    def update_options(self, options):
        group = None
        #self['options-vbox'] = gtk.VBox(spacing=1)
        # Clear the vbox
        for child in self['options-vbox'].get_children():
            self['options-vbox'].remove(child)
        for (number, label) in options:
            radiobtn = gtk.RadioButton(group, label, False)
            # All radiobuttons on the same group
            if not group:
                group = radiobtn
            radiobtn.set_data('number', number)
            radiobtn.connect('toggled', self.controller.on_selection_toggled)
            self['options-vbox'].add(radiobtn)
        self['options-vbox'].show_all()
        return

    pass # End of DirectorPromptView class


class DirectorStatusView(View):
    def __init__(self, controller, parent):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'director-status-window', None, True)
        self['director-status-frame'].reparent(parent)
        return

    def update_name(self, name):
        self['director-name-label'].set_text(name)
        return

    def update_hostname(self, hostname):
        self['director-hostname-label'].set_text(hostname)
        return

    def update_time(self, time):
        self['director-time-label'].set_text(time)
        return

    def update_clients_nr(self, number):
        self['director-clients-nr-label'].set_text(str(number))
        return

    pass # End of DirectorStatusView class


class ClientStatusView(View):
    '''
    This view shows the Client status
    '''
    _client_list_model = None
    
    def __init__(self, controller, parent):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'client-status-window', None, True)
        self._init_client_list()
        self['client-status-frame'].reparent(parent)
        return

    def _init_client_list(self):
        '''
        Setup the TreeView widget with all the clients status
        '''
        self._client_list_model = gtk.ListStore(gobject.TYPE_BOOLEAN,
                                                gobject.TYPE_STRING)
        name_col = gtk.TreeViewColumn('Client',
                                      gtk.CellRendererText(), text=1)
        status_col = gtk.TreeViewColumn('Online',
                                        gtk.CellRendererToggle(), active=0)
        self['client-status-treeview'].set_model(self._client_list_model)
        self['client-status-treeview'].append_column(status_col)
        self['client-status-treeview'].append_column(name_col)
        return

    def update_client_list(self, client_list):
        '''
        Updates the client status list
        '''
        self._client_list_model.clear()
        for client in client_list.itervalues():
            self._client_list_model.append([client.online, client.name])
        return
    
    pass # End of ClientStatusView class

class ClientPageView(View):
    '''
    This view shows the Client page on the notebook
    '''
    _jobs_list_model = None
    _running_jobs_model = None
    
    def __init__(self, controller):
        View.__init__(self, controller, 'client-page.glade',
                      'client-page-frame', None, False)
        self._init_jobs_list()
        controller.registerView(self)
        return

    def _init_jobs_list(self):
        '''
        Various list initializations
        '''
        self._init_finished_jobs_list()
        self._init_running_jobs_list()
        return
    
    def _init_finished_jobs_list(self):
        '''
        Set-up the jobs done list treeview widget
        '''
        self._jobs_list_model = gtk.ListStore(gobject.TYPE_INT,    # id
                                              gobject.TYPE_STRING, # name
                                              gobject.TYPE_STRING, # start time
                                              gobject.TYPE_STRING, # level
                                              gobject.TYPE_INT,    # files
                                              gobject.TYPE_INT,    # bytes
                                              gobject.TYPE_STRING) # status
        # Default sorting by JobId
        self._jobs_list_model.set_sort_column_id(0, gtk.SORT_DESCENDING)
        # Set view's columns
        id_col = gtk.TreeViewColumn('#', gtk.CellRendererText(), text=0)
        id_col.set_sort_column_id(0)
        name_col = gtk.TreeViewColumn('Name', gtk.CellRendererText(), text=1)
        name_col.set_sort_column_id(1)
        start_col = gtk.TreeViewColumn('Start', gtk.CellRendererText(), text=2)
        start_col.set_sort_column_id(2)
        level_col = gtk.TreeViewColumn('Level', gtk.CellRendererText(), text=3)
        level_col.set_sort_column_id(3)
        files_col = gtk.TreeViewColumn('Files', gtk.CellRendererText(), text=4)
        files_col.set_sort_column_id(4)
        bytes_col = gtk.TreeViewColumn('Bytes', gtk.CellRendererText(), text=5)
        bytes_col.set_sort_column_id(5)
        status_col = gtk.TreeViewColumn('Status', gtk.CellRendererText(), text=6)
        status_col.set_sort_column_id(6)
        self['client-jobs-treeview'].set_model(self._jobs_list_model)
        self['client-jobs-treeview'].append_column(id_col)
        self['client-jobs-treeview'].append_column(name_col)
        self['client-jobs-treeview'].append_column(start_col)
        self['client-jobs-treeview'].append_column(level_col)
        self['client-jobs-treeview'].append_column(files_col)
        self['client-jobs-treeview'].append_column(bytes_col)
        self['client-jobs-treeview'].append_column(status_col)
        return

    def _init_running_jobs_list(self):
        '''
        Set-up the running jobs list treeview widget
        '''
        self._running_jobs_model = gtk.ListStore(gobject.TYPE_INT,    # id
                                                 gobject.TYPE_STRING, # name
                                                 gobject.TYPE_STRING, # start time
                                                 gobject.TYPE_STRING, # saved files
                                                 gobject.TYPE_INT,    # bytes
                                                 gobject.TYPE_INT,    # speed
                                                 gobject.TYPE_STRING) # processing file
        # Default sorting by JobId
        self._running_jobs_model.set_sort_column_id(0, gtk.SORT_DESCENDING)
        # Set view's columns
        id_col = gtk.TreeViewColumn('#', gtk.CellRendererText(), text=0)
        id_col.set_sort_column_id(0)
        name_col = gtk.TreeViewColumn('Name', gtk.CellRendererText(), text=1)
        start_col = gtk.TreeViewColumn('Start', gtk.CellRendererText(), text=2)
        saved_files_col = gtk.TreeViewColumn('Saved files', gtk.CellRendererText(), text=3)
        bytes_col = gtk.TreeViewColumn('Bytes', gtk.CellRendererText(), text=4)
        speed_col = gtk.TreeViewColumn('Speed', gtk.CellRendererText(), text=5)
        processing_col = gtk.TreeViewColumn('Processing...', gtk.CellRendererText(), text=6)
        # Set Model
        self['running-jobs-treeview'].set_model(self._running_jobs_model)
        # Set Columns
        self['running-jobs-treeview'].append_column(id_col)
        self['running-jobs-treeview'].append_column(name_col)
        self['running-jobs-treeview'].append_column(start_col)
        self['running-jobs-treeview'].append_column(saved_files_col)
        self['running-jobs-treeview'].append_column(bytes_col)
        self['running-jobs-treeview'].append_column(speed_col)
        self['running-jobs-treeview'].append_column(processing_col)
        return

    def set_online(self, online):
        '''
        Set the client online-ness widget
        '''
        if online:
            self['client-status-label'].set_text('Online')
            self['client-status-image'].set_from_stock(gtk.STOCK_YES,
                                                       gtk.ICON_SIZE_MENU)
        else:
            self['client-status-label'].set_text('Offline')
            self['client-status-image'].set_from_stock(gtk.STOCK_NO,
                                                       gtk.ICON_SIZE_MENU)
        return

    def set_name(self, name):
        self['client-name-label'].set_markup('<span size="15000"><b>Client: ' + name + '</b></span>')
        return

    def set_address(self, address):
        self['client-address-label'].set_text(address)
        return

    def set_port(self, port):
        self['client-port-label'].set_text(port)
        return

    def set_version(self, version):
        self['client-version-label'].set_text(version)
        return

    def update_jobs_list(self, jobs):
        '''
        Update jobs list
        '''
        # Job statuses taken from:
        # http://www.bacula.org/dev-manual/Python_Scripting.html
        job_status = {
            'C' : 'Created, not yet running',
            'R' : 'Running',
            'B' : 'Blocked',
            'T' : 'Completed successfully',
            'E' : 'Terminated with errors',
            'e' : 'Non-fatal error',
            'f' : 'Fatal error',
            'D' : 'Verify found differences',
            'A' : 'Canceled by user',
            'F' : 'Waiting for Client',
            'S' : 'Waiting for Storage daemon',
            'm' : 'Waiting for new media',
            'M' : 'Waiting for media mount',
            's' : 'Waiting for storage resource',
            'j' : 'Waiting for job resource',
            'c' : 'Waiting for client resource',
            'd' : 'Waiting on maximum jobs',
            't' : 'Waiting on start time',
            'p' : 'Waiting on higher priority jobs',
            }
        # Job levels
        job_level = {
            'F' : 'Full',
            'D' : 'Differential',
            'I' : 'Incremental',
            }
        self._jobs_list_model.clear()
        for job in jobs:
            self._jobs_list_model.append([int(job['JobId']),
                                         job['Name'],
                                         job['StartTime'],
                                         job_level[job['Level']],
                                         int(job['JobFiles']),
                                         int(job['JobBytes']),
                                         job_status[job['JobStatus']]])
        return

    def update_running_jobs_list(self, jobs):
        '''
        Update running jobs list
        '''
        self._running_jobs_model.clear()
        for job in jobs:
            self._running_jobs_model.append([job['id'],
                                            job['name'],
                                            job['start_date'],
                                            '%s/%s' % (job['saved_files'],
                                                       job['examined_files']),
                                            job['bytes'],
                                            job['speed'],
                                            job['processing_file']])
        return

    pass # End of ClientPageView class


class DirectorRunningJobsAppletView(View):
    '''
    This class and its controller represent the Director as a status bar
    applet that show the number of jobs running.
    '''
    def __init__(self, controller, parent):
        View.__init__(self, controller, 'running-jobs.glade',
                      None, None, True)
        self['running-jobs-hbox'].reparent(parent)
        return

    def update(self, jobs):
        '''
        Update the view with the new number of running jobs
        '''
        if jobs == 0:
            self['running-jobs-image'].set_from_stock(gtk.STOCK_NO,
                                                      gtk.ICON_SIZE_MENU)
            self['running-jobs-label'].set_text("No jobs running")
        elif jobs > 0:
            self['running-jobs-image'].set_from_stock(gtk.STOCK_YES,
                                                      gtk.ICON_SIZE_MENU)
            if jobs == 1:
                self['running-jobs-label'].set_text("Running a job")
            else:
                self['running-jobs-label'].set_text("Running %s jobs" % str(jobs))
        return

    pass # End of DirectorRunningJobsAppletView class

class DirectorObjectBrowserView(View):
    '''
    This class and its Controller represent the Director as a Tree Browser
    '''
    tree_model = None
    category_names = ['Clients','Storage','Jobs','Pool', 'FileSet',
                      'Schedule','Catalog']
    categories = {} # contain the main categories iterators
    
    def __init__(self, controller, parent):
        View.__init__(self, controller, 'object-browser.glade',
                      None, None, True)
        self._init_browser()
        self['object-browser-scroll'].reparent(parent)
        return

    def _init_browser(self):
        '''
        Initializes the very basic browser structure
        '''
        self.tree_model = gtk.TreeStore(gobject.TYPE_PYOBJECT, gobject.TYPE_STRING)
        column = gtk.TreeViewColumn("Object", gtk.CellRendererText(), text=1)
        self['object-browser'].set_model(self.tree_model)
        self['object-browser'].append_column(column)
        # Populate the tree
        for category in self.category_names:
            iter = self.tree_model.append(None, [None, category])
            self.categories[category] = iter
            self.tree_model.append(iter, [None, '...'])
        return

    def get_selected_object(self):
        '''
        Returns the selected object in the browser
        '''
        selection = self['object-browser'].get_selection()
        (model, iter) = selection.get_selected()
        (obj,) = model.get(iter, 0)
        return obj

    def toggle_category(self):
        '''
        Toggles the category: open/close
        '''
        selection = self['object-browser'].get_selection()
        (model, iter) = selection.get_selected()
        path = model.get_path(iter)
        if self['object-browser'].row_expanded(path):
            self['object-browser'].collapse_row(path)
        else:
            self['object-browser'].expand_row(path, False)
        return

    def insert_client(self, label, client):
        '''
        Inserts a new client on the corresponding category
        '''
        self.tree_model.append(self.categories['Clients'],
                               [client, label])
        return

    def insert_storage(self, label, storage):
        '''
        Inserts a new storage on the corresponding category
        '''
        self.tree_model.append(self.categories['Storage'],
                               [storage, label])
        return

    def insert_job(self, label, job):
        '''
        Inserts a new job on the corresponding category
        '''
        self.tree_model.append(self.categories['Jobs'],
                               [job, label])
        return

    def insert_pool(self, label, pool):
        '''
        Inserts a new pool on the corresponding category
        '''
        self.tree_model.append(self.categories['Pool'],
                               [pool, label])
        return

    def delete_clients(self):
        '''
        Delete all clients from the tree
        '''
        if self.tree_model.iter_has_child(self.categories['Clients']):
            iter = self.tree_model.iter_children(self.categories['Clients'])
            while self.tree_model.iter_is_valid(iter):
                self.tree_model.remove(iter)
        return

    def delete_storages(self):
        '''
        Delete all storages from the tree
        '''
        if self.tree_model.iter_has_child(self.categories['Storage']):
            iter = self.tree_model.iter_children(self.categories['Storage'])
            while self.tree_model.iter_is_valid(iter):
                self.tree_model.remove(iter)
        return

    def delete_jobs(self):
        '''
        Delete all jobs from the tree
        '''
        if self.tree_model.iter_has_child(self.categories['Jobs']):
            iter = self.tree_model.iter_children(self.categories['Jobs'])
            while self.tree_model.iter_is_valid(iter):
                self.tree_model.remove(iter)
        return

    def delete_pools(self):
        '''
        Delete all pools from the tree
        '''
        if self.tree_model.iter_has_child(self.categories['Pool']):
            iter = self.tree_model.iter_children(self.categories['Pool'])
            while self.tree_model.iter_is_valid(iter):
                self.tree_model.remove(iter)
        return

    def popup_client_menu(self, button, time):
        self['client-menu'].popup(None, None, None, button, time)
        return
    
    pass # End of DirectorObjectBrowserView class

class StoragePageView(View):
    '''
    This view shows the Storage page on the notebook
    '''
    def __init__(self, controller):
        View.__init__(self, controller, 'storage-page.glade',
                      'storage-page-frame', None, True)
        return

    def set_online(self, online):
        '''
        Set the storage online-ness widget
        '''
        if online:
            self['conn-status-label'].set_text('Online')
            self['conn-status-image'].set_from_stock(gtk.STOCK_YES,
                                                     gtk.ICON_SIZE_MENU)
        else:
            self['conn-status-label'].set_text('Offline')
            self['conn-status-image'].set_from_stock(gtk.STOCK_NO,
                                                     gtk.ICON_SIZE_MENU)
        return

    def set_enabled(self, enabled):
        '''
        Set the storage enabled-ness widget
        '''
        if enabled:
            self['status-label'].set_text('Enabled')
            self['status-image'].set_from_stock(gtk.STOCK_YES,
                                                gtk.ICON_SIZE_MENU)
        else:
            self['status-label'].set_text('Disabled')
            self['status-image'].set_from_stock(gtk.STOCK_NO,
                                                gtk.ICON_SIZE_MENU)
        return

    def set_name(self, name):
        self['name-label'].set_markup('<span size="15000"><b>Storage: ' + name + '</b></span>')
        return

    def set_address(self, address):
        self['address-label'].set_text(address)
        return

    def set_port(self, port):
        self['port-label'].set_text(port)
        return

    def set_version(self, version):
        self['version-label'].set_text(version)
        return

    def set_media_type(self, media_type):
        self['media-type-label'].set_text(media_type)
        return

    def set_device(self, device):
        self['device-label'].set_text(device)
        return

    pass # End of ClientPageView class


class FileSelectorView(View):
    '''
    File selector view, to be used in partial restore operations
    '''

    # Will use it at registerView() method on controller
    togglerenderer = None
    clear_statusbar = None
    
    def __init__(self, controller):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'select-files-window', None, False)
        self._init_tree()
        self._init_filelist()
        self._init_search_result()
        controller.registerView(self)
        return

    def _init_tree(self):
        '''
        Initializes the directory tree view
        '''
        self.tree_model = gtk.TreeStore(gobject.TYPE_STRING, # full path
                                        gobject.TYPE_STRING, # Dir name
                                        gtk.gdk.Pixbuf)      # Dir icon

        col_name = gtk.TreeViewColumn("Directory Tree")
        renderer = gtk.CellRendererPixbuf()
        col_name.pack_start(renderer, expand=False)
        col_name.add_attribute(renderer, 'pixbuf', 2)
        renderer = gtk.CellRendererText()
        col_name.pack_start(renderer, expand=True)
        col_name.add_attribute(renderer, 'text', 1)

        self['directory-tree'].set_model(self.tree_model)
        self['directory-tree'].append_column(col_name)
        self['directory-tree'].set_search_column(1) # Search by dirname
        icon = self['directory-tree'].render_icon(gtk.STOCK_DIRECTORY,
                                                  gtk.ICON_SIZE_MENU)
        iter = self.tree_model.append(None, ['/', '/', icon])
        self.tree_model.append(iter, ['...', '...', None])


        return

    def _init_filelist(self):
        '''
        Initializes the file list (right pane) list view
        '''
        self.list_model = gtk.ListStore(gobject.TYPE_STRING,  # Permissions
                                        gobject.TYPE_STRING,  # Owner
                                        gobject.TYPE_STRING,  # Group
                                        gobject.TYPE_INT,     # Size
                                        gobject.TYPE_STRING,  # Modif. date
                                        gobject.TYPE_STRING,  # Name
                                        gtk.gdk.Pixbuf,       # File icon
                                        gobject.TYPE_STRING,  # File type
                                        gobject.TYPE_BOOLEAN, # Marked?
                                        gobject.TYPE_STRING)  # Full path
        # Setup columns
        self.togglerenderer = gtk.CellRendererToggle()
        self.togglerenderer.set_property('activatable', True)
        col_marked = gtk.TreeViewColumn("", self.togglerenderer, active=8)
        col_marked.set_sort_column_id(8)
        col_permissions = gtk.TreeViewColumn("Permissions",
                                             gtk.CellRendererText(), text=0)
        col_owner = gtk.TreeViewColumn("Owner", gtk.CellRendererText(), text=1)
        col_owner.set_sort_column_id(1)
        col_group = gtk.TreeViewColumn("Group", gtk.CellRendererText(), text=2)
        col_group.set_sort_column_id(2)
        ##
        # Name column: mixed with pixbuf
        col_name = gtk.TreeViewColumn("Name")
        renderer = gtk.CellRendererPixbuf()
        col_name.pack_start(renderer, expand=False)
        col_name.add_attribute(renderer, 'pixbuf', 6)
        renderer = gtk.CellRendererText()
        col_name.pack_start(renderer, expand=True)
        col_name.add_attribute(renderer, 'text', 5)
        col_name.set_sort_column_id(5)
        col_name.set_resizable(True)
        #
        ##
        col_size = gtk.TreeViewColumn("Bytes", gtk.CellRendererText(), text=3)
        col_size.set_sort_column_id(3)
        col_date = gtk.TreeViewColumn("Date", gtk.CellRendererText(), text=4)
        col_date.set_sort_column_id(4)
        col_type = gtk.TreeViewColumn("Type", gtk.CellRendererText(), text=7)
        col_type.set_sort_column_id(7)
        # Default sorting by file type
        self.list_model.set_sort_column_id(7, gtk.SORT_ASCENDING)
        # Setup treeview
        self['file-list'].set_model(self.list_model)
        self['file-list'].append_column(col_marked)
        self['file-list'].append_column(col_name)
        self['file-list'].append_column(col_size)
        self['file-list'].append_column(col_permissions)
        self['file-list'].append_column(col_owner)
        self['file-list'].append_column(col_group)
        self['file-list'].append_column(col_type)
        self['file-list'].append_column(col_date)
        self['file-list'].set_search_column(5) # Search by filename
        return

    def _init_search_result(self):
        '''
        Initializes the search result list view
        '''
        self.search_model = gtk.ListStore(gobject.TYPE_STRING)
        col_file = gtk.TreeViewColumn('File', gtk.CellRendererText(), text=0)
        self['search-treeview'].set_model(self.search_model)
        self['search-treeview'].append_column(col_file)
        return

    def get_selected_dir(self):
        selection = self['directory-tree'].get_selection()
        (model, iter) = selection.get_selected()
        try:
            (obj,) = model.get(iter, 0)
        except TypeError:
            obj = None
        return obj

    def update_filelist_contents(self, cwd, contents):
        # Clear file list
        self.list_model.clear()
        # Add new content
        for afile in contents:
            # Directory
            if afile['name'][-1:] == '/':
                icon = self['file-list'].render_icon(gtk.STOCK_DIRECTORY,
                                                     gtk.ICON_SIZE_MENU)
                filetype = 'Directory'
                filename = afile['name'][:-1] # Strip trailing '/'
            # File
            else:
                icon = self['file-list'].render_icon(gtk.STOCK_FILE,
                                                     gtk.ICON_SIZE_MENU)
                filetype = 'File'
                filename = afile['name']
            # Append entry
            self.list_model.append([afile['permissions'],
                                    afile['owner'],
                                    afile['group'],
                                    afile['size'],
                                    afile['mdate'],
                                    filename,
                                    icon,
                                    filetype,
                                    afile['marked'],
                                    afile['fullpath']])
        return

    def update_dir_contents(self, cwd, contents):
        match_iter = self._search(self.tree_model,
                                  self.tree_model.iter_children(None),
                                  self._match_func, (0, cwd))
        child_iter = self.tree_model.iter_children(match_iter)
        # Add new content
        for afile in contents:
            # Directories
            if afile['name'][-1:] == '/':
                icon = self['directory-tree'].render_icon(gtk.STOCK_DIRECTORY,
                                                          gtk.ICON_SIZE_MENU)
                dir_iter = self.tree_model.append(match_iter,
                                                  [cwd+afile['name'],
                                                   afile['name'][:-1],
                                                   icon])
                # Add temporal dummy entry
                self.tree_model.append(dir_iter, ['...', '...', None])
        # Remove '...' entry
        if self.tree_model.get_value(child_iter, 0) == '...':
            self.tree_model.remove(child_iter)
        return

    def _match_func(self, model, iter, data):
        column, key = data # data is a tuple containing column number, key
        value = model.get_value(iter, column)
        return value == key
        
    def _search(self, model, iter, func, data):
        while iter:
            if func(model, iter, data):
                return iter
            result = self._search(model, model.iter_children(iter),
                                  func, data)
            if result: return result
            iter = model.iter_next(iter)
        return None

    def update_mark(self, path, marked):
        '''
        Updates marked status on file list
        '''
        iter = self.list_model.get_iter(path)
        self.list_model.set_value(iter, 8, marked)
        return

    def get_filename_from_path(self, path):
        '''
        Returns the file name from a specified path
        '''
        iter = self.list_model.get_iter(path)
        return self.list_model.get_value(iter, 5)

    def update_marked_message(self, message):
        '''
        Updates the statusbar with the marked files message from the director
        '''
        self._insert_statusbar_message(message)
        return

    def update_mouse_cursor(self, allowed):
        '''
        If input not allowed, show a wait cursor
        '''
        if not allowed:
            watch = gtk.gdk.Cursor(gtk.gdk.WATCH)
            self['select-files-window'].window.set_cursor(watch)
        else:
            self['select-files-window'].window.set_cursor(None)
        return

    def update_estimation(self, marked, total, bytes):
        '''
        Updates the statusbar with the last estimation from the director
        '''
        self['file-count'].set_fraction((marked * 1.0)/(total * 1.0))
        self['file-count'].set_text('%s bytes' % bytes)
        return    

    def _insert_statusbar_message(self, message):
        '''
        Updates the statusbar with some message at some context
        '''
        self['statusbar'].set_text(message)
        try:
            gobject.source_remove(self.clear_statusbar)
        except TypeError:
            pass
        self.clear_statusbar = gobject.timeout_add(5000,
                                                   self._delete_statusbar_message,
                                                   None)
        return    

    def _delete_statusbar_message(self, data):
        '''
        Clear the status bar
        '''
        self['statusbar'].set_text('')
        return False # De-schedule callback

    def update_cwd_label(self, cwd):
        self['cwd-label'].set_text(cwd)
        return

    def update_search_result(self, results):
        '''
        Update the treeview with new results
        '''
        self.search_model.clear()
        for result in results:
            self.search_model.append([result])
        return

    def get_search_term(self):
        '''
        Return the term entered by the user
        '''
        return self['search-entry'].get_text()

    def get_selected_search_result(self):
        '''
        Returns the selected filename on the search result list
        '''
        sel = self['search-treeview'].get_selection()
        model, iter = sel.get_selected()
        filename, = model.get(iter, 0)
        return filename

    def get_selected_file_name(self):
        '''
        Returns the selected filename on the file list
        '''
        sel = self['file-list'].get_selection()
        model, iter = sel.get_selected()
        filename, = model.get(iter, 9)
        return filename

    def expand_to_dir(self, iter=None, path='/'):
        '''
        Select the given path on the dir tree, if it not exist, create it
        '''
        if not iter:
            iter = self.tree_model.get_iter_root()

        dirlist = filter(lambda x: x != '', path.split('/'))
        if not dirlist:
            return
        # get the next dir name on the path
        dirname = dirlist.pop(0)        
        
        try:
            # Try to check on every dir child
            total_childs = self.tree_model.iter_n_children(iter)
            print "CHILDS:", total_childs
            
            if total_childs == 0:
                print "DIDN'T FOUND:", path, "...added!"
                new_iter = self.tree_model.append(iter,
                                              [path, dirname, None])
                new_path = '/'.join(dirlist)
                print "NEW PATH:", new_path
                if new_path != '':
                    if self.expand_to_dir(iter=new_iter, path=new_path):
                        return True
                else:
                    if self.expand_to_dir(iter=iter, path=dirname+'/'):
                        return True                    
                
            for child_nr in range(total_childs):
                child_iter = self.tree_model.iter_nth_child(iter, child_nr)
                childname = self.tree_model.get_value(child_iter, 1)
                print "CHECK:", childname
                if dirname == childname and childname != '...':
                    if dirlist != []:
                        # if it's the correct dir but not the last one,
                        # keep digging deeper :-)
                        new_path = '/'.join(dirlist)
                        if self.expand_to_dir(iter=child_iter, path=new_path):
                            return True
                    else:
                        # if it's the correct dir and the last one,
                        # go to it right now!!
                        print "FOUND!:", path
                        self['directory-tree'].expand_to_path(self.tree_model.get_path(child_iter))
                        self['directory-tree'].set_cursor(self.tree_model.get_path(child_iter))
                        return True
                else:
                    if child_nr == total_childs - 1:
                        # No more options, abort
                        print "DIDN'T FOUND:", path, "...added!"
                        iter = self.tree_model.append(iter,
                                                      [path, dirname, None])
                        new_path = '/'.join(dirlist)
                        if self.expand_to_dir(iter=iter, path=new_path):
                            return True
                    else:
                        # Keep looking...
                        print "MMMHHHH where is it:", path
        except TypeError:
            pass
        return
    
    pass # End of FileSelectorView class

class DirectorCommLogView(View):
    '''
    This class and its controller represent the Director as a communications
    log viewer, printing all data to and from the director
    '''

    command_tag = None
    reply_tag = None
    
    def __init__(self, controller, parent):
        View.__init__(self, controller, 'comm-log-viewer.glade',
                      'comm-log-viewer-window', None, True)
        # Setup font type
        font_desc = pango.FontDescription('monospace 10')
        self['log-textview'].modify_font(font_desc)
        self['log-scroll'].reparent(parent)
        # Setup color tags for text
        tag_table = self['log-textview'].get_buffer().get_tag_table()
        self.command_tag = gtk.TextTag("command")
        self.command_tag.set_property("foreground", "firebrick")
        tag_table.add(self.command_tag)
        self.reply_tag = gtk.TextTag("reply")
        self.reply_tag.set_property("foreground", "SteelBlue4")
        tag_table.add(self.reply_tag)
        return

    def _insert_text(self, text, tag):
        buffer = self['log-textview'].get_buffer()
        iter = buffer.get_end_iter()
        buffer.insert_with_tags(iter, text, tag)
        gobject.idle_add(self._scroll_to_end, 'log-textview')
        return

    def add_command(self, command):
        self._insert_text(command + '\n', self.command_tag)
        return
    
    def add_reply(self, reply):
        self._insert_text(reply + '\n', self.reply_tag)
        return

    def _scroll_to_end (self, widget_name):
        text = self[widget_name]
        iter = text.get_buffer().get_end_iter()
        text.scroll_to_iter(iter, 0, 1)
        return False  # don't requeue this handler

    pass # End of DirectorCommLogView class

class DirectorNoticeView(View):
    '''
    Notification to the user
    '''
    def __init__(self, controller):
        View.__init__(self, controller, 'pygtk-console.glade',
                      'notice-window', None, True)
        return

    def set_data(self, data):
        '''
        Establish the data to be shown to the user
        '''
        (summary, details) = data
        self['summary-label'].set_markup('<b>' + summary + '</b>')
        self['details-label'].set_text(details)
        return

    def show(self):
        self['notice-window'].show_all()
        return

    def hide(self):
        self['notice-window'].hide()
        return

    pass # End of NoticeView class
