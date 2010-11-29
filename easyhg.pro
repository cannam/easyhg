
CONFIG += debug

TEMPLATE = app
TARGET = easyhg
unix {
    DESTDIR = .
}

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
    incomingdialog.h
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
    incomingdialog.cpp

macx-* {
    SOURCES += common_osx.mm
    LIBS += -framework Foundation
}

linux* {
    LIBS += -lutil
}

win* {
    LIBS += -lSecur32
}

# ! [0]
RESOURCES = easyhg.qrc
win32 {
    RC_FILE = easyhg.rc
}

QT += network
