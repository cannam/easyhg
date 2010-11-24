#include "filestates.h"

#include "debug.h"

#include <QMap>

FileStates::FileStates(QString text)
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

    DEBUG << "FileStates: " << modified.size() << " modified, " << added.size()
            << " added, " << removed.size() << " removed, " << missing.size()
            << " missing, " << unknown.size() << " unknown" << endl;
}

QStringList FileStates::getFilesInState(State s)
{
    switch (s) {

    case Modified: return modified;
    case Added: return added;
    case Unknown: return unknown;
    case Removed: return removed;
    case Missing: return missing;

    case UpToDate: // not supported yet
    default:
        return QStringList();
    }
}

FileStates::State FileStates::getStateOfFile(QString file)
{
    // slow, but let's worry about that if it becomes a problem
    for (int is = int(FirstState); is <= int(LastState); ++is) {
        QStringList fl = getFilesInState(State(is));
        foreach (QString f, fl) if (f == file) return State(is);
    }
    DEBUG << "FileStates: WARNING: getStateOfFile: file "
            << file << " is unknown to us: returning Unknown state, "
            << "but unknown to us is not supposed to be the same "
            << "thing as unknown state..."
            << endl;
    return Unknown;
}
