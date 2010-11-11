/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

#include <QApplication>

#ifdef Q_WS_X11
#include <QCleanlooksStyle>
#endif

#ifdef Q_WS_WIN
#include <QWindowsXPStyle>
#endif

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(hgexplorer);

    QApplication app(argc, argv);

    app.setApplicationName(APPNAME);
/*!!!
    #ifdef Q_WS_X11
    app.setStyle(new QCleanlooksStyle);
    #endif

    #ifdef Q_WS_WIN
    app.setStyle(new QWindowsXPStyle);
    #endif
*/
    MainWindow mainWin;
    mainWin.show();
    return app.exec();
}
