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

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QTabWidget>
#include <QDateEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum Tab {
        PersonalDetailsTab,
        PresentationTab,
        PathsTab,
        ExtensionsTab
    };

    SettingsDialog(QWidget *parent = 0);

    void setCurrentTab(Tab tab);

    bool presentationChanged() {
        return m_presentationChanged;
    }

    static void findDefaultLocations(QString installPath = m_installPath);
    static QString getUnbundledExtensionFileName();
    
private slots:
    void hgPathBrowse();
    void diffPathBrowse();
    void mergePathBrowse();
    void sshPathBrowse();
    void extensionPathBrowse();

    void accept();
    void reset();
    void clear();
    void restoreDefaults();

private:
    QTabWidget *m_tabs;

    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_hgPathLabel;
    QLineEdit *m_diffPathLabel;
    QLineEdit *m_mergePathLabel;
    QLineEdit *m_sshPathLabel;

    QPushButton *m_extensionBrowse;

    QCheckBox *m_multipleDiffInstances;
    QCheckBox *m_useExtension;
    QLineEdit *m_extensionPathLabel;

    QCheckBox *m_showIconLabels;
    QCheckBox *m_showExtraText;
    QCheckBox *m_showHistoryAutomatically;
    QComboBox *m_dateFormat;

    QDateEdit *m_dateFrom;

#ifdef NOT_IMPLEMENTED_YET
    QComboBox *m_workHistoryArrangement;
#endif

    QPushButton *m_ok;

    bool m_presentationChanged;

    void browseFor(QString, QLineEdit *);

    static void findHgBinaryName();
    static void findExtension();
    static void findDiffBinaryName();
    static void findMergeBinaryName();
    static void findSshBinaryName();

    static QString m_installPath;
};

#endif
