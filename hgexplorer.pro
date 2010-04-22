TEMPLATE = app
TARGET = hgexplorer
unix {
    DESTDIR = .
}

HEADERS = mainwindow.h \
    hgexpwidget.h \
    common.h \
    hgrunner.h \
    settingsdialog.h
SOURCES = main.cpp \
    mainwindow.cpp \
    hgexpwidget.cpp \
    hgrunner.cpp \
    settingsdialog.cpp \
    common.cpp

# ! [0]
RESOURCES = hgexplorer.qrc
win32 {
    RC_FILE = hgexplorer.rc
}
