
CONFIG += release

TEMPLATE = app
TARGET = EasyMercurial

# We use the 10.4 SDK and Carbon for all 32-bit OS/X,
# and 10.6 with Cocoa for all 64-bit
macx-g++40 {
    # Note, to use the 10.4 SDK on 10.6+ you need qmake -spec macx-g++40
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
    QMAKE_CFLAGS += -mmacosx-version-min=10.4
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.4
    CONFIG += x86 ppc 
}
macx-g++ {
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
    CONFIG += x86_64
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
    src/dateitem.h \
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
    src/incomingdialog.h \
    src/uncommitteditem.h \
    src/settingsdialog.h \
    src/clickablelabel.h \
    src/workstatuswidget.h \
    src/moreinformationdialog.h \
    src/annotatedialog.h
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
    src/dateitem.cpp \
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
    src/incomingdialog.cpp \
    src/uncommitteditem.cpp \
    src/settingsdialog.cpp \
    src/workstatuswidget.cpp \
    src/moreinformationdialog.cpp \
    src/annotatedialog.cpp

macx-* {
    SOURCES += src/common_osx.mm
    LIBS += -framework Foundation
    ICON = easyhg.icns
}

linux* {
    LIBS += -lutil
}

win* {
    LIBS += -lSecur32
}

RESOURCES = easyhg.qrc
win32 {
    RC_FILE = easyhg.rc
}

QT += network
