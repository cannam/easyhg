#ifndef LOGPARSER_H
#define LOGPARSER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

typedef QMap<QString, QString> LogEntry;
typedef QList<LogEntry> LogList;

class LogParser : public QObject
{
public:
    LogParser(QString text);

    QStringList split();
    LogList parse();

private:
    QString m_text;
};

#endif // LOGPARSER_H
