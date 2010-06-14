#ifndef COMMON_H
#define COMMON_H

/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

#include <QtCore>

#define APPNAME                         "HgExplorer"
#define APPVERSION                      "0.4.7"
#define MY_ICON_SIZE                    32
#define REPOMENU_TITLE                  "Repository actions"
#define WORKFOLDERMENU_TITLE            "Workfolder actions"
#define EXITOK(x)                       ((x)==0)
#define CHGSET                          "changeset: "
#define REQUIRED_CHGSET_DIFF_COUNT      2

#define WORKTAB                         0
#define HISTORYTAB                      1
#define HEADSTAB                        2

#define HGSTAT_M_BIT    1U
#define HGSTAT_A_BIT    2U
#define HGSTAT_R_BIT    4U
#define HGSTAT_D_BIT    8U
#define HGSTAT_U_BIT    16U
#define HGSTAT_C_BIT    32U
#define HGSTAT_I_BIT    64U

#define DEFAULT_HG_STAT_BITS (HGSTAT_M_BIT | HGSTAT_A_BIT | HGSTAT_R_BIT | HGSTAT_D_BIT | HGSTAT_U_BIT)

#define NUM_PATHS_IN_MRU_LIST           5


extern QString getSystem();
extern QString getHgBinaryName();
extern QString getDiffMergeDefaultPath();
extern QString getHgDirName();

#endif 	//COMMON_H


