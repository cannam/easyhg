
CONFIG += debug

TEMPLATE = app
TARGET = hgexplorer
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
    logparser.h
SOURCES = main.cpp \
    mainwindow.cpp \
    hgexpwidget.cpp \
    hgrunner.cpp \
    grapher.cpp \
    settingsdialog.cpp \
    common.cpp \
    changeset.cpp \
    changesetitem.cpp \
    logparser.cpp

# ! [0]
RESOURCES = hgexplorer.qrc
win32 {
    RC_FILE = hgexplorer.rc
}

QT += network
