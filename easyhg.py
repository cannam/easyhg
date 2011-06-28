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

import sys, os, stat, urllib, urllib2, urlparse

from mercurial.i18n import _
from mercurial import ui, util, error
try:
    from mercurial.url import passwordmgr
except:
    from mercurial.httprepo import passwordmgr

# The value assigned here may be modified during installation, by
# replacing its default value with another one.  We can't compare
# against its default value, because then the comparison text would
# get modified as well.  So, compare using prefix only.
#
easyhg_import_path = 'NO_EASYHG_IMPORT_PATH'
if not easyhg_import_path.startswith('NO_'):
    # We have an installation path: append it twice, once with
    # the Python version suffixed
    version_suffix = 'Py%d.%d' % (sys.version_info[0], sys.version_info[1])
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

# These imports are optional, we just can't use the authfile (i.e.
# "remember this password") feature without them
#
easyhg_authfile_imports_ok = True
try:
    from Crypto.Cipher import AES
    import ConfigParser # Mercurial version won't write files
    import base64
except ImportError:
    print "EasyHg: Failed to import required modules for authfile support"
    easyhg_authfile_imports_ok = False


def encrypt_salted(text, key):
    salt = os.urandom(8)
    text = '%d.%s.%s' % (len(text), base64.b64encode(salt), text)
    text += (16 - len(text) % 16) * ' '
    cipher = AES.new(key)
    return base64.b64encode(cipher.encrypt(text))

def decrypt_salted(ctext, key):
    cipher = AES.new(key)
    text = cipher.decrypt(base64.b64decode(ctext))
    (tlen, d, text) = text.partition('.')
    (salt, d, text) = text.partition('.')
    return text[0:int(tlen)]

# from mercurial_keyring by Marcin Kasperski
def canonical_url(authuri):
    parsed_url = urlparse.urlparse(authuri)
    return "%s://%s%s" % (parsed_url.scheme, parsed_url.netloc,
                          parsed_url.path)

def load_config(pcfg, pfile):
    fp = None
    try:
        fp = open(pfile)
    except:
        return
    pcfg.readfp(fp)
    fp.close()

def save_config(ui, pcfg, pfile):
    ofp = None
    try:
        ofp = open(pfile, 'w')
    except:
        ui.write("failed to open authfile %s for writing\n" % pfile)
        raise
    try:
        #!!! Windows equivalent?
        os.fchmod(ofp.fileno(), stat.S_IRUSR | stat.S_IWUSR)
    except:
        ofp.close()
        ui.write("failed to set permissions on authfile %s\n" % pfile)
        raise
    pcfg.write(ofp)
    ofp.close()

def get_from_config(pcfg, sect, key):
    data = None
    try:
        data = pcfg.get(sect, key)
    except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
        pass
    return data

def get_boolean_from_config(pcfg, sect, key, deflt):
    data = deflt
    try:
        data = pcfg.getboolean(sect, key)
    except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
        pass
    return data

def set_to_config(pcfg, sect, key, data):
    if not pcfg.has_section(sect):
        pcfg.add_section(sect)
    pcfg.set(sect, key, data)

def remote_key(uri, user):
    # generate a "safe-for-config-file" key representing uri+user
    # tuple (n.b. trailing = on base64 is not safe)
    return base64.b64encode('%s@@%s' % (uri, user)).replace('=', '_')


def uisetup(ui):
    if not easyhg_pyqt_ok:
        raise util.Abort(_('Failed to load PyQt4 module required by easyhg.py'))
    global easyhg_qtapp
    easyhg_qtapp = QtGui.QApplication([])

def monkeypatch_method(cls):
    def decorator(func):
        setattr(cls, func.__name__, func)
        return func
    return decorator

orig_find = passwordmgr.find_user_password

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

#    self.ui.write("want username and/or password for %s\n" % authuri)

    short_uri = canonical_url(authuri)

    authkey = self.ui.config('easyhg', 'authkey')
    authfile = self.ui.config('easyhg', 'authfile')
    use_authfile = (easyhg_authfile_imports_ok and authkey and authfile)
    if authfile:
        authfile = os.path.expanduser(authfile)
    authdata = None

    dialog = QtGui.QDialog()
    layout = QtGui.QGridLayout()
    dialog.setLayout(layout)

    layout.addWidget(QtGui.QLabel(_('<h3>Login required</h3><p>Please provide your login details for the repository at<br><code>%s</code>:') % short_uri), 0, 0, 1, 2)

    user_field = QtGui.QLineEdit()
    if user:
        user_field.setText(user)
    layout.addWidget(QtGui.QLabel(_('User:')), 1, 0)
    layout.addWidget(user_field, 1, 1)

    passwd_field = QtGui.QLineEdit()
    passwd_field.setEchoMode(QtGui.QLineEdit.Password)
    if passwd:
        passwd_field.setText(passwd)
    layout.addWidget(QtGui.QLabel(_('Password:')), 2, 0)
    layout.addWidget(passwd_field, 2, 1)

    user_field.connect(user_field, Qt.SIGNAL("textChanged(QString)"),
                       passwd_field, Qt.SLOT("clear()"))

    remember_field = None
    remember = False
    authconfig = None

    if use_authfile:
        authconfig = ConfigParser.RawConfigParser()
        load_config(authconfig, authfile)
        remember = get_boolean_from_config(authconfig, 'preferences',
                                           'remember', False)
        authdata = get_from_config(authconfig, 'auth',
                                   remote_key(short_uri, user))
        if authdata:
            cachedpwd = decrypt_salted(authdata, authkey)
            passwd_field.setText(cachedpwd)
        remember_field = QtGui.QCheckBox()
        remember_field.setChecked(remember)
        remember_field.setText(_('Remember passwords while EasyMercurial is running'))
        layout.addWidget(remember_field, 3, 1)

    bb = QtGui.QDialogButtonBox()
    ok = bb.addButton(bb.Ok)
    cancel = bb.addButton(bb.Cancel)
    cancel.setDefault(False)
    cancel.setAutoDefault(False)
    ok.setDefault(True)
    bb.connect(ok, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("accept()"))
    bb.connect(cancel, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("reject()"))
    layout.addWidget(bb, 4, 0, 1, 2)
    
    dialog.setWindowTitle(_('EasyMercurial: Login'))
    dialog.show()

    if not user:
        user_field.setFocus(True)
    elif not passwd:
        passwd_field.setFocus(True)

    dialog.raise_()
    ok = dialog.exec_()
    if not ok:
        raise util.Abort(_('password entry cancelled'))

    user = user_field.text()
    passwd = passwd_field.text()

    if use_authfile:
        remember = remember_field.isChecked()
        set_to_config(authconfig, 'preferences', 'remember', remember)
        if user:
            if passwd and remember:
                authdata = encrypt_salted(passwd, authkey)
                set_to_config(authconfig, 'auth', remote_key(short_uri, user), authdata)
            else:
                set_to_config(authconfig, 'auth', remote_key(short_uri, user), '')
        save_config(self.ui, authconfig, authfile)

    self.add_password(realm, authuri, user, passwd)
    return (user, passwd)


