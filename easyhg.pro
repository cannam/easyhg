
CONFIG += release

TEMPLATE = app
TARGET = EasyMercurial

QT += widgets
QMAKE_CXXFLAGS += -DQT_DISABLE_DEPRECATED_BEFORE=0x000000

# We use the 10.5 SDK and Carbon for all 32-bit OS/X,
# and 10.6 with Cocoa for all 64-bit. (Since EasyHg 1.2,
# we can sadly no longer build for 10.4 because we need
# the FSEvents API)
macx-g++40 {
    # Note, to use the 10.4 SDK on 10.6+ you need qmake -spec macx-g++40
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
    QMAKE_CFLAGS += -mmacosx-version-min=10.5
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.5
    CONFIG += x86 ppc 
}
macx-g++ {
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
    CONFIG += x86_64
}
macx-llvm {
    CONFIG += x86_64
    QMAKE_CFLAGS += -mmacosx-version-min=10.6
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.6
}

unix {
    DESTDIR = .
}

TRANSLATIONS = easyhg_en.ts

OBJECTS_DIR = o
MOC_DIR = o

HEADERS = \
    src/mainwindow.h \
    src/hgtabwidget.h \
    src/common.h \
    src/grapher.h \
    src/hgrunner.h \
    src/changeset.h \
    src/changesetitem.h \
    src/changesetdetailitem.h \
    src/logparser.h \
    src/panner.h \
    src/panned.h \
    src/connectionitem.h \
    src/textabbrev.h \
    src/colourset.h \
    src/debug.h \
    src/recentfiles.h \
    src/startupdialog.h \
    src/repositorydialog.h \
    src/multichoicedialog.h \
    src/selectablelabel.h \
    src/filestates.h \
    src/filestatuswidget.h \
    src/confirmcommentdialog.h \
    src/hgaction.h \
    src/historywidget.h \
    src/changesetscene.h \
    src/changesetview.h \
    src/incomingdialog.h \
    src/uncommitteditem.h \
    src/settingsdialog.h \
    src/clickablelabel.h \
    src/workstatuswidget.h \
    src/moreinformationdialog.h \
    src/annotatedialog.h \
    src/hgignoredialog.h \
    src/versiontester.h \
    src/squeezedlabel.h \
    src/fswatcher.h \
    src/findwidget.h
SOURCES = \
    src/main.cpp \
    src/mainwindow.cpp \
    src/hgtabwidget.cpp \
    src/hgrunner.cpp \
    src/grapher.cpp \
    src/common.cpp \
    src/changeset.cpp \
    src/changesetdetailitem.cpp \
    src/changesetitem.cpp \
    src/logparser.cpp \
    src/panner.cpp \
    src/panned.cpp \
    src/connectionitem.cpp \
    src/textabbrev.cpp \
    src/colourset.cpp \
    src/debug.cpp \
    src/recentfiles.cpp \
    src/startupdialog.cpp \
    src/repositorydialog.cpp \
    src/multichoicedialog.cpp \
    src/selectablelabel.cpp \
    src/filestates.cpp \
    src/filestatuswidget.cpp \
    src/confirmcommentdialog.cpp \
    src/historywidget.cpp \
    src/changesetscene.cpp \
    src/changesetview.cpp \
    src/incomingdialog.cpp \
    src/uncommitteditem.cpp \
    src/settingsdialog.cpp \
    src/workstatuswidget.cpp \
    src/moreinformationdialog.cpp \
    src/annotatedialog.cpp \
    src/hgignoredialog.cpp \
    src/versiontester.cpp \
    src/squeezedlabel.cpp \
    src/fswatcher.cpp \
    src/findwidget.cpp


macx-* {
    OBJECTIVE_SOURCES += src/common_osx.mm
    LIBS += -framework CoreServices -framework Foundation
    ICON = easyhg-icon.icns
}

linux* {
    LIBS += -lutil
    binaries.path = /usr/local/bin
    binaries.files = EasyMercurial easyhg-extdiff.sh easyhg-merge.sh 
    scripts.path = /usr/local/bin
    scripts.files = easyhg-extdiff.sh easyhg-merge.sh
    desktop.path = /usr/local/share/applications
    desktop.files = deploy/linux/EasyMercurial.desktop
    icon128.path = /usr/local/share/icons/hicolor/128x128/apps
    icon128.files = images/icon/128/easyhg-icon.png
    icon64.path = /usr/local/share/icons/hicolor/64x64/apps
    icon64.files = images/icon/64/easyhg-icon.png
    icon48.path = /usr/local/share/icons/hicolor/48x48/apps
    icon48.files = images/icon/48/easyhg-icon.png
    icon32.path = /usr/local/share/icons/hicolor/32x32/apps
    icon32.files = images/icon/32/easyhg-icon.png
    icon24.path = /usr/local/share/icons/hicolor/24x24/apps
    icon24.files = images/icon/24/easyhg-icon.png
    iconsc.path = /usr/local/share/icons/hicolor/scalable/apps
    iconsc.files = images/icon/scalable/easyhg-icon.svg
    INSTALLS += binaries desktop icon128 icon64 icon48 icon32 icon24 iconsc
}

win* {
    LIBS += -lSecur32 -lAdvapi32
}

RESOURCES = easyhg.qrc
win32 {
    RC_FILE = easyhg.rc
}

QT += network
