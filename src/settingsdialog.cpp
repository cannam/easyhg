/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "settingsdialog.h"
#include "common.h"
#include "debug.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>

QString
SettingsDialog::m_installPath;

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    m_presentationChanged(false)
{
    setModal(true);
    setWindowTitle(tr("Settings"));

    QGridLayout *mainLayout = new QGridLayout;
    setLayout(mainLayout);

    m_tabs = new QTabWidget;
    mainLayout->addWidget(m_tabs, 0, 0);


//    QGroupBox *meBox = new QGroupBox(tr("User details"));
//    mainLayout->addWidget(meBox, 0, 0);

    QWidget *meBox = new QWidget;
    m_tabs->addTab(meBox, tr("User details"));

    QGridLayout *meLayout = new QGridLayout;
    meBox->setLayout(meLayout);

    int row = 0;

    meLayout->addWidget(new QLabel(tr("Name:")), row, 0);

    m_nameEdit = new QLineEdit();
    meLayout->addWidget(m_nameEdit, row++, 1);
    
    meLayout->addWidget(new QLabel(tr("Email address:")), row, 0);

    m_emailEdit = new QLineEdit();
    meLayout->addWidget(m_emailEdit, row++, 1);

    meLayout->setRowStretch(row, 20);


//    QGroupBox *lookBox = new QGroupBox(tr("Presentation"));
//    mainLayout->addWidget(lookBox, 1, 0);
    
    QWidget *lookBox = new QWidget;
    m_tabs->addTab(lookBox, tr("Presentation"));

    QGridLayout *lookLayout = new QGridLayout;
    lookBox->setLayout(lookLayout);

    row = 0;

    m_showIconLabels = new QCheckBox(tr("Show labels on toolbar icons"));
    lookLayout->addWidget(m_showIconLabels, row++, 0, 1, 2);

    m_showExtraText = new QCheckBox(tr("Show long descriptions for file status headings"));
    lookLayout->addWidget(m_showExtraText, row++, 0, 1, 2);
    
#ifdef NOT_IMPLEMENTED_YET
    lookLayout->addWidget(new QLabel(tr("Place the work and history views")), row, 0);
    m_workHistoryArrangement = new QComboBox();
    m_workHistoryArrangement->addItem(tr("In separate tabs"));
    m_workHistoryArrangement->addItem(tr("Side-by-side in a single pane"));
    lookLayout->addWidget(m_workHistoryArrangement, row++, 1, Qt::AlignLeft);
    lookLayout->setColumnStretch(1, 20);
#endif

    lookLayout->addWidget(new QLabel(tr("Label the history timeline with")), row, 0);
    m_dateFormat = new QComboBox();
    m_dateFormat->addItem(tr("Ages, for example \"5 weeks ago\""));
    m_dateFormat->addItem(tr("Dates, for example \"2010-06-23\""));
    lookLayout->addWidget(m_dateFormat, row++, 1, Qt::AlignLeft);
    lookLayout->setColumnStretch(1, 20);

    lookLayout->setRowStretch(row, 20);
    

    QWidget *pathsBox = new QWidget;
    m_tabs->addTab(pathsBox, tr("System application locations"));

//    QGroupBox *pathsBox = new QGroupBox(tr("System application locations"));
//    mainLayout->addWidget(pathsBox, 2, 0);
    QGridLayout *pathsLayout = new QGridLayout;
    pathsBox->setLayout(pathsLayout);

    row = 0;

    pathsLayout->addWidget(new QLabel(tr("Mercurial (hg) program:")), row, 0);

    m_hgPathLabel = new QLineEdit();
    pathsLayout->addWidget(m_hgPathLabel, row, 2);

    QPushButton *browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(hgPathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("External diff program:")), row, 0);

    m_diffPathLabel = new QLineEdit();
    pathsLayout->addWidget(m_diffPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(diffPathBrowse()));
    
    pathsLayout->addWidget(new QLabel(tr("External file-merge program:")), row, 0);

    m_mergePathLabel = new QLineEdit();
    pathsLayout->addWidget(m_mergePathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(mergePathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("SSH program (for ssh URLs):")), row, 0);

    m_sshPathLabel = new QLineEdit();
    pathsLayout->addWidget(m_sshPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(sshPathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("EasyHg Mercurial extension:")), row, 0);

    m_extensionPathLabel = new QLineEdit();
    pathsLayout->addWidget(m_extensionPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(extensionPathBrowse()));

    //!!! more info plz
    m_useExtension = new QCheckBox(tr("Use EasyHg Mercurial extension"));
    pathsLayout->addWidget(m_useExtension, row++, 2);

    pathsLayout->setRowStretch(row, 20);


    reset(); // loads current defaults from settings


    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox->addButton(tr("Restore defaults"), QDialogButtonBox::ResetRole),
            SIGNAL(clicked()), this, SLOT(restoreDefaults()));
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    mainLayout->addWidget(bbox, 3, 0);
    m_ok = bbox->button(QDialogButtonBox::Ok);
}

void
SettingsDialog::setCurrentTab(Tab t)
{
    switch (t) {
    case PersonalDetailsTab: m_tabs->setCurrentIndex(0); break;
    case PresentationTab: m_tabs->setCurrentIndex(1); break;
    case PathsTab: m_tabs->setCurrentIndex(2); break;
    }
}

void
SettingsDialog::hgPathBrowse()
{
    browseFor(tr("Mercurial program"), m_hgPathLabel);
}

void
SettingsDialog::diffPathBrowse()
{
    browseFor(tr("External diff program"), m_diffPathLabel);
}

void
SettingsDialog::mergePathBrowse()
{
    browseFor(tr("External file-merge program"), m_mergePathLabel);
}

void
SettingsDialog::sshPathBrowse()
{
    browseFor(tr("SSH program"), m_sshPathLabel);
}

void
SettingsDialog::extensionPathBrowse()
{
    browseFor(tr("EasyHg Mercurial extension"), m_extensionPathLabel);
}

void
SettingsDialog::browseFor(QString title, QLineEdit *edit)
{
    QString origin = edit->text();

    if (origin == "") {
#ifdef Q_OS_WIN32
        origin = "c:";
#else
        origin = QDir::homePath();
#endif
    }
    
    QString path = QFileDialog::getOpenFileName(this, title, origin);
    if (path != QString()) {
        edit->setText(path);
    }
}

void
SettingsDialog::restoreDefaults()
{
    if (QMessageBox::question
        (this, tr("Restore default settings?"),
         tr("<qt><b>Restore default settings?</b><br><br>Are you sure you want to reset all settings to their default values?"),
         QMessageBox::Ok | QMessageBox::Cancel,
         QMessageBox::Cancel) == QMessageBox::Ok) {
        clear();
        findDefaultLocations();
        reset();
    }
}

void
SettingsDialog::findDefaultLocations(QString installPath)
{
    m_installPath = installPath;
    findHgBinaryName();
    findExtension();
    findDiffBinaryName();
    findMergeBinaryName();
    findSshBinaryName();
}

void
SettingsDialog::findHgBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString hg = settings.value("hgbinary", "").toString();
    if (hg == "") {
        hg = findInPath("hg", m_installPath, true);
    }
    if (hg != "") {
        settings.setValue("hgbinary", hg);
    }
}

QString
SettingsDialog::getUnbundledExtensionFileName()
{
    QString home = QDir::homePath();
    QString target = QString("%1/.easyhg").arg(home);
    QString extpath = QString("%1/easyhg.py").arg(target);
    return extpath;
}

void
SettingsDialog::findExtension()
{
    QSettings settings;
    settings.beginGroup("Locations");

    QString extpath = settings.value("extensionpath", "").toString();
    if (extpath != "" || !QFile(extpath).exists()) {

        extpath = getUnbundledExtensionFileName();

        if (!QFile(extpath).exists()) {
            extpath = findInPath("easyhg.py", m_installPath, false);
        }
    }

    settings.setValue("extensionpath", extpath);
}   

void
SettingsDialog::findDiffBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString diff = settings.value("extdiffbinary", "").toString();
    if (diff != "" && QFile(diff).exists()) {
        return;
    }
    QStringList bases;
#ifdef Q_OS_WIN32
    bases << "easyhg-extdiff.bat";
#else
    bases << "easyhg-extdiff.sh";
#endif
    bases << "kompare" << "kdiff3" << "meld";
    bool found = false;
    foreach (QString base, bases) {
        diff = findInPath(base, m_installPath, true);
        if (diff != "") {
            found = true;
            break;
        }
    }
    if (found) {
        settings.setValue("extdiffbinary", diff);
    }
}

void
SettingsDialog::findMergeBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString merge = settings.value("mergebinary", "").toString();
    if (merge != "" && QFile(merge).exists()) {
        return;
    }
    QStringList bases;
#ifdef Q_OS_WIN32
    bases << "easyhg-merge.bat";
#else
    bases << "easyhg-merge.sh";
#endif
    // NB it's not a good idea to add other tools here, as command
    // line argument ordering varies.  Configure them through hgrc
    // instead
    bool found = false;
    foreach (QString base, bases) {
        merge = findInPath(base, m_installPath, true);
        if (merge != "") {
            found = true;
            break;
        }
    }
    if (found) {
        settings.setValue("mergebinary", merge);
    }
}

void
SettingsDialog::findSshBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString ssh = settings.value("sshbinary", "").toString();
    if (ssh != "" && QFile(ssh).exists()) {
        return;
    }
    QStringList bases;
#ifdef Q_OS_WIN32
    bases << "TortoisePlink.exe";
#else
    bases << "ssh";
#endif
    bool found = false;
    foreach (QString base, bases) {
        ssh = findInPath(base, m_installPath, true);
        if (ssh != "") {
            found = true;
            break;
        }
    }
    if (found) {
        settings.setValue("sshbinary", ssh);
    }
}

void
SettingsDialog::clear()
{
    // Clear everything that has a default setting
    DEBUG << "SettingsDialog::clear" << endl;
    QSettings settings;
    settings.beginGroup("Presentation");
    settings.remove("showiconlabels");
    settings.remove("showhelpfultext");
    settings.remove("dateformat");
    settings.endGroup();
    settings.beginGroup("Locations");
    settings.remove("hgbinary");
    settings.remove("extdiffbinary");
    settings.remove("mergebinary");
    settings.remove("sshbinary");
    settings.remove("extensionpath");
    settings.endGroup();
    settings.beginGroup("");
    settings.remove("useextension");
    settings.endGroup();
}

void
SettingsDialog::reset()
{
    DEBUG << "SettingsDialog::reset" << endl;
    QSettings settings;
    settings.beginGroup("User Information");
    m_nameEdit->setText(settings.value("name", getUserRealName()).toString());
    m_emailEdit->setText(settings.value("email").toString());
    settings.endGroup();
    settings.beginGroup("Presentation");
    m_showIconLabels->setChecked(settings.value("showiconlabels", true).toBool());
    m_showExtraText->setChecked(settings.value("showhelpfultext", true).toBool());
#ifdef NOT_IMPLEMENTED_YET
    m_workHistoryArrangement->setCurrentIndex(settings.value("workhistoryarrangement", 0).toInt());
#endif
    m_dateFormat->setCurrentIndex(settings.value("dateformat", 0).toInt());
    settings.endGroup();
    settings.beginGroup("Locations");
    m_hgPathLabel->setText(settings.value("hgbinary").toString());
    m_diffPathLabel->setText(settings.value("extdiffbinary").toString());
    m_mergePathLabel->setText(settings.value("mergebinary").toString());
    m_sshPathLabel->setText(settings.value("sshbinary").toString());
    m_extensionPathLabel->setText(settings.value("extensionpath").toString());
    settings.endGroup();
    settings.beginGroup("");
    m_useExtension->setChecked(settings.value("useextension", true).toBool());
    settings.endGroup();
}

void
SettingsDialog::accept()
{
    DEBUG << "SettingsDialog::accept" << endl;
    QSettings settings;
    settings.beginGroup("User Information");
    settings.setValue("name", m_nameEdit->text());
    settings.setValue("email", m_emailEdit->text());
    settings.endGroup();
    settings.beginGroup("Presentation");
    bool b;
    b = m_showIconLabels->isChecked();
    if (b != settings.value("showiconlabels", true)) {
        settings.setValue("showiconlabels", b);
        m_presentationChanged = true;
    }
    b = m_showExtraText->isChecked();
    if (b != settings.value("showhelpfultext", true)) {
        settings.setValue("showhelpfultext", b);
        m_presentationChanged = true;
    }
    int i;
#ifdef NOT_IMPLEMENTED_YET
    i = m_workHistoryArrangement->currentIndex();
    if (i != settings.value("workhistoryarrangement", 0)) {
        settings.setValue("workhistoryarrangement", i);
        m_presentationChanged = true;
    }
#endif
    i = m_dateFormat->currentIndex();
    if (i != settings.value("dateformat", 0)) {
        settings.setValue("dateformat", i);
        m_presentationChanged = true;
    }
    settings.endGroup();
    settings.beginGroup("Locations");
    settings.setValue("hgbinary", m_hgPathLabel->text());
    settings.setValue("extdiffbinary", m_diffPathLabel->text());
    settings.setValue("mergebinary", m_mergePathLabel->text());
    settings.setValue("sshbinary", m_sshPathLabel->text());
    settings.setValue("extensionpath", m_extensionPathLabel->text());
    settings.endGroup();
    settings.beginGroup("");
    settings.setValue("useextension", m_useExtension->isChecked());
    settings.endGroup();
    QDialog::accept();
}

    
