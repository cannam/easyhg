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

#ifndef COMMON_H
#define COMMON_H

#include <QString>

#define MY_ICON_SIZE                    32
#define REPOMENU_TITLE                  "Repository actions"
#define WORKFOLDERMENU_TITLE            "Workfolder actions"
#define EXITOK(x)                       ((x)==0)
#define CHGSET                          "changeset: "
#define REQUIRED_CHGSET_DIFF_COUNT      2

#define WORKTAB                         0
#define HISTORYTAB                      2
#define HEADSTAB                        3

#define HGSTAT_M_BIT    1U
#define HGSTAT_A_BIT    2U
#define HGSTAT_R_BIT    4U
#define HGSTAT_D_BIT    8U
#define HGSTAT_U_BIT    16U
#define HGSTAT_C_BIT    32U
#define HGSTAT_I_BIT    64U

#define DEFAULT_HG_STAT_BITS (HGSTAT_M_BIT | HGSTAT_A_BIT | HGSTAT_R_BIT | HGSTAT_D_BIT | HGSTAT_U_BIT)

extern QString findExecutable(QString name);

extern QString getSystem();
extern QString getHgDirName();

extern QString getUserRealName();

extern void loseControllingTerminal();

#endif 	//COMMON_H


