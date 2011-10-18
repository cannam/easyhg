/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef CHANGESET_H
#define CHANGESET_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QSharedPointer>

#include "logparser.h"

class Changeset;

typedef QList<Changeset *> Changesets;

class Changeset : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged STORED true);
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged STORED true);
    Q_PROPERTY(QString branch READ branch WRITE setBranch NOTIFY branchChanged STORED true);
    Q_PROPERTY(QStringList tags READ tags WRITE setTags NOTIFY tagsChanged STORED true);
    Q_PROPERTY(QString datetime READ datetime WRITE setDatetime NOTIFY datetimeChanged STORED true);
    Q_PROPERTY(qulonglong timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged STORED true);
    Q_PROPERTY(QString age READ age WRITE setAge NOTIFY ageChanged STORED true);
    Q_PROPERTY(QStringList parents READ parents WRITE setParents NOTIFY parentsChanged STORED true);
    Q_PROPERTY(QStringList children READ children WRITE setChildren NOTIFY childrenChanged STORED true);
    Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY commentChanged STORED true);

public:
    Changeset() : QObject() { }
    explicit Changeset(const LogEntry &e);

    QString id() const { return m_id; }
    QString author() const { return m_author; }
    QString branch() const { return m_branch; }
    QStringList tags() const { return m_tags; }
    QString datetime() const { return m_datetime; }
    qulonglong timestamp() const { return m_timestamp; }
    QString age() const { return m_age; }
    QStringList parents() const { return m_parents; }
    QString comment() const { return m_comment; }

    /**
     * The children property is not obtained from Hg, but set in
     * Grapher::layout() based on reported parents
     */
    QStringList children() const { return m_children; }

    /**
     * The closed property is not obtained from Hg, but set in
     * Grapher::layout() based on traversing closed branch graphs
     */
    bool closed() const { return m_closed; }

    int number() const {
        return id().split(':')[0].toInt();
    }

    QString authorName() const {
	QString a = author();
	return a.replace(QRegExp("\\s*<[^>]*>"), "");
    }

    QString date() const {
        return datetime().split(' ')[0];
    }

    bool isOnBranch(QString branch) {
        QString b = m_branch;
        if (branch == "") branch = "default";
        if (b == "") b = "default";
        if (branch == b) return true;
        return false;
    }

    static QString hashOf(QString id) {
        return id.split(':')[1];
    }

    static QStringList getIds(Changesets csets) {
        QStringList ids;
        foreach (Changeset *cs, csets) ids.push_back(cs->id());
        return ids;
    }

    static Changesets parseChangesets(QString logText) {
        Changesets csets;
        LogList log = LogParser(logText).parse();
        foreach (LogEntry e, log) {
            csets.push_back(new Changeset(e));
        }
        return csets;
    }

    static QString getLogTemplate();

    QString formatHtml();
    
signals:
    void idChanged(QString id);
    void authorChanged(QString author);
    void branchChanged(QString branch);
    void tagsChanged(QStringList tags);
    void datetimeChanged(QString datetime);
    void timestampChanged(qulonglong timestamp);
    void ageChanged(QString age);
    void parentsChanged(QStringList parents);
    void childrenChanged(QStringList children);
    void closedChanged(bool closed);
    void commentChanged(QString comment);

public slots:
    void setId(QString id) { m_id = id; emit idChanged(id); }
    void setAuthor(QString author) { m_author = author; emit authorChanged(author); }
    void setBranch(QString branch) { m_branch = branch; emit branchChanged(branch); }
    void setTags(QStringList tags) { m_tags = tags; emit tagsChanged(tags); }
    void addTag(QString tag) { m_tags.push_back(tag); emit tagsChanged(m_tags); }
    void setDatetime(QString datetime) { m_datetime = datetime; emit datetimeChanged(datetime); }
    void setTimestamp(qulonglong timestamp) { m_timestamp = timestamp; emit timestampChanged(timestamp); }
    void setAge(QString age) { m_age = age; emit ageChanged(age); }
    void setParents(QStringList parents) { m_parents = parents; emit parentsChanged(parents); }
    void setChildren(QStringList children) { m_children = children; emit childrenChanged(m_children); }
    void addChild(QString child) { m_children.push_back(child); emit childrenChanged(m_children); }
    void setClosed(bool closed) { m_closed = closed; emit closedChanged(closed); }
    void setComment(QString comment) { m_comment = comment; emit commentChanged(comment); }

private:
    QString m_id;
    QString m_author;
    QString m_branch;
    QStringList m_tags;
    QString m_datetime;
    qulonglong m_timestamp;
    QString m_age;
    QStringList m_parents;
    QStringList m_children;
    bool m_closed;
    QString m_comment;
};


#endif // CHANGESET_H
