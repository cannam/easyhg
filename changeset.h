#ifndef CHANGESET_H
#define CHANGESET_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

class Changeset : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged STORED true);
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged STORED true);
    Q_PROPERTY(QString branch READ branch WRITE setBranch NOTIFY branchChanged STORED true);
    Q_PROPERTY(QString tag READ tag WRITE setTag NOTIFY tagChanged STORED true);
    Q_PROPERTY(QString datetime READ datetime WRITE setdatetime NOTIFY datetimeChanged STORED true);
    Q_PROPERTY(QString age READ age WRITE setAge NOTIFY ageChanged STORED true);
    Q_PROPERTY(QStringList parents READ parents WRITE setParents NOTIFY parentsChanged STORED true);
    Q_PROPERTY(QStringList children READ children WRITE setChildren NOTIFY childrenChanged STORED true);
    Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY commentChanged STORED true);

public:
    Changeset() : QObject() { }

    QString id() const { return m_id; }
    QString author() const { return m_author; }
    QString branch() const { return m_branch; }
    QString tag() const { return m_tag; }
    QString datetime() const { return m_datetime; }
    QString age() const { return m_age; }
    QStringList parents() const { return m_parents; }
    QStringList children() const { return m_children; }
    QString comment() const { return m_comment; }

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

signals:
    void idChanged(QString id);
    void authorChanged(QString author);
    void branchChanged(QString branch);
    void tagChanged(QString tag);
    void datetimeChanged(QString datetime);
    void ageChanged(QString age);
    void parentsChanged(QStringList parents);
    void childrenChanged(QStringList children);
    void commentChanged(QString comment);

public slots:
    void setId(QString id) { m_id = id; emit idChanged(id); }
    void setAuthor(QString author) { m_author = author; emit authorChanged(author); }
    void setBranch(QString branch) { m_branch = branch; emit branchChanged(branch); }
    void setTag(QString tag) { m_tag = tag; emit tagChanged(tag); }
    void setdatetime(QString datetime) { m_datetime = datetime; emit datetimeChanged(datetime); }
    void setAge(QString age) { m_age = age; emit ageChanged(age); }
    void setParents(QStringList parents) { m_parents = parents; emit parentsChanged(parents); }
    void setChildren(QStringList children) { m_children = children; emit childrenChanged(m_children); }
    void addChild(QString child) { m_children.push_back(child); emit childrenChanged(m_children); }
    void setComment(QString comment) { m_comment = comment; emit commentChanged(comment); }

private:
    QString m_id;
    QString m_author;
    QString m_branch;
    QString m_tag;
    QString m_datetime;
    QString m_age;
    QStringList m_parents;
    QStringList m_children;
    QString m_comment;
};

typedef QList<Changeset *> Changesets;

#endif // CHANGESET_H
