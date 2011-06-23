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

import urllib, urllib2, urlparse

from mercurial import ui, getpass, util
try:
    from mercurial.url import passwordmgr
except:
    from mercurial.httprepo import passwordmgr

from mercurial.i18n import _

#!!! do this later? as for Qt? also see notes on demandimport in mercurial_keyring
import keyring

# The value assigned here may be modified during installation, by
# replacing its default value with another one.  We can't compare
# against its default value, because then the comparison text would
# get modified as well.  So, compare using prefix only.
#
easyhg_import_path = 'NO_EASYHG_IMPORT_PATH'
if not easyhg_import_path.startswith('NO_'):
    # We have an installation path: append it twice, once with
    # the Python version suffixed
    version_suffix = "Py" + str(sys.version_info[0]) + "." + str(sys.version_info[1]);
    sys.path.append(easyhg_import_path + "/" + version_suffix)
    sys.path.append(easyhg_import_path)

# Try to load the PyQt4 module that we need.  If this fails, we should
# bail out later (in uisetup), because if we bail out now, Mercurial
# will just continue without us and report success.  The invoking
# application needs to be able to discover whether the module load
# succeeded or not, so we need to ensure that Mercurial itself returns
# failure if it didn't.
#
easyhg_pyqt_ok = True
try:
    from PyQt4 import Qt, QtGui
except ImportError:
    easyhg_pyqt_ok = False

easyhg_qtapp = None

def monkeypatch_method(cls):
    def decorator(func):
        setattr(cls, func.__name__, func)
        return func
    return decorator

def uisetup(ui):
    if not easyhg_pyqt_ok:
        raise util.Abort(_('Failed to load PyQt4 module required by easyhg.py'))
    global easyhg_qtapp
    easyhg_qtapp = QtGui.QApplication([])

orig_find = passwordmgr.find_user_password

# from mercurial_keyring by Marcin Kasperski
def canonical_url(authuri):
    """
    Strips query params from url. Used to convert urls like
    https://repo.machine.com/repos/apps/module?pairs=0000000000000000000000000000000000000000-0000000000000000000000000000000000000000&cmd=between
    to
    https://repo.machine.com/repos/apps/module
    """
    parsed_url = urlparse.urlparse(authuri)
    return "%s://%s%s" % (parsed_url.scheme, parsed_url.netloc,
                          parsed_url.path)

@monkeypatch_method(passwordmgr)
def find_user_password(self, realm, authuri):

    if not self.ui.interactive():
        return orig_find(self, realm, authuri)
    if not easyhg_pyqt_ok:
        return orig_find(self, realm, authuri)

    authinfo = urllib2.HTTPPasswordMgrWithDefaultRealm.find_user_password(
        self, realm, authuri)
    user, passwd = authinfo

    if user and passwd:
        return orig_find(self, realm, authuri)

    self.ui.write("want username and/or password for %s\n" % authuri)

    uri = canonical_url(authuri)

    from_keyring = False
    keyring_key = ''
    if user and not passwd:
        keyring_key = '%s@@%s' % (uri, user)
        self.ui.write("keyring_key is %s" % keyring_key)
#        passwd = keyring.get_password('Mercurial', keyring_key)
        self.ui.write("got passwd: %s\n" % passwd)
        if passwd:
            from_keyring = True

    dialog = QtGui.QDialog()
    layout = QtGui.QGridLayout()
    dialog.setLayout(layout)

    layout.addWidget(QtGui.QLabel(_('Please supply your user name and password for\n%s:') % uri), 0, 0, 1, 2)

    userfield = QtGui.QLineEdit()
    if user:
        userfield.setText(user)
    layout.addWidget(QtGui.QLabel(_('User:')), 1, 0)
    layout.addWidget(userfield, 1, 1)

    passfield = QtGui.QLineEdit()
    passfield.setEchoMode(QtGui.QLineEdit.Password)
    if passwd:
        userfield.setText(passwd)
    layout.addWidget(QtGui.QLabel(_('Password:')), 2, 0)
    layout.addWidget(passfield, 2, 1)

    bb = QtGui.QDialogButtonBox()
    ok = bb.addButton(bb.Ok)
    cancel = bb.addButton(bb.Cancel)
    cancel.setDefault(False)
    cancel.setAutoDefault(False)
    ok.setDefault(True)
    bb.connect(ok, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("accept()"))
    bb.connect(cancel, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("reject()"))
    layout.addWidget(bb, 3, 0, 1, 2)
    
    dialog.setWindowTitle(_('EasyMercurial: Password'))
    dialog.show()
    dialog.raise_()
    ok = dialog.exec_()
    if ok:
        self.ui.write('Dialog accepted\n')
        user = userfield.text()
        passwd = passfield.text()
        if passwd and keyring_key != '' and not from_keyring:
            keyring_key = '%s@@%s' % (uri, user)
            keyring.set_password('Mercurial', keyring_key, passwd)
        self.add_password(realm, authuri, user, passwd)
    else:
        raise util.Abort(_('password entry cancelled'))
    return (user, passwd)


 
