/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
    This file adapted from Rosegarden, a sequencer and musical
    notation editor.  Copyright 2000-2012 the Rosegarden development
    team.

    Adapted from KDE 4.2.0, this code originally Copyright (c) 2000
    Ronny Standtke.
*/

#ifndef _SQUEEZED_LABEL_H_
#define _SQUEEZED_LABEL_H_

#include <QLabel>

class SqueezedLabelPrivate;

/**
 * @short A replacement for QLabel that squeezes its text
 *
 * A label class that squeezes its text into the label
 *
 * If the text is too long to fit into the label it is divided into
 * remaining left and right parts which are separated by three dots.
 */

class SqueezedLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)

public:
    /**
    * Default constructor.
    */
    explicit SqueezedLabel(QWidget *parent = 0);
    explicit SqueezedLabel(const QString &text, QWidget *parent = 0);

    virtual ~SqueezedLabel();

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    /**
    * Overridden for internal reasons; the API remains unaffected.
    */
    virtual void setAlignment(Qt::Alignment);

    /**
    *  Returns the text elide mode.
    */
    Qt::TextElideMode textElideMode() const;

    /**
    * Sets the text elide mode.
    * @param mode The text elide mode.
    */
    void setTextElideMode(Qt::TextElideMode mode);

public Q_SLOTS:
    /**
    * Sets the text. Note that this is not technically a reimplementation of QLabel::setText(),
    * which is not virtual (in Qt 4.3). Therefore, you may need to cast the object to
    * SqueezedLabel in some situations:
    * \Example
    * \code
    * SqueezedLabel* squeezed = new SqueezedLabel("text", parent);
    * QLabel* label = squeezed;
    * label->setText("new text");    // this will not work
    * squeezed->setText("new text");    // works as expected
    * static_cast<SqueezedLabel*>(label)->setText("new text");    // works as expected
    * \endcode
    * @param mode The new text.
    */
    void setText(const QString &text);
    /**
    * Clears the text. Same remark as above.
    *
    */
    void clear();

protected:
    /**
    * Called when widget is resized
    */
    void resizeEvent(QResizeEvent *);
    /**
    * \reimp
    */
    void contextMenuEvent(QContextMenuEvent*);
    /**
    * does the dirty work
    */
    void squeezeTextToLabel();

private slots:
    void _k_copyFullText();

private:
    QString fullText;
    Qt::TextElideMode elideMode;
};


#endif
