
CONFIG += debug

TEMPLATE = app
TARGET = easyhg
unix {
    DESTDIR = .
}

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
    debug.h
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
    debug.cpp

macx-* {
SOURCES += common_osx.mm
}

# ! [0]
RESOURCES = hgexplorer.qrc
win32 {
    RC_FILE = hgexplorer.rc
}

QT += network opengl
