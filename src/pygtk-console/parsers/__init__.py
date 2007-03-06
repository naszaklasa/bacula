# parsers/__init__.py: Parsers support classes
#
# Author: Lucas Di Pentima <lucas@lunix.com.ar>
#
# Version $Id: __init__.py,v 1.32 2006/11/27 10:03:01 kerns Exp $
#
# This file was contributed to the Bacula project by Lucas Di
# Pentima and LUNIX S.R.L.
#
# LUNIX S.R.L. has been granted a perpetual, worldwide,
# non-exclusive, no-charge, royalty-free, irrevocable copyright
# license to reproduce, prepare derivative works of, publicly
# display, publicly perform, sublicense, and distribute the original
# work contributed by Three Rings Design, Inc. and its employees to
# the Bacula project in source or object form.
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

import sre

# Callback's parsers
class Parser:
    '''
    Abstract class: defines the parser interface for commands callbacks
    '''
    data = {}
    
    def __init__(self):
        pass

    def parse(message):
        raise NotImplementedError

    pass # End of Parser class


class ClientStatusParser(Parser):
    '''
    Parses the status client=blablabla command
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        online = sre.search('Daemon started.*', message)
        try:
            version = sre.search('Version: (\d+\.\d+\.\d+ \(.*\).*)\n', message).groups()[0]
        except:
            version = 'unknown'
        self.data['version'] = version
        if online:
            self.data['online'] = True
        else:
            self.data['online'] = False
        # Get running job data
        running_jobs = []
        try:
            job_data = sre.findall('JobId ([\d,]+) Job (.*) is running.*?\n.*Backup Job started: (.*).*?\n.*Files=([\d,]+) Bytes=([\d,]+) Bytes/sec=([\d,]+).*?\n.*Files Examined=([\d,]+).*?\n.*Processing file: (.*)', message)
            for data in job_data:
                job = {}
                job['id'] = int(data[0])
                job['name'] = data[1]
                job['start_date'] = data[2]
                job['saved_files'] = int(data[3].replace(',',''))
                job['bytes'] = int(data[4].replace(',',''))
                job['speed'] = int(data[5].replace(',',''))
                job['examined_files'] = int(data[6].replace(',',''))
                job['processing_file'] = data[7]
                running_jobs.append(job)
        except:
            running_jobs == []
        self.data['running_jobs'] = running_jobs
        return self.data

    pass # End of ClientStatusParser class

class StorageStatusParser(Parser):
    '''
    Storage the status client=blablabla command
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        online = sre.search('Daemon started.*', message)
        try:
            version = sre.search('Version: (\d+\.\d+\.\d+ \(.*\).*)\n', message).groups()[0]
        except:
            version = 'unknown'
        self.data['version'] = version
        if online:
            self.data['online'] = True
        else:
            self.data['online'] = False
        return self.data

    pass # End of StorageStatusParser class

class ClientDefaultsParser(Parser):
    '''
    Returns a hash table with default client data
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        reply = {}
        try:
            match = sre.match('^client=(.*)address=(.*)fdport=(.*)file_retention=(.*)job_retention=(.*)autoprune=(.*)$', message).groups()
            reply['client'] = match[0]
            reply['address'] = match[1]
            reply['fdport'] = match[2]
            reply['file_retention'] = match[3]
            reply['job_retention'] = match[4]
            reply['autoprune'] = match[5]
        except AttributeError:
            reply = None
        return reply

    pass # End of DefaultsClientParser class

class JobDefaultsParser(Parser):
    '''
    Returns a hash table with default job data
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        try:
            match = sre.match('^job=(.*)pool=(.*)messages=(.*)client=(.*)storage=(.*)where=(.*)level=(.*)type=(.*)fileset=(.*)enabled=(.*)$', message).groups()
            self.data['job'] = match[0]
            self.data['pool'] = match[1]
            self.data['messages'] = match[2]
            self.data['client'] = match[3]
            self.data['storage'] = match[4]
            self.data['where'] = match[5]
            self.data['level'] = match[6]
            self.data['job_type'] = match[7]
            self.data['fileset'] = match[8]
            self.data['enabled'] = match[9]
        except AttributeError:
            self.data = None
        return self.data

    pass # End of DefaultsClientParser class

class StorageDefaultsParser(Parser):
    '''
    Returns a hash table with default storage data
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        try:
            match = sre.match('^storage=(.*)address=(.*)enabled=(.*)media_type=(.*)sdport=(.*)device=(.*)$', message).groups()
            self.data['storage'] = match[0]
            self.data['address'] = match[1]
            self.data['enabled'] = match[2]
            self.data['media_type'] = match[3]
            self.data['sdport'] = match[4]
            self.data['device'] = match[5]
        except AttributeError:
            self.data = None
        return self.data

    pass # End of DefaultsClientParser class

class PoolDefaultsParser(Parser):
    '''
    Returns a hash table with default pool data
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        try:
            match = sre.match('^pool=(.*)pool_type=(.*)label_format=(.*)use_volume_once=(.*)accept_any_volume=(.*)purge_oldest_volume=(.*)recycle_oldest_volume=(.*)recycle_current_volume=(.*)max_volumes=(.*)vol_retention=(.*)vol_use_duration=(.*)max_vol_jobs=(.*)max_vol_files=(.*)max_vol_bytes=(.*)auto_prune=(.*)recycle=(.*)$', message).groups()
            self.data['pool'] = match[0]
            self.data['pool_type'] = match[1]
            self.data['label_format'] = match[2]
            self.data['use_volume_once'] = match[3]
            self.data['accept_any_volume'] = match[4]
            self.data['purge_oldest_volume'] = match[5]
            self.data['recycle_oldest_volume'] = match[6]
            self.data['recycle_current_volume'] = match[7]
            self.data['max_volumes'] = match[8]
            self.data['vol_retention'] = match[9]
            self.data['vol_use_duration'] = match[10]
            self.data['max_vol_jobs'] = match[11]
            self.data['max_vol_files'] = match[12]
            self.data['max_vol_bytes'] = match[13]
            self.data['auto_prune'] = match[14]
            self.data['recycle'] = match[15]
        except AttributeError:
            self.data = None
        return self.data

    pass # End of DefaultsClientParser class

class DirectorPromptParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def extract_options(self, message):
        text = []
        prompt = None
        options = []
        for line in message.split("\n"):
            try:
                options.append(sre.findall('^.*([0-9]+): (.*)$', line)[0])
            except:
                # If not an option, it's some message
                if sre.search('^.*[Ss]elect.*', line):
                    prompt = line.strip()
                else:
                    text.append(line.strip())
        return [text, options, prompt]

    def parse(self, message):
        return self.extract_options(message)

    pass # End of DirectorPromptParser class

class TableParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        if sre.search('.*No results.*', message):
            return []
        a = message.split("\n")
        # Clean useless rows
        for row in a:
            if not (sre.match('^\+', row) or sre.match('^\|', row)):
                a.remove(row)
        # Re-assemble message
        message = "\n".join(a)
        # Continue processing
        headers = map(lambda x: x.strip(),a[1].split("|")[1:-1])
        data = message.split("\n")[3:-1]
        rtn = []
        for row in data:
            parsed={}
            r = row.split("|")[1:-1]
            for c in headers:
                parsed[c] = r.pop(0).strip()
            rtn.append(parsed)
        return rtn
    pass # End of TableParser class

class FullRestoreParser(Parser):
    '''
    Parses the director message for restore job creation and returns the
    jobid number.
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        try:
            jobid = sre.search('Job started. JobId=(\d+)', message).groups()[0]
        except:
            jobid = None
        return jobid

    pass # End of FullRestoreParser class

class PartialRestoreParser(Parser):
    '''
    Check if the director prompts for some answer. In the case of prompting the
    file selection, return None.
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        if sre.search('cwd is\:', message):
            # File selection ready
            return None
        else:
            # Director needs additional data, return message as is
            return message
        pass
    
    pass # End of PartialRestoreParser class

class GetDirContentParser(Parser):
    '''
    Returns a list with the file names got from a "ls" or "dir" command
    on interactive file marking
    '''
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        filelist = []
        # Avoid the last $ prompt
        files = message.split('\n')[:-1]
        for file_entry in files:
            try:
                # Separate all file data from extended file listing
                match = sre.match('^(.*),(.*),(.*),(.*),(.*),(.*),(.*),(.*)$', file_entry).groups()
                afile = {}
                afile['permissions'] = match[0]
                afile['owner'] = match[2]
                afile['group'] = match[3]
                afile['size'] = int(match[4])
                afile['mdate'] = match[5]
                # Check if marked
                if sre.match("^\*.*", match[6]):
                    afile['marked'] = True
                else:
                    afile['marked'] = False                    
                # Strip dirname
                afile['name'] = sre.match('^.*/(.+)$', match[7]).groups()[0]
                afile['fullpath'] = match[7]
                filelist.append(afile)
            except AttributeError:
                pass
        return filelist

    pass # End of GetDirContentParser class

class ChangeDirParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        if sre.search(".*Invalid path given.*", message):
            return None
        else:
            return sre.search(".*cwd is: (.*)\n.*", message).groups()[0]

    pass # End of ChangeDirParser class

class MarkFileParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        files = sre.match("^([0-9]+ files? .*\.).*", message).groups()[0]
        return files

    pass # End of MarkFileParser class

class EstimateParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        reply = {}
        total, marked, bytes = message.split(';')
        reply['total'] = int(sre.match('^([0-9]+).*', total.strip()).groups()[0])
        reply['marked'] = int(sre.match('^([0-9]+).*', marked.strip()).groups()[0])
        reply['bytes'] = sre.match('^([0-9,]+).*', bytes.strip()).groups()[0]
        return reply

    pass # End of EstimateParser class

class SearchResultParser(Parser):
    def __init__(self):
        Parser.__init__(self)
        return

    def parse(self, message):
        reply = []
        for result in message.split('\n')[:-1]:
            if result[0] == '*' or result[0] == '+':
                reply.append(result[1:]) # Avoid the last char
            else:
                reply.append(result)
        return reply

    pass # End of SearchResultParser class
