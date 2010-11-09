#ifndef GRAPHER_H
#define GRAPHER_H

#include "changeset.h"
#include "changesetitem.h"

typedef QMap<Changeset *, ChangesetItem *> ChangesetItemMap;

class Grapher
{
public:
    void layout(Changesets csets, ChangesetItemMap items);
};

#endif 
