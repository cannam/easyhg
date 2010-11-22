#include "statparser.h"

#include "debug.h"

#include <QMap>

StatParser::StatParser(QString text)
{
    text.replace("\r\n", "\n");

    QMap<QChar, QStringList *> buckets;
    buckets['M'] = &modified;
    buckets['A'] = &added;
    buckets['R'] = &removed;
    buckets['!'] = &missing;
    buckets['?'] = &unknown;

    QStringList lines = text.split("\n", QString::SkipEmptyParts);
    foreach (QString line, lines) {
        if (line.length() < 3 || line[1] != ' ') {
            continue;
        }
        QChar tag = line[0];
        QString file = line.right(line.length() - 2);
        if (buckets.contains(tag)) {
            buckets[tag]->push_back(file);
        }
    }

    DEBUG << "StatParser: " << modified.size() << " modified, " << added.size()
            << " added, " << removed.size() << " removed, " << missing.size()
            << " missing, " << unknown.size() << " unknown" << endl;
}
