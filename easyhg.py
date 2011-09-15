# -*- coding: utf-8 -*-
#
#    EasyMercurial
#
#    Based on hgExplorer by Jari Korhonen
#    Copyright (c) 2010 Jari Korhonen
#    Copyright (c) 2010-2011 Chris Cannam
#    Copyright (c) 2010-2011 Queen Mary, University of London
#    
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation; either version 2 of the
#    License, or (at your option) any later version.  See the file
#    COPYING included with this distribution for more information.

import sys, os, stat, urllib, urllib2, urlparse, hashlib

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
    from PyQt4 import Qt, QtCore, QtGui
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


class EasyHgAuthStore(object):

    def __init__(self, ui, url, user, passwd):

        self.ui = ui
        self.remote_url = url

        self.user = user
        self.passwd = passwd

        self.auth_key = self.ui.config('easyhg', 'authkey')
        self.auth_file = self.ui.config('easyhg', 'authfile')

        self.use_auth_file = (easyhg_authfile_imports_ok and
                         self.auth_key and self.auth_file)

        self.auth_config = None
        self.auth_cipher = None
        self.remember = False

        if self.use_auth_file:
            self.auth_cipher = AES.new(self.auth_key, AES.MODE_CBC)
            self.auth_file = os.path.expanduser(self.auth_file)
            self.load_auth_data()

    def save(self):
        if self.use_auth_file:
            self.save_auth_data()
    
    def encrypt(self, text):
        iv = os.urandom(12)
        text = '%s.%d.%s.easyhg' % (base64.b64encode(iv), len(text), text)
        text += (16 - (len(text) % 16)) * ' '
        ctext = base64.b64encode(self.auth_cipher.encrypt(text))
        return ctext

    def decrypt(self, ctext):
        try:
            text = self.auth_cipher.decrypt(base64.b64decode(ctext))
            (iv, d, text) = text.partition('.')
            (tlen, d, text) = text.partition('.')
            return text[0:int(tlen)]
        except:
            self.ui.write("failed to decrypt/convert cached data!")
            return ''

    def argless_url(self):
        parsed = urlparse.urlparse(self.remote_url)
        return "%s://%s%s" % (parsed.scheme, parsed.netloc, parsed.path)

    def pathless_url(self):
        parsed = urlparse.urlparse(self.remote_url)
        return "%s://%s" % (parsed.scheme, parsed.netloc)

    def load_config(self):
        if not self.auth_config:
            self.auth_config = ConfigParser.RawConfigParser()
        fp = None
        try:
            fp = open(self.auth_file)
        except:
            self.ui.write("unable to read authfile %s, ignoring\n" % self.auth_file)
            return
        self.auth_config.readfp(fp)
        fp.close()

    def save_config(self):
        ofp = None
        try:
            ofp = open(self.auth_file, 'w')
        except:
            self.ui.write("failed to open authfile %s for writing\n" % self.auth_file)
            raise
        if os.name == 'posix':
            try:
                os.fchmod(ofp.fileno(), stat.S_IRUSR | stat.S_IWUSR)
            except:
                ofp.close()
                self.ui.write("failed to set permissions on authfile %s\n" % self.auth_file)
                raise
        self.auth_config.write(ofp)
        ofp.close()

    def get_from_config(self, sect, key):
        data = None
        try:
            data = self.auth_config.get(sect, key)
        except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
            pass
        return data

    def get_boolean_from_config(self, sect, key, deflt):
        data = deflt
        try:
            data = self.auth_config.getboolean(sect, key)
        except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
            pass
        return data

    def set_to_config(self, sect, key, data):
        if not self.auth_config.has_section(sect):
            self.auth_config.add_section(sect)
        self.auth_config.set(sect, key, data)

    def remote_key(self, url, user):
        # generate a "safe-for-config-file" key representing uri+user
#        self.ui.write('generating remote_key for url %s and user %s\n' % (url, user))
        s = '%s@@%s' % (url, user)
        h = hashlib.sha1()
        h.update(self.auth_key)
        h.update(s)
        hx = h.hexdigest()
        return hx
    
    def remote_user_key(self):
        return self.remote_key(self.pathless_url(), '')
    
    def remote_passwd_key(self):
        return self.remote_key(self.pathless_url(), self.user)

    def load_auth_data(self):

        self.load_config()
        if not self.auth_config: return

        self.remember = self.get_boolean_from_config(
            'preferences', 'remember', False)

        if not self.user:
            d = self.get_from_config('user', self.remote_user_key())
            if d:
                self.user = self.decrypt(d)

        if self.user:
            d = self.get_from_config('auth', self.remote_passwd_key())
            if d:
                self.passwd = self.decrypt(d)

    def save_auth_data(self):

        self.load_config()
        if not self.auth_config: return
    
        self.set_to_config('preferences', 'remember', self.remember)

#        self.ui.write('aiming to store details for user %s\n' % self.user)
    
        if self.remember and self.user:
            d = self.encrypt(self.user)
            self.set_to_config('user', self.remote_user_key(), d)
        else:
            self.set_to_config('user', self.remote_user_key(), '')
    
        if self.remember and self.user and self.passwd:
            d = self.encrypt(self.passwd)
            self.set_to_config('auth', self.remote_passwd_key(), d)
        elif self.user:
            self.set_to_config('auth', self.remote_passwd_key(), '')
            
        self.save_config()

class EasyHgAuthDialog(object):

    auth_store = None

    def __init__(self, ui, url, user, passwd):
        self.auth_store = EasyHgAuthStore(ui, url, user, passwd)

    def ask(self, repeat):

        if self.auth_store.user and self.auth_store.passwd and self.auth_store.remember:
            if not repeat:
                return (self.auth_store.user, self.auth_store.passwd)

        dialog = QtGui.QDialog()
        layout = QtGui.QGridLayout()
        dialog.setLayout(layout)

        heading = _('Login required')
        if repeat:
            heading = _('Login failed: please try again')
        label_text = _(('<h3>%s</h3><p>Please provide your login details for the repository at<br><code>%s</code>:') % (heading, self.auth_store.argless_url()))
        layout.addWidget(QtGui.QLabel(label_text), 0, 0, 1, 2)

        user_field = QtGui.QLineEdit()
        if self.auth_store.user: user_field.setText(self.auth_store.user)
        layout.addWidget(QtGui.QLabel(_('User:')), 1, 0)
        layout.addWidget(user_field, 1, 1)

        passwd_field = QtGui.QLineEdit()
        passwd_field.setEchoMode(QtGui.QLineEdit.Password)
        if self.auth_store.passwd: passwd_field.setText(self.auth_store.passwd)
        layout.addWidget(QtGui.QLabel(_('Password:')), 2, 0)
        layout.addWidget(passwd_field, 2, 1)

        user_field.connect(user_field, Qt.SIGNAL("textChanged(QString)"),
                           passwd_field, Qt.SLOT("clear()"))

        remember_field = None
        if self.auth_store.use_auth_file:
            remember_field = QtGui.QCheckBox()
            remember_field.setChecked(self.auth_store.remember)
            remember_field.setText(_('Remember these details while EasyMercurial is running'))
            layout.addWidget(remember_field, 3, 1)
            warning_field = QtGui.QLabel()
            warning_field.setText(_('<qt><i><small>Do not use this option if anyone else has access to your computer!</small></i><br></qt>'))
            warning_field.hide()
            remember_field.connect(remember_field, Qt.SIGNAL("clicked()"),
                                   warning_field, Qt.SLOT("show()"))
            layout.addWidget(warning_field, 4, 1, QtCore.Qt.AlignRight)

        bb = QtGui.QDialogButtonBox()
        ok = bb.addButton(bb.Ok)
        cancel = bb.addButton(bb.Cancel)
        cancel.setDefault(False)
        cancel.setAutoDefault(False)
        ok.setDefault(True)
        bb.connect(ok, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("accept()"))
        bb.connect(cancel, Qt.SIGNAL("clicked()"), dialog, Qt.SLOT("reject()"))
        layout.addWidget(bb, 5, 0, 1, 2)

        dialog.setWindowTitle(_('EasyMercurial: Login'))
        dialog.show()

        if not self.auth_store.user:
            user_field.setFocus(True)
        elif not self.auth_store.passwd:
            passwd_field.setFocus(True)
        else:
            ok.setFocus(True)

        dialog.raise_()
        ok = dialog.exec_()
        if not ok:
            raise util.Abort(_('password entry cancelled'))

        self.auth_store.user = user_field.text()
        self.auth_store.passwd = passwd_field.text()

        if remember_field:
            self.auth_store.remember = remember_field.isChecked()
    
        self.auth_store.save()
    
        return (self.auth_store.user, self.auth_store.passwd)


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

    if not hasattr(self, '__easyhg_last'):
        self.__easyhg_last = None

    if not self.ui.interactive():
        return orig_find(self, realm, authuri)
    if not easyhg_pyqt_ok:
        return orig_find(self, realm, authuri)

    authinfo = urllib2.HTTPPasswordMgrWithDefaultRealm.find_user_password(
        self, realm, authuri)
    user, passwd = authinfo

    repeat = False

    if (realm, authuri) == self.__easyhg_last:
        # If we are called again just after identical previous
        # request, then the previously returned auth must have been
        # wrong. So we note this to force password prompt (and avoid
        # reusing bad password indefinitely). Thanks to
        # mercurial_keyring (Marcin Kasperski) for this logic
        repeat = True

    if user and passwd and not repeat:
        return orig_find(self, realm, authuri)

    dialog = EasyHgAuthDialog(self.ui, authuri, user, passwd)

    (user, passwd) = dialog.ask(repeat)

    self.add_password(realm, authuri, user, passwd)
    self.__easyhg_last = (realm, authuri)
    return (user, passwd)


