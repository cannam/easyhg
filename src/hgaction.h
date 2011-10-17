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

#ifndef HGACTION_H
#define HGACTION_H

#include <QString>
#include <QStringList>

enum HGACTIONS
{
    ACT_NONE,
    ACT_TEST_HG,
    ACT_TEST_HG_EXT,
    ACT_QUERY_PATHS,
    ACT_QUERY_BRANCH,
    ACT_STAT,
    ACT_RESOLVE_LIST,
    ACT_QUERY_HEADS,
    ACT_QUERY_HEADS_ACTIVE,
    ACT_QUERY_PARENTS,
    ACT_LOG,
    ACT_LOG_INCREMENTAL,
    ACT_REMOVE,
    ACT_ADD,
    ACT_INCOMING,
    ACT_PUSH,
    ACT_PULL,
    ACT_CLONEFROMREMOTE,
    ACT_INIT,
    ACT_COMMIT,
    ACT_ANNOTATE,
    ACT_UNCOMMITTED_SUMMARY,
    ACT_DIFF_SUMMARY,
    ACT_FOLDERDIFF,
    ACT_CHGSETDIFF,
    ACT_UPDATE,
    ACT_REVERT,
    ACT_MERGE,
    ACT_SERVE,
    ACT_RESOLVE_MARK,
    ACT_RETRY_MERGE,
    ACT_TAG,
    ACT_NEW_BRANCH,
    ACT_HG_IGNORE,
    ACT_COPY_FILE,
    ACT_RENAME_FILE
};

struct HgAction
{
    HGACTIONS action;
    QString workingDir;
    QStringList params;
    QString executable; // empty for normal Hg, but gets filled in by hgrunner
    void *extraData;

    HgAction() : action(ACT_NONE) { }

    HgAction(HGACTIONS _action, QString _wd, QStringList _params) :
        action(_action), workingDir(_wd), params(_params), extraData(0) { }

    HgAction(HGACTIONS _action, QString _wd, QStringList _params, void *_d) :
        action(_action), workingDir(_wd), params(_params), extraData(_d) { }

    bool operator==(const HgAction &a) {
        return (a.action == action && a.workingDir == workingDir &&
                a.params == params && a.executable == executable &&
                a.extraData == extraData);
    }

    bool shouldBeFast() const {
        switch (action) {
        case ACT_NONE:
        case ACT_TEST_HG:
        case ACT_TEST_HG_EXT:
        case ACT_QUERY_PATHS:
        case ACT_QUERY_BRANCH:
        case ACT_STAT:
        case ACT_RESOLVE_LIST:
        case ACT_QUERY_HEADS:
        case ACT_QUERY_HEADS_ACTIVE:
        case ACT_QUERY_PARENTS:
        case ACT_LOG_INCREMENTAL:
            return true;
        default:
            return false;
        }
    }
    
    bool mayBeInteractive() const {
	switch (action) {
        case ACT_TEST_HG_EXT: // so we force the module load to be tested
	case ACT_INCOMING:
	case ACT_PUSH:
	case ACT_PULL:
	case ACT_CLONEFROMREMOTE:
	case ACT_FOLDERDIFF:
	case ACT_CHGSETDIFF:
	case ACT_SERVE:
	    return true;
	default:
	    return false;
	}
    }
};

#endif
