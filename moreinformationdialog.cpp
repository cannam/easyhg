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

#include "moreinformationdialog.h"

#include <QMessageBox>

void
MoreInformationDialog::critical(QWidget *parent, QString title,
				QString text, QString more)
{
    QMessageBox mb(QMessageBox::Critical,
		   title,
		   text,
		   QMessageBox::Ok,
		   parent,
		   Qt::Dialog);
    mb.setDetailedText(more);
    mb.exec();
}

void
MoreInformationDialog::information(QWidget *parent, QString title,
				   QString text, QString more)
{
    QMessageBox mb(QMessageBox::Information,
		   title,
		   text,
		   QMessageBox::Ok,
		   parent,
		   Qt::Dialog);
    mb.setDefaultButton(QMessageBox::Ok);
    mb.setDetailedText(more);
    mb.exec();
}

void
MoreInformationDialog::warning(QWidget *parent, QString title,
			       QString text, QString more)
{
    QMessageBox mb(QMessageBox::Warning,
		   title,
		   text,
		   QMessageBox::Ok,
		   parent,
		   Qt::Dialog);
    mb.setDetailedText(more);
    mb.exec();
}

