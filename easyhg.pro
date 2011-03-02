
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

HEADERS = mainwindow.h \
    hgtabwidget.h \
    common.h \
    grapher.h \
    hgrunner.h \
    changeset.h \
    changesetitem.h \
    changesetdetailitem.h \
    logparser.h \
    panner.h \
    panned.h \
    connectionitem.h \
    textabbrev.h \
    dateitem.h \
    colourset.h \
    debug.h \
    recentfiles.h \
    startupdialog.h \
    repositorydialog.h \
    multichoicedialog.h \
    selectablelabel.h \
    filestates.h \
    filestatuswidget.h \
    confirmcommentdialog.h \
    hgaction.h \
    historywidget.h \
    changesetscene.h \
    incomingdialog.h \
    uncommitteditem.h \
    settingsdialog.h \
    clickablelabel.h \
    workstatuswidget.h \
    moreinformationdialog.h
SOURCES = main.cpp \
    mainwindow.cpp \
    hgtabwidget.cpp \
    hgrunner.cpp \
    grapher.cpp \
    common.cpp \
    changeset.cpp \
    changesetdetailitem.cpp \
    changesetitem.cpp \
    logparser.cpp \
    panner.cpp \
    panned.cpp \
    connectionitem.cpp \
    textabbrev.cpp \
    dateitem.cpp \
    colourset.cpp \
    debug.cpp \
    recentfiles.cpp \
    startupdialog.cpp \
    repositorydialog.cpp \
    multichoicedialog.cpp \
    selectablelabel.cpp \
    filestates.cpp \
    filestatuswidget.cpp \
    confirmcommentdialog.cpp \
    historywidget.cpp \
    changesetscene.cpp \
    incomingdialog.cpp \
    uncommitteditem.cpp \
    settingsdialog.cpp \
    workstatuswidget.cpp \
    moreinformationdialog.cpp

macx-* {
    SOURCES += common_osx.mm
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
