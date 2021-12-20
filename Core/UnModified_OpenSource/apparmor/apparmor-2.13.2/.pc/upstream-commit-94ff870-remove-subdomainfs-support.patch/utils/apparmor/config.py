# ----------------------------------------------------------------------
#    Copyright (C) 2013 Kshitij Gupta <kgupta8592@gmail.com>
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License as published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
# ----------------------------------------------------------------------
from __future__ import with_statement
import os
import shlex
import shutil
import stat
import sys
import tempfile
if sys.version_info < (3, 0):
    import ConfigParser as configparser

    # Class to provide the object[section][option] behavior in Python2
    class configparser_py2(configparser.ConfigParser):
        def __getitem__(self, section):
            section_val = self.items(section)
            section_options = dict()
            for option, value in section_val:
                section_options[option] = value
            return section_options


else:
    import configparser


from apparmor.common import AppArmorException, open_file_read  # , warn, msg,


# CFG = None
# REPO_CFG = None
# SHELL_FILES = ['easyprof.conf', 'notify.conf', 'parser.conf', 'subdomain.conf']
class Config(object):
    def __init__(self, conf_type, conf_dir='/etc/apparmor'):
        self.CONF_DIR = conf_dir
        # The type of config file that'll be read and/or written
        if conf_type == 'shell' or conf_type == 'ini':
            self.conf_type = conf_type
            self.input_file = None
        else:
            raise AppArmorException("Unknown configuration file type")

    def new_config(self):
        if self.conf_type == 'shell':
            config = {'': dict()}
        elif self.conf_type == 'ini':
            config = configparser.ConfigParser()
        return config

    def read_config(self, filename):
        """Reads the file and returns a config[section][attribute]=property object"""
        # LP: Bug #692406
        # Explicitly disabled repository
        filepath = self.CONF_DIR + '/' + filename
        self.input_file = filepath
        if filename == "repository.conf":
            config = dict()
            config['repository'] = {'enabled': 'no'}
        elif self.conf_type == 'shell':
            config = self.read_shell(filepath)
        elif self.conf_type == 'ini':
            if sys.version_info > (3, 0):
                config = configparser.ConfigParser()
            else:
                config = configparser_py2()
            # Set the option form to string -prevents forced conversion to lowercase
            config.optionxform = str
            if sys.version_info > (3, 0):
                config.read(filepath)
            else:
                try:
                    config.read(filepath)
                except configparser.ParsingError:
                    tmp_filepath = py2_parser(filepath)
                    config.read(tmp_filepath.name)
                    ##config.__get__()
        return config

    def write_config(self, filename, config):
        """Writes the given config to the specified file"""
        filepath = self.CONF_DIR + '/' + filename
        permission_600 = stat.S_IRUSR | stat.S_IWUSR    # Owner read and write
        try:
            # Open a temporary file in the CONF_DIR to write the config file
            config_file = tempfile.NamedTemporaryFile('w', prefix='aa_temp', delete=False, dir=self.CONF_DIR)
            if os.path.exists(self.input_file):
                # Copy permissions from an existing file to temporary file
                shutil.copymode(self.input_file, config_file.name)
            else:
                # If no existing permission set the file permissions as 0600
                os.chmod(config_file.name, permission_600)
            if self.conf_type == 'shell':
                self.write_shell(filepath, config_file, config)
            elif self.conf_type == 'ini':
                self.write_configparser(filepath, config_file, config)
            config_file.close()
        except IOError:
            raise AppArmorException("Unable to write to %s" % filename)
        else:
            # Replace the target config file with the temporary file
            os.rename(config_file.name, filepath)

    def find_first_file(self, file_list):
        """Returns name of first matching file None otherwise"""
        filename = None
        if file_list:
            for f in file_list.split():
                if os.path.isfile(f):
                    filename = f
                    break
        return filename

    def find_first_dir(self, dir_list):
        """Returns name of first matching directory None otherwise"""
        dirname = None
        if dir_list:
            for direc in dir_list.split():
                if os.path.isdir(direc):
                    dirname = direc
                    break
        return dirname

    def read_shell(self, filepath):
        """Reads the shell type conf files and returns config[''][option]=value"""
        config = {'': dict()}
        with open_file_read(filepath) as conf_file:
            for line in conf_file:
                result = shlex.split(line, True)
                # If not a comment of empty line
                if result:
                    # option="value" or option=value type
                    if '=' in result[0]:
                        option, value = result[0].split('=')
                    # option type
                    else:
                        option = result[0]
                        value = None
                    config[''][option] = value
        return config

    def write_shell(self, filepath, f_out, config):
        """Writes the config object in shell file format"""
        # All the options in the file
        options = [key for key in config[''].keys()]
        # If a previous file exists modify it keeping the comments
        if os.path.exists(self.input_file):
            with open_file_read(self.input_file) as f_in:
                for line in f_in:
                    result = shlex.split(line, True)
                    # If line is not empty or comment
                    if result:
                        # If option=value or option="value" type
                        if '=' in result[0]:
                            option, value = result[0].split('=')
                            if '#' in line:
                                comment = value.split('#', 1)[1]
                                comment = '#' + comment
                            else:
                                comment = ''
                            # If option exists in the new config file
                            if option in options:
                                # If value is different
                                if value != config[''][option]:
                                    value_new = config[''][option]
                                    if value_new is not None:
                                        # Update value
                                        if '"' in line:
                                            value_new = '"' + value_new + '"'
                                        line = option + '=' + value_new + comment + '\n'
                                    else:
                                        # If option changed to option type from option=value type
                                        line = option + comment + '\n'
                                f_out.write(line)
                                # Remove from remaining options list
                                options.remove(option)
                        else:
                            # If option type
                            option = result[0]
                            value = None
                            # If option exists in the new config file
                            if option in options:
                                # If its no longer option type
                                if config[''][option] is not None:
                                    value = config[''][option]
                                    line = option + '=' + value + '\n'
                                f_out.write(line)
                                # Remove from remaining options list
                                options.remove(option)
                    else:
                        # If its empty or comment copy as it is
                        f_out.write(line)
        # If any new options are present
        if options:
            for option in options:
                value = config[''][option]
                # option type entry
                if value is None:
                    line = option + '\n'
                # option=value type entry
                else:
                    line = option + '=' + value + '\n'
                f_out.write(line)

    def write_configparser(self, filepath, f_out, config):
        """Writes/updates the given file with given config object"""
        # All the sections in the file
        sections = config.sections()
        write = True
        section = None
        options = []
        # If a previous file exists modify it keeping the comments
        if os.path.exists(self.input_file):
            with open_file_read(self.input_file) as f_in:
                for line in f_in:
                    # If its a section
                    if line.lstrip().startswith('['):
                        # If any options from preceding section remain write them
                        if options:
                            for option in options:
                                line_new = '  ' + option + ' = ' + config[section][option] + '\n'
                                f_out.write(line_new)
                            options = []
                        if section in sections:
                            # Remove the written section from the list
                            sections.remove(section)
                        section = line.strip()[1:-1]
                        if section in sections:
                            # enable write for all entries in that section
                            write = True
                            options = config.options(section)
                            # write the section
                            f_out.write(line)
                        else:
                            # disable writing until next valid section
                            write = False
                    # If write enabled
                    elif write:
                        value = shlex.split(line, True)
                        # If the line is empty or a comment
                        if not value:
                            f_out.write(line)
                        else:
                            option, value = line.split('=', 1)
                            try:
                                # split any inline comments
                                value, comment = value.split('#', 1)
                                comment = '#' + comment
                            except ValueError:
                                comment = ''
                            if option.strip() in options:
                                if config[section][option.strip()] != value.strip():
                                    value = value.replace(value, config[section][option.strip()])
                                    line = option + '=' + value + comment
                                f_out.write(line)
                                options.remove(option.strip())
        # If any options remain from the preceding section
        if options:
            for option in options:
                line = '  ' + option + ' = ' + config[section][option] + '\n'
                f_out.write(line)
        options = []
        # If any new sections are present
        if section in sections:
            sections.remove(section)
        for section in sections:
            f_out.write('\n[%s]\n' % section)
            options = config.options(section)
            for option in options:
                line = '  ' + option + ' = ' + config[section][option] + '\n'
                f_out.write(line)

def py2_parser(filename):
    """Returns the de-dented ini file from the new format ini"""
    tmp = tempfile.NamedTemporaryFile('rw')
    f_out = open(tmp.name, 'w')
    if os.path.exists(filename):
        with open_file_read(filename) as f_in:
            for line in f_in:
                # The ini format allows for multi-line entries, with the subsequent
                # entries being indented deeper hence simple lstrip() is not appropriate
                if line[:2] == '  ':
                    line = line[2:]
                elif line[0] == '\t':
                    line = line[1:]
                f_out.write(line)
    f_out.flush()
    return tmp
