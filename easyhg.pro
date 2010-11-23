
CONFIG += debug

TEMPLATE = app
TARGET = easyhg
unix {
    DESTDIR = .
}

OBJECTS_DIR = o
MOC_DIR = o

HEADERS = mainwindow.h \
    hgexpwidget.h \
    common.h \
    grapher.h \
    hgrunner.h \
    settingsdialog.h \
    changeset.h \
    changesetitem.h \
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
    statparser.h \
    filestatuswidget.h
SOURCES = main.cpp \
    mainwindow.cpp \
    hgexpwidget.cpp \
    hgrunner.cpp \
    grapher.cpp \
    settingsdialog.cpp \
    common.cpp \
    changeset.cpp \
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
    statparser.cpp \
    filestatuswidget.cpp

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
