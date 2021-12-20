#!/usr/bin/python
#
#    Copyright (C) 2012 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    Written by Steve Beattie <steve@nxnw.org>, based on work by
#    Christian Boltz <apparmor@cboltz.de>

from __future__ import with_statement
import re
import subprocess
import sys

# dangerous capabilities
danger_caps = ["audit_control",
               "audit_write",
               "mac_override",
               "mac_admin",
               "set_fcap",
               "sys_admin",
               "sys_module",
               "sys_rawio"]


def cmd(command, input=None, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, stdin=None, timeout=None):
    '''Try to execute given command (array) and return its stdout, or
    return a textual error if it failed.'''

    try:
        sp = subprocess.Popen(command, stdin=stdin, stdout=stdout, stderr=stderr, close_fds=True, universal_newlines=True)
    except OSError as ex:
        return [127, str(ex)]

    out, outerr = sp.communicate(input)

    # Handle redirection of stdout
    if out is None:
        out = ''
    # Handle redirection of stderr
    if outerr is None:
        outerr = ''
    return [sp.returncode, out + outerr]

# get capabilities list
(rc, output) = cmd(['make', '-s', '--no-print-directory', 'list_capabilities'])
if rc != 0:
    sys.stderr.write("make list_capabilities failed: " + output)
    exit(rc)

capabilities = re.sub('CAP_', '', output.strip()).lower().split(" ")
benign_caps = []
for cap in capabilities:
    if cap not in danger_caps:
        benign_caps.append(cap)

# get network protos list
(rc, output) = cmd(['make', '-s', '--no-print-directory', 'list_af_names'])
if rc != 0:
    sys.stderr.write("make list_af_names failed: " + output)
    exit(rc)

af_names = []
af_pairs = re.sub('AF_', '', output.strip()).lower().split(",")
for af_pair in af_pairs:
    af_name = af_pair.lstrip().split(" ")[0]
    # skip max af name definition
    if len(af_name) > 0 and af_name != "max":
        af_names.append(af_name)

# TODO: does a "debug" flag exist? Listed in apparmor.vim.in sdFlagKey,
# but not in aa_flags...
# -> currently (2011-01-11) not, but might come back

aa_network_types = r'\s+tcp|\s+udp|\s+icmp'

aa_flags = ['complain',
            'audit',
            'attach_disconnected',
            'no_attach_disconnected',
            'chroot_attach',
            'chroot_no_attach',
            'chroot_relative',
            'namespace_relative',
            'mediate_deleted',
            'delegate_deleted']

filename = r'(\/|\@\{\S*\})\S*'

aa_regex_map = {
    'FILENAME':         filename,
    'FILE':             r'\v^\s*(audit\s+)?(deny\s+|allow\s+)?(owner\s+|other\s+)?' + filename + r'\s+',  # Start of a file rule
                        # (whitespace_+_, owner etc. flag_?_, filename pattern, whitespace_+_)
    'DENYFILE':         r'\v^\s*(audit\s+)?deny\s+(owner\s+|other\s+)?' + filename + r'\s+',  # deny, otherwise like FILE
    'auditdenyowner':   r'(audit\s+)?(deny\s+|allow\s+)?(owner\s+|other\s+)?',
    'audit_DENY_owner': r'(audit\s+)?deny\s+(owner\s+|other\s+)?',  # must include "deny", otherwise like auditdenyowner
    'auditdeny':        r'(audit\s+)?(deny\s+|allow\s+)?',
    'EOL':              r'\s*,(\s*$|(\s*#.*$)\@=)',  # End of a line (whitespace_?_, comma, whitespace_?_ comment.*)
    'TRANSITION':       r'(\s+-\>\s+\S+)?',
    'sdKapKey':         " ".join(benign_caps),
    'sdKapKeyDanger':   " ".join(danger_caps),
    'sdKapKeyRegex':    "|".join(capabilities),
    'sdNetworkType':    aa_network_types,
    'sdNetworkProto':   "|".join(af_names),
    'flags':            r'((flags\s*\=\s*)?\(\s*(' + '|'.join(aa_flags) + r')(\s*,\s*(' + '|'.join(aa_flags) + r'))*\s*\)\s+)',
}


def my_repl(matchobj):
    matchobj.group(1)
    if matchobj.group(1) in aa_regex_map:
        return aa_regex_map[matchobj.group(1)]

    return matchobj.group(0)


def create_file_rule(highlighting, permissions, comment, denyrule=0):

    if denyrule == 0:
        keywords = '@@auditdenyowner@@'
    else:
        keywords = '@@audit_DENY_owner@@'  # TODO: not defined yet, will be '(audit\s+)?deny\s+(owner\s+)?'

    sniplet = ''
    sniplet = sniplet + "\n" + '" ' + comment + "\n"

    prefix = r'syn match  ' + highlighting + r' /\v^\s*' + keywords
    suffix = r'@@EOL@@/ contains=sdGlob,sdComment nextgroup=@sdEntry,sdComment,sdError,sdInclude' + "\n"
    # filename without quotes
    sniplet = sniplet + prefix + r'@@FILENAME@@\s+' + permissions + suffix
    # filename with quotes
    sniplet = sniplet + prefix + r'"@@FILENAME@@"\s+' + permissions + suffix
    # filename without quotes, reverse syntax
    sniplet = sniplet + prefix + permissions + r'\s+@@FILENAME@@' + suffix
    # filename with quotes, reverse syntax
    sniplet = sniplet + prefix + permissions + r'\s+"@@FILENAME@@"+' + suffix

    return sniplet


filerule = ''
filerule = filerule + create_file_rule('sdEntryWriteExec ', r'(l|r|w|a|m|k|[iuUpPcC]x)+@@TRANSITION@@', 'write + exec/mmap - danger! (known bug: accepts aw to keep things simple)')
filerule = filerule + create_file_rule('sdEntryUX',  r'(r|m|k|ux|pux)+@@TRANSITION@@',  'ux(mr) - unconstrained entry, flag the line red. also includes pux which is unconstrained if no profile exists')
filerule = filerule + create_file_rule('sdEntryUXe', r'(r|m|k|Ux|PUx)+@@TRANSITION@@',  'Ux(mr) and PUx(mr) - like ux + clean environment')
filerule = filerule + create_file_rule('sdEntryPX',  r'(r|m|k|px|cx|pix|cix)+@@TRANSITION@@',  'px/cx/pix/cix(mrk) - standard exec entry, flag the line blue')
filerule = filerule + create_file_rule('sdEntryPXe', r'(r|m|k|Px|Cx|Pix|Cix)+@@TRANSITION@@', 'Px/Cx/Pix/Cix(mrk) - like px/cx + clean environment')
filerule = filerule + create_file_rule('sdEntryIX',  r'(r|m|k|ix)+',  'ix(mr) - standard exec entry, flag the line green')
filerule = filerule + create_file_rule('sdEntryM',   r'(r|m|k)+',  'mr - mmap with PROT_EXEC')

filerule = filerule + create_file_rule('sdEntryM',   r'(r|m|k|x)+',  'special case: deny x is allowed (does not need to be ix, px, ux or cx)', 1)
#syn match  sdEntryM /@@DENYFILE@@(r|m|k|x)+@@EOL@@/ contains=sdGlob,sdComment nextgroup=@sdEntry,sdComment,sdError,sdInclude


filerule = filerule + create_file_rule('sdError',    r'\S*(w\S*a|a\S*w)\S*',  'write + append is an error')
filerule = filerule + create_file_rule('sdEntryW',   r'(l|r|w|k)+',  'write entry, flag the line yellow')
filerule = filerule + create_file_rule('sdEntryW',   r'(l|r|a|k)+',  'append entry, flag the line yellow')
filerule = filerule + create_file_rule('sdEntryK',   r'[rlk]+',  'read entry + locking, currently no highlighting')
filerule = filerule + create_file_rule('sdEntryR',   r'[rl]+',  'read entry, no highlighting')

# " special case: deny x is allowed (doesn't need to be ix, px, ux or cx)
# syn match  sdEntryM /@@DENYFILE@@(r|m|k|x)+@@EOL@@/ contains=sdGlob,sdComment nextgroup=@sdEntry,sdComment,sdError,sdInclude

# " TODO: Support filenames enclosed in quotes ("/home/foo/My Documents/") - ideally by only allowing quotes pair-wise


regex = "@@(" + "|".join(aa_regex_map) + ")@@"

sys.stdout.write('" generated from apparmor.vim.in by create-apparmor.vim.py\n')
sys.stdout.write('" do not edit this file - edit apparmor.vim.in or create-apparmor.vim.py instead' + "\n\n")

with open("apparmor.vim.in") as template:
    for line in template:
        line = re.sub(regex, my_repl, line.rstrip())
        sys.stdout.write('%s\n' % line)

sys.stdout.write("\n\n\n\n")

sys.stdout.write('" file rules added with create_file_rule()\n')
sys.stdout.write(re.sub(regex, my_repl, filerule) + '\n')
