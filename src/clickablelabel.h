/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CLICKABLE_LABEL_H_
#define _CLICKABLE_LABEL_H_

#include "squeezedlabel.h"

#include <QMouseEvent>

class ClickableLabel : public SqueezedLabel
{
    Q_OBJECT

    Q_PROPERTY(bool mouseUnderline READ mouseUnderline WRITE setMouseUnderline)

public:
    ClickableLabel(const QString &text, QWidget *parent = 0) :
        SqueezedLabel(text, parent),
	m_naturalText(text)
    { }

    ClickableLabel(QWidget *parent = 0) :
	SqueezedLabel(parent)
    { }

    ~ClickableLabel()
    { }

    void setText(const QString &t) {
	m_naturalText = t;
	SqueezedLabel::setText(t);
    }

    bool mouseUnderline() const {
	return m_mouseUnderline;
    }

    void setMouseUnderline(bool mu) {
	m_mouseUnderline = mu;
	if (mu) {
	    setTextFormat(Qt::RichText);
	    setCursor(Qt::PointingHandCursor);
	}
    }

signals:
    void clicked();

protected:
    virtual void enterEvent(QEvent *) {
	if (m_mouseUnderline) {
	    SqueezedLabel::setText(tr("<u>%1</u>").arg(m_naturalText));
	}
    }

    virtual void leaveEvent(QEvent *) {
	if (m_mouseUnderline) {
	    SqueezedLabel::setText(m_naturalText);
	}
    }

    virtual void mousePressEvent(QMouseEvent *ev) {
        if (ev->button() == Qt::LeftButton) {
            emit clicked();
        } else {
            SqueezedLabel::mousePressEvent(ev);
        }
    }

private:
    bool m_mouseUnderline;
    QString m_naturalText;
};

#endif
