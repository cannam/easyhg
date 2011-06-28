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

import sys, os, stat

import urllib, urllib2, urlparse

from mercurial import ui, util, error
try:
    from mercurial.url import passwordmgr
except:
    from mercurial.httprepo import passwordmgr

from mercurial.i18n import _

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

easyhg_authfile_imports_ok = True
try:
    from Crypto.Cipher import AES
    import ConfigParser # Mercurial version won't write files
    import base64
except ImportError:
    easyhg_authfile_imports_ok = False

#!!! should be in a class here

def encrypt(text, key):
    text = '%d.%s' % (len(text), text)
    text += (16 - len(text) % 16) * ' '
    cipher = AES.new(key)
    return base64.b64encode(cipher.encrypt(text))

def decrypt(ctext, key):
    cipher = AES.new(key)
    text = cipher.decrypt(base64.b64decode(ctext))
    (tlen, d, text) = text.partition('.')
    return text[0:int(tlen)]

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

def load_config(pcfg, pfile):
    fp = None
    try:
        fp = open(pfile)
    except:
        pass
    if fp:
        pcfg.readfp(fp)
        fp.close()

def save_config(pcfg, pfile):
    ofp = None
    try:
        ofp = open(pfile, 'w')
    except:
        self.ui.write("failed to open authfile %s for writing\n" % pfile)
        raise
    try:
        os.fchmod(ofp.fileno(), stat.S_IRUSR | stat.S_IWUSR) #!!! Windows equivalent?
    except:
        ofp.close()
        ofp = None
        self.ui.write("failed to set proper permissions on authfile %s\n" % pfile)
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

    pkey = None
    pekey = self.ui.config('easyhg', 'authkey')
    pfile = self.ui.config('easyhg', 'authfile')
    use_authfile = (easyhg_authfile_imports_ok and pekey and pfile)
    if use_authfile:
        pkey = base64.b64encode('%s@@%s' % (uri, user)).replace('=', '_')
    if pfile:
        pfile = os.path.expanduser(pfile)
    pdata = None

    self.ui.write("pekey is %s\n" % pekey)
    self.ui.write("pfile is %s\n" % pfile)

    dialog = QtGui.QDialog()
    layout = QtGui.QGridLayout()
    dialog.setLayout(layout)

    layout.addWidget(QtGui.QLabel(_('<h3>Login required</h3><p>Please provide your login details for the repository at<br><code>%s</code>:') % uri), 0, 0, 1, 2)

    userfield = QtGui.QLineEdit()
    if user:
        userfield.setText(user)
    layout.addWidget(QtGui.QLabel(_('User:')), 1, 0)
    layout.addWidget(userfield, 1, 1)

    passfield = QtGui.QLineEdit()
    passfield.setEchoMode(QtGui.QLineEdit.Password)
    if passwd:
        passfield.setText(passwd)
    layout.addWidget(QtGui.QLabel(_('Password:')), 2, 0)
    layout.addWidget(passfield, 2, 1)

    userfield.connect(userfield, Qt.SIGNAL("textChanged(QString)"), passfield, Qt.SLOT("clear()"))

    remember = None
    pcfg = None

    if use_authfile:
        # load pwd from our cache file, decrypt with given key
        pcfg = ConfigParser.RawConfigParser()
        load_config(pcfg, pfile)
        remember_default = get_boolean_from_config(pcfg, 'preferences', 'remember', False)
        pdata = get_from_config(pcfg, 'auth', pkey)
        if pdata:
            cachedpwd = decrypt(pdata, pekey)
            passfield.setText(cachedpwd)
        remember = QtGui.QCheckBox()
        remember.setChecked(remember_default)
        remember.setText(_('Remember this password until EasyMercurial exits'))
        layout.addWidget(remember, 3, 1)

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
        userfield.setFocus(True)
    elif not passwd:
        passfield.setFocus(True)

    dialog.raise_()
    ok = dialog.exec_()
    if not ok:
        raise util.Abort(_('password entry cancelled'))

    self.ui.write('Dialog accepted\n')
    user = userfield.text()
    passwd = passfield.text()

    if use_authfile:
        set_to_config(pcfg, 'preferences', 'remember', remember.isChecked())
        if remember.isChecked():
            pdata = encrypt(passwd, pekey)
            set_to_config(pcfg, 'auth', pkey, pdata)
        else:
            set_to_config(pcfg, 'auth', pkey, '')
        save_config(pcfg, pfile)

    self.add_password(realm, authuri, user, passwd)
    return (user, passwd)


