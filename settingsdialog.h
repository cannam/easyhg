/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2010 Chris Cannam
    Copyright (c) 2010 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = 0);
    
private slots:
    void hgPathBrowse();
    void diffPathBrowse();
    void mergePathBrowse();
    void editPathBrowse();
    void extensionPathBrowse();

    void accept();

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_hgPathLabel;
    QLineEdit *m_diffPathLabel;
    QLineEdit *m_mergePathLabel;
    QLineEdit *m_editPathLabel;

    QCheckBox *m_useExtension;
    QLineEdit *m_extensionPathLabel;

    QPushButton *m_ok;

    void browseFor(QString, QLineEdit *);
};

#endif
