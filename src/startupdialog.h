/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

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

#ifndef STARTUP_DIALOG_H
#define STARTUP_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    StartupDialog(QWidget *parent = 0);
    
private slots:
    void realNameChanged(const QString &);
    void emailChanged(const QString &);
    void accept();

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QLabel *m_example;
    QPushButton *m_ok;

    QString m_name;
    QString m_email;

    void updateExample();
};

#endif
