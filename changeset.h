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
    Q_PROPERTY(QString date READ date WRITE setDate NOTIFY dateChanged STORED true);
    Q_PROPERTY(QString age READ age WRITE setAge NOTIFY ageChanged STORED true);
    Q_PROPERTY(QStringList parents READ parents WRITE setParents NOTIFY parentsChanged STORED true);
    Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY commentChanged STORED true);

public:
    Changeset() : QObject() { }

    QString id() const { return m_id; }
    QString author() const { return m_author; }
    QString branch() const { return m_branch; }
    QString tag() const { return m_tag; }
    QString date() const { return m_date; }
    QString age() const { return m_age; }
    QStringList parents() const { return m_parents; }
    QString comment() const { return m_comment; }

    int number() const {
        return id().split(':')[0].toInt();
    }

    QString authorName() const {
	QString a = author();
	return a.replace(QRegExp("\\s*<[^>]*>"), "");
    }

signals:
    void idChanged(QString id);
    void authorChanged(QString author);
    void branchChanged(QString branch);
    void tagChanged(QString tag);
    void dateChanged(QString date);
    void ageChanged(QString age);
    void parentsChanged(QStringList parents);
    void commentChanged(QString comment);

public slots:
    void setId(QString id) { m_id = id; emit idChanged(id); }
    void setAuthor(QString author) { m_author = author; emit authorChanged(author); }
    void setBranch(QString branch) { m_branch = branch; emit branchChanged(branch); }
    void setTag(QString tag) { m_tag = tag; emit tagChanged(tag); }
    void setDate(QString date) { m_date = date; emit dateChanged(date); }
    void setAge(QString age) { m_age = age; emit ageChanged(age); }
    void setParents(QStringList parents) { m_parents = parents; emit parentsChanged(parents); }
    void setComment(QString comment) { m_comment = comment; emit commentChanged(comment); }

private:
    QString m_id;
    QString m_author;
    QString m_branch;
    QString m_tag;
    QString m_date;
    QString m_age;
    QStringList m_parents;
    QString m_comment;
};

typedef QList<Changeset *> Changesets;

#endif // CHANGESET_H
