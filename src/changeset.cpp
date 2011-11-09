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

#include "changeset.h"
#include "common.h"
#include "debug.h"

#include <QVariant>

Changeset::Changeset(const LogEntry &e) :
    m_closed(false)
{
    foreach (QString key, e.keys()) {
        if (key == "parents") {
            QStringList parents = e.value(key).split
                (" ", QString::SkipEmptyParts);
            setParents(parents);
        } else if (key == "tag") {
            QStringList tags = e.value(key).split
                (" ", QString::SkipEmptyParts);
            setTags(tags);
        } else if (key == "bookmarks") {
            QStringList bmarks = e.value(key).split
                (" ", QString::SkipEmptyParts);
            setBookmarks(bmarks);
        } else if (key == "timestamp") {
            setTimestamp(e.value(key).split(" ")[0].toULongLong());
        } else if (key == "changeset") {
            setId(e.value(key));
        } else {
            setProperty(key.toLocal8Bit().data(), e.value(key));
        }
    }
}

QString Changeset::getLogTemplate()
{
    return "id: {rev}:{node|short}\\nauthor: {author}\\nbranch: {branches}\\ntag: {tags}\\nbookmarks: {bookmarks}\\ndatetime: {date|isodate}\\ntimestamp: {date|hgdate}\\nage: {date|age}\\nparents: {parents}\\ncomment: {desc|json}\\n\\n";
}

QString Changeset::formatHtml()
{
    QString description;
    QString rowTemplate = "<tr><td><b>%1</b>&nbsp;</td><td>%2</td></tr>";

    description = "<qt><table border=0>";

//    DEBUG << "comment is " << comment() << endl;

    QString c = comment().trimmed();
    c = c.replace(QRegExp("^\""), "");
    c = c.replace(QRegExp("\"$"), "");
    c = c.replace("\\\"", "\"");
    c = xmlEncode(c);
    c = c.replace("\\n", "<br>");

    QStringList propNames, propTexts;
    
    propNames << "id"
	      << "author"
	      << "datetime"
	      << "branch"
	      << "tags"
	      << "bookmarks"
	      << "comment";

    propTexts << QObject::tr("Identifier:")
	      << QObject::tr("Author:")
	      << QObject::tr("Date:")
	      << QObject::tr("Branch:")
	      << QObject::tr("Tag:")
	      << QObject::tr("Comment:");

    for (int i = 0; i < propNames.size(); ++i) {
	QString prop = propNames[i];
	QString value;
        if (prop == "id") {
            value = hashOf(id());
        } else if (prop == "comment") {
            value = c;
        } else if (prop == "tags") {
            value = tags().join(" ");
        } else if (prop == "bookmarks") {
            value = bookmarks().join(" ");
        } else {
	    value = xmlEncode(property(prop.toLocal8Bit().data()).toString());
	}
	if (value != "") {
	    description += rowTemplate
		.arg(xmlEncode(propTexts[i]))
		.arg(value);
	}
    }

    description += "</table></qt>";

    return description;
}
