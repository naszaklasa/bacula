# Controllers.py: Controller classes
#
# Author: Lucas Di Pentima <lucas@lunix.com.ar>
#
# Version $Id: Controllers.py,v 1.48 2006/11/27 10:03:01 kerns Exp $
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

from gtkmvc.controller import Controller
from Views import *
from Models import *

class LoginController (Controller):
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_name_change_notification (self, model, old, new):
        '''Update the view with the new name'''
        self.view.update_name_entry(new)
        return

    def property_hostname_change_notification (self, model, old, new):
        '''Update the view with the new hostname'''
        self.view.update_hostname_entry(new)
        return

    def property_password_change_notification (self, model, old, new):
        '''Update the view with the new password'''
        self.view.update_password_entry(new)
        return

    def property_port_change_notification (self, model, old, new):
        '''Update the view with the new port number'''
        self.view.update_port_entry(new)
        return

    def property_servers_loaded_change_notification(self, model, old, new):
        '''
        Populates the servers combobox
        '''
        default = self.model.servers.index(self.model.name)
        self.view.populate_combobox(self.model.servers, default)
        return

    def on_login_button_ok_clicked (self, widget):
        '''Get the login data from the View and tell the Model to
        authenticate against the director'''

        self.model.name = self.view.name
        self.model.hostname = self.view.hostname
        self.model.password = self.view.password
        self.model.port = self.view.port
                              
        if self.model.authenticate():
            print "Connected!!!"
        else:
            # NOTE: show some error dialog
            print "Error connecting to Bacula"
        return

    def on_login_button_quit_clicked (self, widget):
        '''Tell the model to quit application'''
        self.model.quit()
        return

    def on_login_dialog_destroy (self, widget):
        '''Tell the model to quit application'''
        self.model.quit()
        return

    def on_combobox_servers_changed(self, widget):
        '''
        User selects another server to connect to
        '''
        self.model.select_server(widget.get_active_text())
        return

    def destroy(self):
        '''Tell the view to quit'''
        self.view.hide()
        return

    def registerView(self, view):
        '''
        Tell the view to show itself
        '''
        Controller.registerView(self, view)
        self.model.load_config()
        self.view.show()
        return
        

    pass # End of LoginController class

class ConsoleController (Controller):
    client_status_controller = None
    client_status_view = None
    director_status_controller = None
    director_status_view = None
    director_prompt_controller = None
    director_prompt_view = None
    director_object_browser_controller = None
    director_object_browser_view = None
    director_running_jobs_applet_view = None
    director_running_jobs_applet_controller = None
    director_comm_log_controller = None
    director_comm_log_view = None
    director_info_controller = None
    director_info_view = None
    
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def remove_page(self, child):
        '''
        Ask the view to close a notebook page
        '''
        self.view.remove_page(child)
        return

    def add_page(self, view, label, icon=None):
        '''
        Ask the view to add the child to the notebook
        '''
        self.view.add_page(view, label, icon)
        return

    def show_client(self, client):
        '''
        Instantiate a new client view-controller pair and show it.
        '''
        client.update() # Force an update
        controller = ClientPageController(client)
        view = ClientPageView(controller)
        self.add_page(view, client.name, 'Client')
        return

    def show_storage(self, storage):
        '''
        Instantiate a new storage view-controller pair and show it.
        '''
        storage.update() # Force an update
        controller = StoragePageController(storage)
        view = StoragePageView(controller)
        self.add_page(view, storage.name, 'Storage')
        return

    def do_full_restore(self, client):
        '''
        Perform a full restore job on desired client
        '''
        self.model.do_full_restore(client)
        return

    def on_console_window_destroy (self, widget):
        self.model.quit()
        return

    def on_input_activate(self, widget):
        if self.model.at_prompt:
            self.view.update_output_command("%s\n" % self.view.input)
        else:
            self.view.update_output_command("* %s\n" % self.view.input)
        self.model.input = self.view.input
        self.view.reset_prompt()
        return

    def on_input_entry_key_press_event(self, widget, event):
        '''
        Detects up/down arrow key press to retrieve old commands
        '''
        if event.keyval in (gtk.keysyms.Up, gtk.keysyms.KP_Up):
            self.model.cmd_history_up()
        elif event.keyval in (gtk.keysyms.Down, gtk.keysyms.KP_Down):
            self.model.cmd_history_down()
        return

    def on_toolbutton_disconnect_clicked(self, widget):
        '''Ask the model to disconnect from the Director'''
        self.model.disconnect()
        return

    def on_toolbutton_quit_clicked(self, widget):
        '''Ask the model to close the application'''
        self.model.quit()
        return

    def on_console_hide_button_clicked(self, widget):
        self.model.show_console = False
        return

    def on_stats_hide_button_clicked(self, widget):
        self.model.show_stats = False
        return

    def on_messages_hide_button_clicked(self, widget):
        self.model.show_messages = False
        return

    def on_log_hide_button_clicked(self, widget):
        self.model.show_log = False
        return

    def on_clean_messages_button_clicked(self, widget):
        self.view.clean_messages()
        return

    def on_toggletoolbutton_auto_update_toggled(self, widget):
        '''
        Change the model to reflect this button status
        '''
        self.model.auto_update = widget.get_active()
        return

    def on_toggletoolbutton_console_toggled(self, widget):
        '''
        Change the model to reflect this button status
        '''
        self.model.show_console = widget.get_active()
        return

    def on_toggletoolbutton_stats_toggled(self, widget):
        '''
        Change the model to reflect this button status
        '''
        self.model.show_stats = widget.get_active()
        return

    def on_toggletoolbutton_messages_toggled(self, widget):
        '''
        Change the model to reflect this button status
        '''
        self.model.show_messages = widget.get_active()
        return

    def on_toggletoolbutton_log_toggled(self, widget):
        '''
        Change the model to reflect this button status
        '''
        self.model.show_log = widget.get_active()
        return

    def on_output_textview_focus_in_event(self, widget, event):
        '''
        When clicking on the console output textview, return focus
        to the commandline input text entry.
        '''
        self.view.set_focus_commandline()
        return

    def on_notebook_switch_page(self, widget, page, page_num):
        '''
        Do stuff when notebook page selections changes
        '''
        if page_num == 0: # Console
            self.view.set_focus_commandline()
        elif page_num == 2: # Message log
            self.view.reset_messages_tab()
        return

    def property_history_index_change_notification(self, model, old, new):
        '''
        Retrieves old command to input widget
        '''
        if new != 0:
            self.view.retrieve_command(self.model.cmd_history[new])
        elif new == 0:
            self.view.retrieve_command('') # Clear input field
        return

    def property_input_change_notification(self, model, old, new):
        '''
        Input property changed, execute command.
        '''
        if new == '':
            # If we send '' through the connection, the bacula module
            # goes crazy taking all available memory :-/
            self.model.execute("\n")
        else:
            self.model.execute(new)
        return

    def property_auto_update_change_notification(self, model, old, new):
        '''
        Change the view to reflect this state
        '''
        if new != self.view['toggletoolbutton-auto-update'].get_active():
            self.view['toggletoolbutton-auto-update'].set_active(new)
        return

    def property_show_console_change_notification(self, model, old, new):
        self.view['toggletoolbutton-console'].set_active(new)
        self.view.show_console(new)
        self.model.application.config.set('global', 'show_console', new)
        return

    def property_show_stats_change_notification(self, model, old, new):
        self.view['toggletoolbutton-stats'].set_active(new)
        self.view.show_stats(new)
        self.model.application.config.set('global', 'show_stats', new)
        return

    def property_show_messages_change_notification(self, model, old, new):
        self.view['toggletoolbutton-messages'].set_active(new)
        self.view.show_messages(new)
        self.model.application.config.set('global', 'show_messages', new)
        return

    def property_show_log_change_notification(self, model, old, new):
        self.view['toggletoolbutton-log'].set_active(new)
        self.view.show_log(new)
        self.model.application.config.set('global', 'show_log', new)
        return

    def property_host_change_notification(self, model, old, new):
        '''Update the view with the new director\'s hostname'''
        self.view.update_director_name(new)
        return

    def property_connected_change_notification(self, model, old, new):
        '''Update the view with the new connection status'''
        self.view.update_connection_status(new)
        return

    def property_running_change_notification(self, model, old, new):
        '''Update the view with the running command status'''
        self.view.update_running_status(running=new,
                                        at_prompt=self.model.at_prompt)
        return

    def property_at_prompt_change_notification(self, model, old, new):
        '''Update the view with the running command status'''
        self.view.update_running_status(running=self.model.running,
                                        at_prompt=new)
        return

    def property_output_change_notification(self, model, old, new):
        '''Update the view with the director\'s result'''
        self.view.update_output_reply(new)
        return

    def property_messages_change_notification(self, model, old, new):
        '''Update the view with the director\'s messages'''
        self.view.update_messages(new)
        return

    def destroy(self):
        '''Tell the view to disappear!'''
        self.view.hide()
        return
    
    def registerView(self, view):
        '''
        Completes model initialization, and tell the view to show itself
        '''
        Controller.registerView(self, view)
        self.model.post_init()
        # Director Status
        self.director_status_controller = DirectorStatusController(self.model.director)
        self.director_status_view = DirectorStatusView(self.director_status_controller, self.view['status-vbox'])
        # Director Prompt
        self.director_prompt_controller = DirectorPromptController(self.model.director)
        self.director_prompt_view = DirectorPromptView(self.director_prompt_controller)
        # Client Status
        self.client_status_controller = ClientStatusController(self.model.director)
        self.client_status_view = ClientStatusView(self.client_status_controller, self.view['status-vbox'])
        # Object Browser
        self.director_object_browser_controller = DirectorObjectBrowserController(self.model.director, self)
        self.director_object_browser_view = DirectorObjectBrowserView(self.director_object_browser_controller, self.view['object-browser-frame'])
        # Running Jobs Applet
        self.director_running_jobs_applet_controller = DirectorRunningJobsAppletController(self.model.director)
        self.director_running_jobs_applet_view = DirectorRunningJobsAppletView(self.director_running_jobs_applet_controller, self.view['running-jobs-slot'])
        # Communication Log
        self.director_comm_log_controller = DirectorCommLogController(self.model.director)
        self.director_comm_log_view = DirectorCommLogView(self.director_comm_log_controller, self.view['log-vbox'])
        # Info Dialog
        self.director_info_controller = DirectorNoticeController(self.model.director)
        self.director_info_view = DirectorNoticeView(self.director_info_controller)
        # Show time!
        self.view.show()
        
        # Open / close panels from config settings
        try:
            self.model.show_console = self.model.application.config.getboolean('global', 'show_console')
            self.model.show_stats = self.model.application.config.getboolean('global', 'show_stats')
            self.model.show_messages = self.model.application.config.getboolean('global', 'show_messages')
            self.model.show_log = self.model.application.config.getboolean('global', 'show_log')
        except NoOptionError:
            pass
        return

    pass # end of ConsoleController class


class DirectorPromptController(Controller):
    prompt_answer = '1' # First option always selected
    
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_prompt_data_change_notification(self, model, old, new):
        (text, options, prompt) = new
        self.view.update_text(text)
        self.view.update_prompt(prompt)
        self.view.update_options(options)
        self.view.show()
        return

    def on_selection_toggled(self, widget):
        if widget.get_active():
            number = widget.get_data('number')
            self.prompt_answer = number
        return

    def on_ok_button_clicked(self, widget):
        self.view.hide()
        self.model.prompt_answer(self.prompt_answer)
        return

    def on_cancel_button_clicked(self, widget):
        self.view.hide()
        self.model.prompt_cancel()
        return

    pass # End of DirectorPromptController class


class DirectorStatusController(Controller):
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_name_change_notification(self, model, old, new):
        self.view.update_name(new)
        return

    def property_hostname_change_notification(self, model, old, new):
        self.view.update_hostname(new)
        return

    def property_time_change_notification(self, mode, old, new):
        self.view.update_time(new)
        return

    def property_clients_nr_change_notification(self, mode, old, new):
        self.view.update_clients_nr(new)
        return

    def property_running_jobs_change_notification(self, model, old, new):
        '''
        Checks for running jobs
        '''
        # NO-OP for now...
        return

    pass # End of DirectorStatusController class


class ClientStatusController(Controller):
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_clientlist_updated_change_notification(self, model, old, new):
        '''
        Update the view with the new data
        '''
        self.view.update_client_list(self.model._client_list)
        return

    pass # End of ClientStatusController class

class ClientPageController(Controller):
    '''
    Client Page controller
    '''
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_online_change_notification(self, model, old, new):
        '''
        Tell the view to update the online status
        '''
        if old != new:
            self.view.set_online(new)
        return

    def property_version_change_notification(self, model, old, new):
        '''
        Tell the view to update the client version
        '''
        if old != new:
            self.view.set_version(new)
        return

    def property_address_change_notification(self, model, old, new):
        '''
        Tell the view to update the client address
        '''
        if old != new:
            self.view.set_address(new)
        return

    def property_port_change_notification(self, model, old, new):
        '''
        Tell the view to update the client port
        '''
        if old != new:
            self.view.set_port(new)
        return

    def property_jobs_done_change_notification(self, model, old, new):
        '''
        Tell the view to update the jobs list
        '''
        if old != new:
            self.view.update_jobs_list(self.model.jobs_done)
        return

    def property_jobs_running_change_notification(self, model, old, new):
        '''
        Tell the view to update the running jobs list
        '''
        if old != new:
            self.view.update_running_jobs_list(self.model.jobs_running)
        return

    def registerView(self, view):
        '''
        Some view initializations
        '''
        Controller.registerView(self, view)
        self.view.set_name(self.model.name)
        self.view.set_address(self.model.address)
        self.view.set_port(self.model.port)
        self.view.set_version(self.model.version)
        self.view.set_online(self.model.online)
        self.view.update_jobs_list(self.model.jobs_done)
        return

    pass # End of ClientPageController class

class StoragePageController(Controller):
    '''
    Storage Page controller
    '''
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_enabled_change_notification(self, model, old, new):
        '''
        Tell the view to update the enabled status
        '''
        self.view.set_enabled(new)
        return

    def property_online_change_notification(self, model, old, new):
        '''
        Tell the view to update the online status
        '''
        self.view.set_online(new)
        return

    def registerView(self, view):
        '''
        Some view initializations
        '''
        Controller.registerView(self, view)
        self.view.set_name(self.model.name)
        self.view.set_address(self.model.address)
        self.view.set_port(self.model.port)
        self.view.set_version(self.model.version)
        self.view.set_online(self.model.online)
        self.view.set_media_type(self.model.media_type)
        self.view.set_device(self.model.device)
        self.view.set_enabled(self.model.enabled)
        return

    pass # End of StoragePageController class

class DirectorRunningJobsAppletController(Controller):
    '''
    This class and its view represent the Director as a status applet
    that shows the number of running jobs
    '''
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_running_jobs_change_notification(self, model, old, new):
        '''
        Updates the applet
        '''
        self.view.update(new)
        return

    pass # end of DirectorRunningJobsAppletController class

class DirectorObjectBrowserController(Controller):
    '''
    This class and its View represent the Director as Tree Browser
    '''
    console_controller = None
    
    def __init__(self, model, console):
        Controller.__init__(self, model)
        self.console_controller = console
        model.registerObserver(self)
        return

    def property_clientlist_updated_change_notification(self, model, old, new):
        '''
        Updates client list on browser
        '''
        self.view.delete_clients()
        for client in self.model.get_clients().itervalues():
            self.view.insert_client(client.name, client)
        return

    def property_storagelist_updated_change_notification(self, model, old, new):
        '''
        Updates storage list on browser
        '''
        self.view.delete_storages()
        for storage in self.model.get_storages().itervalues():
            self.view.insert_storage(storage.name, storage)
        return

    def property_joblist_updated_change_notification(self, model, old, new):
        '''
        Updates job list on browser
        '''
        self.view.delete_jobs()
        for job in self.model.get_jobs().itervalues():
            self.view.insert_job(job.name, job)
        return

    def property_poollist_updated_change_notification(self, model, old, new):
        '''
        Updates pool list on browser
        '''
        self.view.delete_pools()
        for pool in self.model.get_pools().itervalues():
            self.view.insert_pool(pool.name, pool)
        return

    def on_object_browser_button_press_event(self, treeview, event):
        '''
        Popup menus and open new tabs, depending on the object selected
        '''
        x = int(event.x)
        y = int(event.y)
        time = event.time
        pthinfo = treeview.get_path_at_pos(x, y)
        if pthinfo != None:
            path, col, cellx, celly = pthinfo
            treeview.grab_focus()
            treeview.set_cursor(path, col, 0)
            obj = self.view.get_selected_object()
            # Left-clicks
            if event.button == 1:
                # Open different view depending on object type
                if isinstance(obj, Client):
                    self.model.update_jobs_list()
                    self.console_controller.show_client(obj)
                elif isinstance(obj, Storage):
                    self.console_controller.show_storage(obj)
                else:
                    # Toggle category: expand/collapse
                    self.view.toggle_category()
            # Middle-clicks
            elif event.button == 2:
                pass
            # Right-clicks
            elif event.button == 3:
                # Popup messages depending on object type
                if isinstance(obj, Client):
                    self.view.popup_client_menu(event.button, time)
                else:
                    pass
        return True

    def on_restore_all_files_activate(self, widget, data=None):
        '''
        Execute a full restore on selected client
        '''
        obj = self.view.get_selected_object()
        if isinstance(obj, Client):
            self.model.do_full_restore(obj)
        return

    def on_restore_some_files_activate(self, widget, data=None):
        '''
        Execute a partial restore on selected client
        '''
        obj = self.view.get_selected_object()
        if isinstance(obj, Client):
            selector = FileSelector(self.model)
            sel_controller = FileSelectorController(selector)
            sel_view = FileSelectorView(sel_controller)
            self.model.do_partial_restore(obj, selector)
        return

    pass # end of DirectorObjectBrowserController class


class FileSelectorController(Controller):
    '''
    This class is to be used in partial restore operations
    '''
    _visited = {} # Paths already visited

    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_cwd_path_change_notification(self, model, old, new):
        '''
        User ask for a directory change
        '''
        if old != new and self.model.allow_input:
            self.view.update_cwd_label(new)
            self.view.expand_to_dir(path=new)
            self.model.get_list()
        return

    def property_cwd_contents_change_notification(self, model, old, new):
        '''
        Update the view about cwd contents
        '''
        if old != new and self.model.allow_input:
            # update tree view only when not in cache
            if not self.model.cached(self.model.cwd_path):
                self.view.update_dir_contents(self.model.cwd_path, new)
            # update list view always
            self.view.update_filelist_contents(self.model.cwd_path, new)
        return

    def property_active_change_notification(self, model, old, new):
        '''
        Activates/deactivates selector
        '''
        if new:
            self.model.reset()
            self.view.show()
        else:
            self.view.hide()
        return

    def property_marked_message_change_notification(self, model, old, new):
        '''
        Updates view with last marked files count
        '''
        self.view.update_marked_message(new)
        return

    def property_last_estimation_change_notification(self, model, old, new):
        '''
        Updates the view with the las estimation data
        '''
        self.view.update_estimation(new['marked'],
                                    new['total'],
                                    new['bytes'])
        return

    def property_allow_input_change_notification(self, model, old, new):
        '''
        Tell the view to show the correct mouse cursor
        '''
        if new != old:
            self.view.update_mouse_cursor(new)
        return

    def on_directory_tree_button_press_event(self, treeview, event):
        '''
        Respond to user clicks on directories.
        '''
        x = int(event.x)
        y = int(event.y)
        pthinfo = treeview.get_path_at_pos(x, y)
        if pthinfo != None:
            path, col, cellx, celly = pthinfo
            treeview.grab_focus()
            treeview.set_cursor(path, col, 0)
            directory = self.view.get_selected_dir()
            if directory != None and directory != '...' and self.model.allow_input:
                self.model.change_dir(directory)
        return

    def on_cancel_button_clicked(self, widget):
        '''
        Reset data and tell the model to cancel file selection
        '''
        self.model.cancel_operation()
        return

    def on_select_files_window_delete_event(self, window, data):
        '''
        Reset data and tell the model to cancel file selection
        '''
        self.model.cancel_operation()
        return

    def on_filelist_toggled(self, cell, path, data):
        '''
        Mark file or dir for restoration
        '''
        if self.model.allow_input:
            marked = not cell.get_active()
            filename = self.view.get_filename_from_path(path)
            if marked:
                self.model.mark(filename, True)
            else:
                self.model.mark(filename, False)
            self.view.update_mark(path, marked)
        return

    def on_estimate_button_clicked(self, widget):
        '''
        Tell the model to estimate number of files selected
        '''
        if self.model.allow_input:
            self.model.estimate()
        return

    def on_restore_button_clicked(self, widget):
        '''
        Confirm to the director that the selection is done
        '''
        self.model.confirm_operation()
        return

    def registerView(self, view):
        '''
        Connect signal from toggle button
        '''
        Controller.registerView(self, view)
        view.togglerenderer.connect('toggled', self.on_filelist_toggled, None)
        return

    def on_search_button_clicked(self, widget):
        '''
        Tell the model to search
        '''
        self._search()
        return

    def on_search_entry_activate(self, widget):
        '''
        Tell the model to search
        '''
        self._search()
        return

    def _search(self):
        '''
        Tell the model to search
        '''        
        search_term = self.view.get_search_term()
        if search_term:
            self.model.search(search_term)
        return
        
    def property_search_result_change_notification(self, model, old, new):
        '''
        Show the search results on the view
        '''
        if old != new:
            self.view.update_search_result(new)
        return

    def on_search_treeview_button_press_event(self, treeview, event):
        '''
        On double click, select the clicked result
        '''
        if event.button == 1 and event.type == gtk.gdk._2BUTTON_PRESS:
            result = self.view.get_selected_search_result()
            if result[-1:] == '/':
                # Dir stuff
                self.view.expand_to_dir(path=result)
                #self.model.change_dir(result)
                pass
            else:
                # File stuff
                pass
        return

    def on_file_list_button_press_event(self, treeview, event):
        '''
        On double click, change to selected directory
        '''
        if event.button == 1 and event.type == gtk.gdk._2BUTTON_PRESS:
            filename = self.view.get_selected_file_name()
            if filename[-1:] == '/':
                self.model.change_dir(filename)
        return

    pass # End of FileSelectorController class

class DirectorCommLogController(Controller):
    '''
    This class and its view represent the Director as a communications
    log viewer, printing all data to and from the director
    '''
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_last_command_change_notification(self, model, old, new):
        self.view.add_command(new)
        return

    def property_last_reply_change_notification(self, model, old, new):
        self.view.add_reply(new)
        return

    pass # End of DirectorCommLogController class

class DirectorNoticeController(Controller):
    '''
    Notification to the user
    '''
    def __init__(self, model):
        Controller.__init__(self, model)
        model.registerObserver(self)
        return

    def property_show_info_change_notification(self, model, old, new):
        '''
        Show an info dialog to the user
        '''
        if model.show_info:
            self.view.set_data(model.show_info_data)
            self.view.show()
        return

    def on_notice_window_delete_event(self, widget, event=None): 
        self.view.hide()
        return True
   
    def on_close_button_clicked(self, widget):
        self.view.hide()
        return
    
    pass # End of NoticeController class
