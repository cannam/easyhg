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

# The value assigned here may be modified during installation, by
# replacing its default value with another one.  We can't compare
# against its default value, because then the comparison text would
# get modified as well.  So, compare using prefix only.
#
easyhg_import_path = 'NO_EASYHG_IMPORT_PATH'
if not easyhg_import_path.startswith('NO_'):
    sys.path.append(easyhg_import_path)

from PyQt4 import QtGui

easyhg_qtapp = None

def uisetup(ui):
    ui.__class__.prompt = easyhg_prompt
    ui.__class__.getpass = easyhg_getpass
    global easyhg_qtapp
    easyhg_qtapp = QtGui.QApplication([])

def easyhg_prompt(self, msg, default="y"):
    if not self.interactive():
        self.write(msg, ' ', default, "\n")
        return default
    if msg == _('user:'):
        msg = _('User:')
    (r,ok) = QtGui.QInputDialog.getText(None, _('Information needed'),
                                        msg, QtGui.QLineEdit.Normal)
    if not ok:
        raise util.Abort(_('response expected'))
    if not r:
        return default
    return r

def easyhg_getpass(self, prompt=None, default=None):
    if not self.interactive():
        return default
    if not prompt or prompt == _('password:'):
        prompt = _('Password:');
    (r,ok) = QtGui.QInputDialog.getText(None, _('Password'), prompt,
                                        QtGui.QLineEdit.Password)
    if not ok:
        raise util.Abort(_('response expected'))
    if not r:
        return default
    return r

