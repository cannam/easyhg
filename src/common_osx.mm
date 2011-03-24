
#include "common.h"

#include <QString>

#ifdef Q_OS_MAC

#include <Foundation/Foundation.h>

QString getUserRealName()
{
    NSString *s = NSFullUserName();
    return QString::fromLocal8Bit([s UTF8String]);
}

#endif
