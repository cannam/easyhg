/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _HGIGNORE_DIALOG_H_
#define _HGIGNORE_DIALOG_H_

#include <QDialog>

class HgIgnoreDialog : public QDialog
{
    Q_OBJECT

public:
    enum IgnoreType {
	IgnoreNothing,
	IgnoreGivenFilesOnly,
	IgnoreAllFilesOfGivenNames,
	IgnoreAllFilesOfGivenSuffixes
    };

    static IgnoreType confirmIgnore(QWidget *parent,
				    QStringList files, QStringList suffixes);

private:
    HgIgnoreDialog(QWidget *parent,
		   QString title,
		   QString introText,
		   QString question,
		   QStringList options,
		   QString okButtonText);

    
};

#endif
