# -*- coding: utf-8 -*-
#
#    EasyMercurial
#
#    Based on hgExplorer by Jari Korhonen
#    Copyright (c) 2010 Jari Korhonen
#    Copyright (c) 2010 Chris Cannam
#    Copyright (c) 2010 Queen Mary, University of London
#    
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation; either version 2 of the
#    License, or (at your option) any later version.  See the file
#    COPYING included with this distribution for more information.

import sys
from mercurial import ui, getpass, util
from mercurial.i18n import _
from PyQt4 import QtGui

# This is a gross hack throughout

easyhg_qtapp = None
easyhg_userprompt = ''
easyhg_pwdprompt = ''

def uisetup(ui):
    ui.__class__.write = easyhg_write
    ui.__class__.prompt = easyhg_prompt
    ui.__class__.getpass = easyhg_getpass
    global easyhg_qtapp, easyhg_pwdprompt, easyhg_userprompt
    easyhg_qtapp = QtGui.QApplication([])
    easyhg_pwdprompt = ''
    easyhg_userprompt = ''

def easyhg_write(self, *args, **opts):
    global easyhg_pwdprompt
    global easyhg_userprompt
    for a in args:
        (pfx, div, sfx) = a.partition(': ');
        if pfx == 'realm' and sfx != '':
            easyhg_userprompt = easyhg_pwdprompt = '<qt>' + sfx + '<br>';
        elif pfx == 'user' and sfx != '':
            easyhg_pwdprompt += _('Password for user') + ' <b>' + sfx + '</b>:';
        sys.stdout.write(str(a))

def easyhg_prompt(self, msg, default="y"):
    if not self.interactive():
        self.write(msg, ' ', default, "\n")
        return default
    if msg == _('user:'):
        msg = _('User:')
    global easyhg_userprompt, easyhg_pwdprompt
    if easyhg_userprompt != '':
        msg = easyhg_userprompt + msg;
    (r,ok) = QtGui.QInputDialog.getText(None, _('Question'),
                                        msg, QtGui.QLineEdit.Normal)
    if not ok:
        raise util.Abort(_('response expected'))
    if not r:
        easyhg_pwdprompt += _('Password:');
        return default
    easyhg_pwdprompt += _('Password for user') + ' <b>' + r + '</b>:';
    return r

def easyhg_getpass(self, prompt=None, default=None):
    if not self.interactive():
        return default
    global easyhg_pwdprompt
    if easyhg_pwdprompt != '':
        msg = easyhg_pwdprompt
    else:
        msg = _('Password:');
    (r,ok) = QtGui.QInputDialog.getText(None, _('Password'), msg,
                                        QtGui.QLineEdit.Password)
    if not ok:
        raise util.Abort(_('response expected'))
    if not r:
        return default
    return r

