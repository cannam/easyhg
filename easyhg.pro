
CONFIG += debug

macx-* {
    QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
    CONFIG += x86 ppc
}

TEMPLATE = app
TARGET = EasyMercurial

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
    clickablelabel.h
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
    settingsdialog.cpp

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
