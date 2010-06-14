/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/


#include "common.h"

QString getSystem()
{
    #ifdef Q_WS_X11
    return QString("Linux");
    #endif

    #ifdef Q_WS_MAC
    return QString("Mac");
    #endif

    #ifdef Q_WS_WIN
    return QString("Windows");
    #endif

    return QString("");
}

QString getHgBinaryName()
{
    if (getSystem() == "Windows")
        return QString("hg.exe");
    else
        return QString("hg");
}

QString getDiffMergeDefaultPath()
{
    if (getSystem() == "Windows")
        return QString("c:\\program files\\sourcegear\\diffmerge\\diffmerge.exe");
    else
        return QString("/usr/bin/diffmerge");
}


QString getHgDirName()
{
    if (getSystem() == "Windows")
    {
        return QString(".hg\\");
    }
    else
    {
        return QString(".hg/");
    }
}






