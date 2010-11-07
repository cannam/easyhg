#include "logparser.h"

#include <QStringList>
#include <QRegExp>

LogParser::LogParser(QString text) : m_text(text)
{
    m_text.replace("\r\n", "\n");
}

QStringList LogParser::split()
{
    return m_text.split("\n\n", QString::SkipEmptyParts);
}

LogList LogParser::parse()
{
    LogList results;
    QRegExp re("^(\\w+):\\s+(.*)$");
    QStringList entries = split();
    foreach (QString entry, entries) {
        LogEntry dictionary;
        QStringList lines = entry.split('\n');
        foreach (QString line, lines) {
            if (re.indexIn(line) == 0) {
                QString key = re.cap(1);
                QString value = re.cap(2);
                dictionary[key] = value;
            }
        }
        results.push_back(dictionary);
    }
    return results;
}
