# ------------------------------------------------------------------
#
#    Copyright (C) 2011-2013 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

from apparmor.common import AppArmorException, debug, error, msg, cmd
import apparmor.easyprof
import optparse
import os
import pwd
import re
import signal
import socket
import sys
import tempfile
import time

def check_requirements(binary):
    '''Verify necessary software is installed'''
    exes = ['xset',        # for detecting free X display
            'aa-easyprof', # for templates
            'aa-exec',     # for changing profile
            'sudo',        # eventually get rid of this
            'pkexec',      # eventually get rid of this
            binary]

    for e in exes:
        debug("Searching for '%s'" % e)
        rc, report = cmd(['which', e])
        if rc != 0:
            error("Could not find '%s'" % e, do_exit=False)
            return False

    return True

def parse_args(args=None, parser=None):
    '''Parse arguments'''
    if parser == None:
        parser = optparse.OptionParser()

    parser.add_option('-X', '--with-x',
                      dest='withx',
                      default=False,
                      help='Run in isolated X server',
                      action='store_true')
    parser.add_option('--with-xserver',
                      dest='xserver',
                      default='xpra',
                      help='Nested X server to use: xpra (default), xpra3d, xephyr')
    parser.add_option('--with-clipboard',
                      dest='with_clipboard',
                      default=False,
                      help='Allow clipboard access',
                      action='store_true')
    parser.add_option('--with-xauthority',
                      dest='xauthority',
                      default=None,
                      help='Specify Xauthority file to use')
    parser.add_option('-d', '--debug',
                      dest='debug',
                      default=False,
                      help='Show debug messages',
                      action='store_true')
    parser.add_option('--with-xephyr-geometry',
                      dest='xephyr_geometry',
                      default=None,
                      help='Geometry for Xephyr window')
    parser.add_option('--profile',
                      dest='profile',
                      default=None,
                      help='Specify an existing profile (see aa-status)')

    (my_opt, my_args) = parser.parse_args()
    if my_opt.debug:
        apparmor.common.DEBUGGING = True

    valid_xservers = ['xpra', 'xpra3d', 'xephyr']
    if my_opt.withx and my_opt.xserver.lower() not in valid_xservers:
            error("Invalid server '%s'. Use one of: %s" % (my_opt.xserver, \
                                                           ", ".join(valid_xservers)))

    if my_opt.withx:
        if my_opt.xephyr_geometry and my_opt.xserver.lower() != "xephyr":
            error("Invalid option --with-xephyr-geometry with '%s'" % my_opt.xserver)
        elif my_opt.with_clipboard and my_opt.xserver.lower() == "xephyr":
            error("Clipboard not supported with '%s'" % my_opt.xserver)

    if my_opt.template == "default":
        if my_opt.withx:
            my_opt.template = "sandbox-x"
        else:
            my_opt.template = "sandbox"

    return (my_opt, my_args)

def gen_policy_name(binary):
    '''Generate a temporary policy based on the binary name'''
    return "sandbox-%s%s" % (pwd.getpwuid(os.geteuid())[0],
                              re.sub(r'/', '_', binary))

def set_environ(env):
    keys = env.keys()
    keys.sort()
    for k in keys:
        msg("Using: %s=%s" % (k, env[k]))
        os.environ[k] = env[k]

def aa_exec(command, opt, environ={}, verify_rules=[]):
    '''Execute binary under specified policy'''
    if opt.profile != None:
        policy_name = opt.profile
    else:
        opt.ensure_value("template_var", None)
        opt.ensure_value("name", None)
        opt.ensure_value("comment", None)
        opt.ensure_value("author", None)
        opt.ensure_value("copyright", None)

        binary = command[0]
        policy_name = gen_policy_name(binary)
        easyp = apparmor.easyprof.AppArmorEasyProfile(binary, opt)
        params = apparmor.easyprof.gen_policy_params(policy_name, opt)
        policy = easyp.gen_policy(**params)
        debug("\n%s" % policy)

        tmp = tempfile.NamedTemporaryFile(prefix = '%s-' % policy_name)
        if sys.version_info[0] >= 3:
            tmp.write(bytes(policy, 'utf-8'))
        else:
            tmp.write(policy)
        tmp.flush()

        debug("using '%s' template" % opt.template)
        # TODO: get rid of this
        if opt.withx:
            rc, report = cmd(['pkexec', 'apparmor_parser', '-r', '%s' % tmp.name])
        else:
            rc, report = cmd(['sudo', 'apparmor_parser', '-r', tmp.name])
        if rc != 0:
            raise AppArmorException("Could not load policy")

        rc, report = cmd(['sudo', 'apparmor_parser', '-p', tmp.name])
        if rc != 0:
            raise AppArmorException("Could not dump policy")

        # Make sure the dynamic profile has the appropriate line for X
        for r in verify_rules:
            found = False
            for line in report.splitlines():
                line = line.strip()
                if r == line:
                    found = True
                    break
            if not found:
                raise AppArmorException("Could not find required rule: %s" % r)

    set_environ(environ)
    args = ['aa-exec', '-p', policy_name, '--'] + command
    rc, report = cmd(args)
    return rc, report

def run_sandbox(command, opt):
    '''Run application'''
    # aa-exec
    rc, report = aa_exec(command, opt)
    return rc, report

class SandboxXserver():
    def __init__(self, title, geometry=None,
                              driver=None,
                              xauth=None,
                              clipboard=False):
        self.geometry = geometry
        self.title = title
        self.pids = []
        self.driver = driver
        self.clipboard = clipboard
        self.tempfiles = []
        self.timeout = 5 # used by xauth and for server starts

        # preserve our environment
        self.old_environ = dict()
        for env in ['DISPLAY', 'XAUTHORITY', 'UBUNTU_MENUPROXY',
                    'QT_X11_NO_NATIVE_MENUBAR', 'LIBOVERLAY_SCROLLBAR']:
            if env in os.environ:
                self.old_environ[env] = os.environ[env]

        # prepare the new environment
        self.display, self.xauth = self.find_free_x_display()
        if xauth:
            abs_xauth = os.path.expanduser(xauth)
            if os.path.expanduser("~/.Xauthority") == abs_xauth:
                raise AppArmorException("Trusted Xauthority file specified. Aborting")
            self.xauth = abs_xauth
        self.new_environ = dict()
        self.new_environ['DISPLAY'] = self.display
        self.new_environ['XAUTHORITY'] = self.xauth
        # Disable the global menu for now
        self.new_environ["UBUNTU_MENUPROXY"] = ""
        self.new_environ["QT_X11_NO_NATIVE_MENUBAR"] = "1"
        # Disable the overlay scrollbar for now-- they don't track correctly
        self.new_environ["LIBOVERLAY_SCROLLBAR"] = "0"

    def cleanup(self):
        '''Cleanup our forked pids, reset the environment, etc'''
        self.pids.reverse()
        debug(self.pids)
        for pid in self.pids:
            # Kill server with TERM
            debug("kill %d" % pid)
            os.kill(pid, signal.SIGTERM)

        for pid in self.pids:
            # Shoot the server dead
            debug("kill -9 %d" % pid)
            os.kill(pid, signal.SIGKILL)

        for t in self.tempfiles:
            if os.path.exists(t):
                os.unlink(t)

        if os.path.exists(self.xauth):
            os.unlink(self.xauth)

        # Reset our environment
        set_environ(self.old_environ)

    def find_free_x_display(self):
        '''Find a free X display'''
        old_lang = None
        if 'LANG' in os.environ:
            old_lang = os.environ['LANG']
        os.environ['LANG'] = 'C'

        display = ""
        current = self.old_environ["DISPLAY"]
        for i in range(1,257): # TODO: this puts an artificial limit of 256
                               #       sandboxed applications
            tmp = ":%d" % i
            os.environ["DISPLAY"] = tmp
            rc, report = cmd(['xset', '-q'])
            if rc != 0 and 'Invalid MIT-MAGIC-COOKIE-1' not in report:
                display = tmp
                break

        if old_lang:
            os.environ['LANG'] = old_lang

        os.environ["DISPLAY"] = current
        if display == "":
            raise AppArmorException("Could not find available X display")

        # Use dedicated .Xauthority file
        xauth = os.path.join(os.path.expanduser('~'), \
                             '.Xauthority-sandbox%s' % display.split(':')[1])

        return display, xauth

    def generate_title(self):
        return "(Sandbox%s) %s" % (self.display, self.title)

    def verify_host_setup(self):
        '''Make sure we have everything we need'''
        old_lang = None
        if 'LANG' in os.environ:
            old_lang = os.environ['LANG']

        os.environ['LANG'] = 'C'
        rc, report = cmd(['xhost'])

        if old_lang:
            os.environ['LANG'] = old_lang

        if rc != 0:
            raise AppArmorException("'xhost' exited with error")
        if 'access control enabled' not in report:
            raise AppArmorException("Access control currently disabled. Please enable with 'xhost -'")
        username = pwd.getpwuid(os.geteuid())[0]
        if ':localuser:%s' % username in report:
            raise AppArmorException("Access control allows '%s' full access. Please see 'man aa-sandbox' for details" % username)

    def start(self):
        '''Start a nested X server (need to override)'''
        # clean up the old one
        if os.path.exists(self.xauth):
            os.unlink(self.xauth)
        rc, cookie = cmd(['mcookie'])
        if rc != 0:
            raise AppArmorException("Could not generate magic cookie")

        rc, out = cmd(['xauth', '-f', self.xauth, \
                       'add', \
                       self.display, \
                       'MIT-MAGIC-COOKIE-1', \
                       cookie.strip()])
        if rc != 0:
            raise AppArmorException("Could not generate '%s'" % self.display)


class SandboxXephyr(SandboxXserver):
    def start(self):
        for e in ['Xephyr', 'matchbox-window-manager']:
            debug("Searching for '%s'" % e)
            rc, report = cmd(['which', e])
            if rc != 0:
                raise AppArmorException("Could not find '%s'" % e)

        '''Run any setup code'''
        SandboxXserver.start(self)

        '''Start a Xephyr server'''
        listener_x = os.fork()
        if listener_x == 0:
            # TODO: break into config file? Which are needed?
            x_exts = ['-extension', 'GLX',
                      '-extension', 'MIT-SHM',
                      '-extension', 'RENDER',
                      '-extension', 'SECURITY',
                      '-extension', 'DAMAGE'
                     ]
            # verify_these
            x_extra_args = ['-host-cursor', # less secure?
                            '-fakexa',      # for games? seems not needed
                            '-nodri',       # more secure?
                           ]

            if not self.geometry:
                self.geometry = "640x480"
            x_args = ['-nolisten', 'tcp',
                      '-screen', self.geometry,
                      '-br',        # black background
                      '-reset',     # reset after last client exists
                      '-terminate', # terminate at server reset
                      '-title', self.generate_title(),
                      ] + x_exts + x_extra_args

            args = ['/usr/bin/Xephyr'] + x_args + [self.display]
            debug(" ".join(args))
            os.execv(args[0], args)
            sys.exit(0)
        self.pids.append(listener_x)

        time.sleep(1) # FIXME: detect if running

        # Next, start the window manager
        sys.stdout.flush()
        os.chdir(os.environ["HOME"])
        listener_wm = os.fork()
        if listener_wm == 0:
            # update environment
            set_environ(self.new_environ)

            args = ['/usr/bin/matchbox-window-manager', '-use_titlebar', 'no']
            debug(" ".join(args))
            cmd(args)
            sys.exit(0)

        self.pids.append(listener_wm)
        time.sleep(1) # FIXME: detect if running


class SandboxXpra(SandboxXserver):
    def cleanup(self):
        sys.stderr.flush()
        listener = os.fork()
        if listener == 0:
            args = ['/usr/bin/xpra', 'stop', self.display]
            debug(" ".join(args))
            os.execv(args[0], args)
            sys.exit(0)
        time.sleep(2)

        # Annoyingly, xpra doesn't clean up itself well if the application
        # failed for some reason. Try to account for that.
        rc, report = cmd(['ps', 'auxww'])
        for line in report.splitlines():
            if '-for-Xpra-%s' % self.display in line:
                self.pids.append(int(line.split()[1]))

        SandboxXserver.cleanup(self)

    def _get_xvfb_args(self):
        '''Setup xvfb arguments'''
        # Debugging tip (can also use glxinfo):
        # $ xdpyinfo > /tmp/native
        # $ aa-sandbox -X -t sandbox-x /usr/bin/xdpyinfo > /tmp/nested
        # $ diff -Naur /tmp/native /tmp/nested

        xvfb_args = []

        if self.driver == None:
            # The default from the man page, but be explicit in what we enable
            xvfb_args.append('--xvfb=Xvfb')
            xvfb_args.append('-screen 0 3840x2560x24+32')
            xvfb_args.append('-nolisten tcp')
            xvfb_args.append('-noreset')
            xvfb_args.append('-auth %s' % self.new_environ['XAUTHORITY'])
            xvfb_args.append('+extension Composite')
            xvfb_args.append('+extension SECURITY')
            xvfb_args.append('-extension GLX')
        elif self.driver == 'xdummy':
            # The dummy driver allows us to use GLX, etc. See:
            # http://xpra.org/Xdummy.html
            conf = '''# /usr/share/doc/xpra/examples/dummy.xorg.conf.gz
# http://xpra.org/Xdummy.html
##Xdummy:##
Section "ServerFlags"
  Option "DontVTSwitch" "true"
  Option "AllowMouseOpenFail" "true"
  Option "PciForceNone" "true"
  Option "AutoEnableDevices" "false"
  Option "AutoAddDevices" "false"
EndSection


##Xdummy:##
Section "InputDevice"
  Identifier "NoMouse"
  Option "CorePointer" "true"
  Driver "void"
EndSection

Section "InputDevice"
  Identifier "NoKeyboard"
  Option "CoreKeyboard" "true"
  Driver "void"
EndSection

##Xdummy:##
Section "Device"
  Identifier "Videocard0"
  Driver "dummy"
  # In kByte
  #VideoRam 4096000
  #VideoRam 256000
  # This should be good for 3840*2560*32bpp: http://winswitch.org/trac/ticket/140
  VideoRam 64000
EndSection

##Xdummy:##
Section "Monitor"
  Identifier "Monitor0"
  HorizSync   10.0 - 300.0
  VertRefresh 10.0 - 200.0
  DisplaySize 4335 1084
  #The following modeline is invalid (calculator overflowed):
  #Modeline "32000x32000@0" -38917.43 32000 32032 -115848 -115816 32000 32775 32826 33601
  Modeline "16384x8192@10" 2101.93 16384 16416 24400 24432 8192 8390 8403 8602
  Modeline "8192x4096@10" 424.46 8192 8224 9832 9864 4096 4195 4202 4301
  Modeline "5120x3200@10" 199.75 5120 5152 5904 5936 3200 3277 3283 3361
  Modeline "3840x2880@10" 133.43 3840 3872 4376 4408 2880 2950 2955 3025
  Modeline "3840x2560@10" 116.93 3840 3872 4312 4344 2560 2622 2627 2689
  Modeline "3840x2048@10" 91.45 3840 3872 4216 4248 2048 2097 2101 2151
  Modeline "2048x2048@10" 49.47 2048 2080 2264 2296 2048 2097 2101 2151
  Modeline "2560x1600@10" 47.12 2560 2592 2768 2800 1600 1639 1642 1681
  Modeline "1920x1200@10" 26.28 1920 1952 2048 2080 1200 1229 1231 1261
  Modeline "1920x1080@10" 23.53 1920 1952 2040 2072 1080 1106 1108 1135
  Modeline "1680x1050@10" 20.08 1680 1712 1784 1816 1050 1075 1077 1103
  Modeline "1600x900@20" 33.92 1600 1632 1760 1792 900 921 924 946
  Modeline "1440x900@20" 30.66 1440 1472 1584 1616 900 921 924 946
  Modeline "1360x768@20" 24.49 1360 1392 1480 1512 768 786 789 807
  #common resolutions for android devices (both orientations):
  Modeline "800x1280@20" 25.89 800 832 928 960 1280 1310 1315 1345
  Modeline "1280x800@20" 24.15 1280 1312 1400 1432 800 819 822 841
  Modeline "720x1280@25" 30.22 720 752 864 896 1280 1309 1315 1345
  Modeline "1280x720@25" 27.41 1280 1312 1416 1448 720 737 740 757
  Modeline "768x1024@25" 24.93 768 800 888 920 1024 1047 1052 1076
  Modeline "1024x768@25" 23.77 1024 1056 1144 1176 768 785 789 807
  Modeline "600x1024@25" 19.90 600 632 704 736 1024 1047 1052 1076
  Modeline "1024x600@25" 18.26 1024 1056 1120 1152 600 614 617 631
  Modeline "536x960@25" 16.74 536 568 624 656 960 982 986 1009
  Modeline "960x536@25" 15.23 960 992 1048 1080 536 548 551 563
  Modeline "600x800@25" 15.17 600 632 688 720 800 818 822 841
  Modeline "800x600@25" 14.50 800 832 880 912 600 614 617 631
  Modeline "480x854@25" 13.34 480 512 560 592 854 873 877 897
  Modeline "848x480@25" 12.09 848 880 920 952 480 491 493 505
  Modeline "480x800@25" 12.43 480 512 552 584 800 818 822 841
  Modeline "800x480@25" 11.46 800 832 872 904 480 491 493 505
  Modeline "320x480@50" 10.73 320 352 392 424 480 490 494 505
  Modeline "480x320@50" 9.79 480 512 544 576 320 327 330 337
  Modeline "240x400@50" 6.96 240 272 296 328 400 408 412 421
  Modeline "400x240@50" 6.17 400 432 448 480 240 245 247 253
  Modeline "240x320@50" 5.47 240 272 288 320 320 327 330 337
  Modeline "320x240@50" 5.10 320 352 368 400 240 245 247 253
  #resolutions for android devices (both orientations)
  #minus the status bar
  #38px status bar (and width rounded up)
  Modeline "800x1242@20" 25.03 800 832 920 952 1242 1271 1275 1305
  Modeline "1280x762@20" 22.93 1280 1312 1392 1424 762 780 783 801
  Modeline "720x1242@25" 29.20 720 752 856 888 1242 1271 1276 1305
  Modeline "1280x682@25" 25.85 1280 1312 1408 1440 682 698 701 717
  Modeline "768x986@25" 23.90 768 800 888 920 986 1009 1013 1036
  Modeline "1024x730@25" 22.50 1024 1056 1136 1168 730 747 750 767
  Modeline "600x986@25" 19.07 600 632 704 736 986 1009 1013 1036
  Modeline "1024x562@25" 17.03 1024 1056 1120 1152 562 575 578 591
  Modeline "536x922@25" 16.01 536 568 624 656 922 943 947 969
  Modeline "960x498@25" 14.09 960 992 1040 1072 498 509 511 523
  Modeline "600x762@25" 14.39 600 632 680 712 762 779 783 801
  Modeline "800x562@25" 13.52 800 832 880 912 562 575 578 591
  Modeline "480x810@25" 12.59 480 512 552 584 810 828 832 851
  Modeline "848x442@25" 11.09 848 880 920 952 442 452 454 465
  Modeline "480x762@25" 11.79 480 512 552 584 762 779 783 801
  Modeline "800x442@25" 10.51 800 832 864 896 442 452 454 465
  #32px status bar (no need for rounding):
  Modeline "320x448@50" 9.93 320 352 384 416 448 457 461 471
  Modeline "480x288@50" 8.75 480 512 544 576 288 294 297 303
  #24px status bar:
  Modeline "240x376@50" 6.49 240 272 296 328 376 384 387 395
  Modeline "400x216@50" 5.50 400 432 448 480 216 220 222 227
  Modeline "240x296@50" 5.02 240 272 288 320 296 302 305 311
  Modeline "320x216@50" 4.55 320 352 368 400 216 220 222 227
EndSection

##Xdummy:##
Section "Screen"
  Identifier "Screen0"
  Device "Videocard0"
  Monitor "Monitor0"
  DefaultDepth 24
  SubSection "Display"
    Viewport 0 0
    Depth 24
    Modes "32000x32000" "16384x8192" "8192x4096" "5120x3200" "3840x2880" "3840x2560" "3840x2048" "2048x2048" "2560x1600" "1920x1440" "1920x1200" "1920x1080" "1600x1200" "1680x1050" "1600x900" "1400x1050" "1440x900" "1280x1024" "1366x768" "1280x800" "1024x768" "1024x600" "800x600" "320x200"
    #Virtual 32000 32000
    #Virtual 16384 8192
    #Virtual 8192 4096
    # http://winswitch.org/trac/ticket/140
    Virtual 3840 2560
  EndSubSection
EndSection

Section "ServerLayout"
  Identifier   "dummy_layout"
  Screen       "screen0"
  InputDevice  "NoMouse"
  InputDevice  "NoKeyboard"
EndSection
'''

            tmp, xorg_conf = tempfile.mkstemp(prefix='aa-sandbox-xorg.conf-')
            self.tempfiles.append(xorg_conf)
            if sys.version_info[0] >= 3:
                os.write(tmp, bytes(conf, 'utf-8'))
            else:
                os.write(tmp, conf)
            os.close(tmp)

            xvfb_args.append('--xvfb=Xorg')
            xvfb_args.append('-dpi 96') # https://www.xpra.org/trac/ticket/163
            xvfb_args.append('-nolisten tcp')
            xvfb_args.append('-noreset')
            xvfb_args.append('-logfile %s' % os.path.expanduser('~/.xpra/%s.log' % self.display))
            xvfb_args.append('-auth %s' % self.new_environ['XAUTHORITY'])
            xvfb_args.append('-config %s' % xorg_conf)
            extensions = ['Composite', 'GLX', 'RANDR', 'RENDER', 'SECURITY']
            for i in extensions:
                xvfb_args.append('+extension %s' % i)
        else:
            raise AppArmorException("Unsupported X driver '%s'" % self.driver)

        return xvfb_args

    def start(self):
        debug("Searching for '%s'" % 'xpra')
        rc, report = cmd(['which', 'xpra'])
        if rc != 0:
            raise AppArmorException("Could not find '%s'" % 'xpra')

        if self.driver == "xdummy":
            # FIXME: is there a better way we can detect this?
            drv = "/usr/lib/xorg/modules/drivers/dummy_drv.so"
            debug("Searching for '%s'" % drv)
            if not os.path.exists(drv):
                raise AppArmorException("Could not find '%s'" % drv)

        '''Run any setup code'''
        SandboxXserver.start(self)

        xvfb_args = self._get_xvfb_args()
        listener_x = os.fork()
        if listener_x == 0:
            os.environ['XAUTHORITY'] = self.xauth

            # This will clean out any dead sessions
            cmd(['xpra', 'list'])

            x_args = ['--no-daemon',
                      #'--no-mmap', # for security?
                      '--no-pulseaudio']
            if not self.clipboard:
                x_args.append('--no-clipboard')

            if xvfb_args != '':
                x_args.append(" ".join(xvfb_args))

            args = ['/usr/bin/xpra', 'start', self.display] + x_args
            debug(" ".join(args))
            sys.stderr.flush()
            os.execv(args[0], args)
            sys.exit(0)
        self.pids.append(listener_x)

        started = False

        # We need to wait for the xpra socket to exist before attaching
        fn = os.path.join(os.environ['HOME'], '.xpra', '%s-%s' % \
                          (socket.gethostname(), self.display.split(':')[1]))
        for i in range(self.timeout * 2): # up to self.timeout seconds to start
            if os.path.exists(fn):
                debug("Found '%s'! Proceeding to attach" % fn)
                break
            debug("'%s' doesn't exist yet, waiting" % fn)
            time.sleep(0.5)

        if not os.path.exists(fn):
            sys.stdout.flush()
            self.cleanup()
            raise AppArmorException("Could not start xpra (try again with -d)")

        for i in range(self.timeout): # Up to self.timeout seconds to start
            rc, out = cmd(['xpra', 'list'])

            if 'DEAD session at %s' % self.display in out:
                error("xpra session at '%s' died" % self.display, do_exit=False)
                break

            search = 'LIVE session at %s' % self.display
            if search in out:
                started = True
                break
            time.sleep(0.5)
            debug("Could not find '%s' in:\n" % search)
            debug(out)

        if not started:
            sys.stdout.flush()
            self.cleanup()
            raise AppArmorException("Could not start xpra (try again with -d)")

        # Next, attach to xpra
        sys.stdout.flush()
        os.chdir(os.environ["HOME"])
        listener_attach = os.fork()
        if listener_attach == 0:
            args = ['/usr/bin/xpra', 'attach', self.display,
                                     '--title=%s' % self.generate_title(),
                                     #'--no-mmap', # for security?
                                     '--no-tray',
                                     '--no-pulseaudio']
            if not self.clipboard:
                args.append('--no-clipboard')

            debug(" ".join(args))
            sys.stderr.flush()
            os.execv(args[0], args)
            sys.exit(0)

        self.pids.append(listener_attach)

        # Make sure that a client has attached
        for i in range(self.timeout): # up to self.timeout seconds to attach
            time.sleep(1)
            rc, out = cmd (['xpra', 'info', self.display])
            search = 'clients=1'
            if search in out:
                debug("Client successfully attached!")
                break
            debug("Could not find '%s' in:\n" % search)
            debug(out)

        msg("TODO: filter '~/.xpra/run-xpra'")


def run_xsandbox(command, opt):
    '''Run X application in a sandbox'''
    old_cwd = os.getcwd()

    # first, start X
    if opt.xserver.lower() == "xephyr":
        x = SandboxXephyr(command[0], geometry=opt.xephyr_geometry,
                                      xauth=opt.xauthority)
    elif opt.xserver.lower() == "xpra3d":
        x = SandboxXpra(command[0], geometry=None,
                                    driver="xdummy",
                                    xauth=opt.xauthority,
                                    clipboard=opt.with_clipboard)
    else:
        x = SandboxXpra(command[0], geometry=None,
                                    xauth=opt.xauthority,
                                    clipboard=opt.with_clipboard)

    x.verify_host_setup()

    # Debug: show old environment
    keys = x.old_environ.keys()
    keys.sort()
    for k in keys:
        debug ("Old: %s=%s" % (k, x.old_environ[k]))

    try:
        x.start()
    except Exception as e:
        error(e)
    os.chdir(old_cwd)

    if not opt.read_path:
        opt.read_path = []
    opt.read_path.append(x.xauth)

    # Only used with dynamic profiles
    required_rules = ['audit deny @{HOME}/.Xauthority mrwlk,']

    # aa-exec
    try:
        rc, report = aa_exec(command, opt, x.new_environ, required_rules)
    except Exception:
        x.cleanup()
        raise
    x.cleanup()

    return rc, report
