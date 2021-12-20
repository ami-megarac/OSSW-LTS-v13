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
import re
from apparmor.common import AppArmorBug

def AA_OTHER(mode):
    other = set()
    for i in mode:
        other.add('::%s' % i)
    return other

def AA_OTHER_REMOVE(mode):
    other = set()
    for i in mode:
        if '::' in i:
            other.add(i[2:])
    return other

AA_MAY_EXEC = set('x')
AA_MAY_WRITE = set('w')
AA_MAY_READ = set('r')
AA_MAY_APPEND = set('a')
AA_MAY_LINK = set('l')
AA_MAY_LOCK = set('k')
AA_EXEC_MMAP = set('m')
AA_EXEC_UNSAFE = set(['execunsafe'])
AA_EXEC_INHERIT = set('i')
AA_EXEC_UNCONFINED = set('U')
AA_EXEC_PROFILE = set('P')
AA_EXEC_CHILD = set('C')
AA_EXEC_NT = set('N')
AA_LINK_SUBSET = set(['linksubset'])
AA_BARE_FILE_MODE = set(['bare_file_mode'])
#AA_OTHER_SHIFT = 14
#AA_USER_MASK = 16384 - 1

MODE_HASH = {'x': AA_MAY_EXEC, 'X': AA_MAY_EXEC,
             'w': AA_MAY_WRITE, 'W': AA_MAY_WRITE,
             'r': AA_MAY_READ, 'R': AA_MAY_READ,
             'a': AA_MAY_APPEND, 'A': AA_MAY_APPEND,
             'l': AA_MAY_LINK, 'L': AA_MAY_LINK,
             'k': AA_MAY_LOCK, 'K': AA_MAY_LOCK,
             'm': AA_EXEC_MMAP, 'M': AA_EXEC_MMAP,
             'i': AA_EXEC_INHERIT, 'I': AA_EXEC_INHERIT,
             'u': AA_EXEC_UNCONFINED | AA_EXEC_UNSAFE,  # Unconfined + Unsafe
             'U': AA_EXEC_UNCONFINED,
             'p': AA_EXEC_PROFILE | AA_EXEC_UNSAFE,    # Profile + unsafe
             'P': AA_EXEC_PROFILE,
             'c': AA_EXEC_CHILD | AA_EXEC_UNSAFE,  # Child + Unsafe
             'C': AA_EXEC_CHILD,
             'n': AA_EXEC_NT | AA_EXEC_UNSAFE,
             'N': AA_EXEC_NT
             }

LOG_MODE_RE = re.compile('^(r|w|l|m|k|a|x|ix|ux|px|pux|cx|nx|pix|cix|Ux|Px|PUx|Cx|Nx|Pix|Cix)+$')
MODE_MAP_SET = {"r", "w", "l", "m", "k", "a", "x", "i", "u", "p", "c", "n", "I", "U", "P", "C", "N"}

def str_to_mode(string):
    if not string:
        return set()
    user, other = split_log_mode(string)
    if not user:
        user = other

    mode = sub_str_to_mode(user)
    #print(string, mode)
    #print(string, 'other', sub_str_to_mode(other))
    mode |= (AA_OTHER(sub_str_to_mode(other)))
    #print (string, mode)
    #print('str_to_mode:', mode)
    return mode

def sub_str_to_mode(string):
    mode = set()

    for mode_char in string:
        if mode_char in MODE_MAP_SET and MODE_HASH.get(mode_char, False):
            mode |= MODE_HASH[mode_char]
        else:
            raise AppArmorBug("Mode string '%s' contains invalid char '%s'" % (string, mode_char))

    return mode

def split_log_mode(mode):
    #if the mode has a "::", then the left side is the user mode, and the right side is the other mode
    #if not, then the mode is both the user and other mode
    user = ''
    other = ''

    if "::" in mode:
        try:
            user, other = mode.split("::")
        except ValueError as e:
            raise AppArmorBug("Got ValueError '%s' when splitting %s" % (str(e), mode))
    else:
        user = mode
        other = mode

    return user, other

def mode_contains(mode, subset):
    # w implies a
    if mode & AA_MAY_WRITE:
        mode |= AA_MAY_APPEND
    if mode & (AA_OTHER(AA_MAY_WRITE)):
        mode |= (AA_OTHER(AA_MAY_APPEND))

    return (mode & subset) == subset

def validate_log_mode(mode):
    if LOG_MODE_RE.search(mode):
        return True
    else:
        return False

def hide_log_mode(mode):
    mode = mode.replace('::', '')
    return mode

def split_mode(mode):
    user = set()
    for i in mode:
        if not '::' in i:
            user.add(i)
    other = mode - user
    other = AA_OTHER_REMOVE(other)
    return user, other

def log_str_to_mode(profile, string, nt_name):
    mode = str_to_mode(string)
    # If contains nx and nix
    #print (profile, string, nt_name)
    if mode_contains(mode, str_to_mode('Nx')):
        # Transform to px, cx
        match = re.search('(.+?)//(.+?)', nt_name)
        if match:
            lprofile, lhat = match.groups()
            tmode = 0

            if lprofile == profile:
                if mode & AA_MAY_EXEC:
                    tmode = str_to_mode('Cx::')
                if mode & AA_OTHER(AA_MAY_EXEC):
                    tmode |= str_to_mode('Cx')
                nt_name = lhat
            else:
                if mode & AA_MAY_EXEC:
                    tmode = str_to_mode('Px::')
                if mode & AA_OTHER(AA_MAY_EXEC):
                    tmode |= str_to_mode('Px')
                nt_name = lhat

            mode = mode - str_to_mode('Nx')
            mode |= tmode

    return mode, nt_name
