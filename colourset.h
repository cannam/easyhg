#ifndef _COLOURSET_H_
#define _COLOURSET_H_

#include <QSet>
#include <QMap>
#include <QColor>
#include <QString>

class ColourSet
{
public:
    void clearDefaultNames() { m_defaultNames.clear(); }
    void addDefaultName(QString n) { m_defaultNames.insert(n); }

    QColor getColourFor(QString n);

    static ColourSet *instance();

private:
    ColourSet();
    QSet<QString> m_defaultNames;
    QMap<QString, QColor> m_colours;
    QColor m_lastColour;

    static ColourSet m_instance;
};

#endif
