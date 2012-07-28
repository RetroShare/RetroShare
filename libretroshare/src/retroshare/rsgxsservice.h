#ifndef RSGXSSERVICE_H
#define RSGXSSERVICE_H


#include "gxs/rstokenservice.h"


/*!
 * The aim of this class is to abstract
 * how changes are represented so
 * they can be determined outside the
 * client API without explcitly
 * enumerating all possible changes
 * at the interface
 */
class RsGxsNotify
{
public:
    RsGxsNotify(){ return; }

};

/*!
 * Relevant to group changes
 * TODO: extent to indicate whether a meta change or actual data
 */
class RsGxsGroupChange : public RsGxsNotify
{
public:
    std::list<RsGxsGroupId> grpIdList;
};

/*!
 * Relevant to message changes
 * TODO: extent to indicate whether a meta change or actual data
 */
class RsGxsMsgChange : public RsGxsNotify
{
public:
    std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgChangeMap;
};



#endif // RSGXSSERVICE_H
