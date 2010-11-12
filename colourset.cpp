
#include "colourset.h"

ColourSet
ColourSet::m_instance;

ColourSet::ColourSet() { }

ColourSet *
ColourSet::instance()
{
    return &m_instance;
}

QColor
ColourSet::getColourFor(QString n)
{
    if (m_defaultNames.contains(n)) return Qt::black;
    if (m_colours.contains(n)) return m_colours[n];

    QColor c;

    if (m_colours.empty()) {
	c = QColor::fromHsv(0, 200, 100);
    } else {
	c = QColor::fromHsv((m_lastColour.hue() + 70) % 360, 200, 100);
    }

    m_colours[n] = c;
    m_lastColour = c;
    return c;
}


    
