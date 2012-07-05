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
class RsGxsChange
{
public:
    RsGxsChange(){ return; }

};

/*!
 * Relevant to group changes
 * TODO: extent to indicate whether a meta change or actual data
 */
class RsGxsGroupChange : RsGxsChange
{
public:
    std::list<std::string> grpIdList;
};

/*!
 * Relevant to message changes
 * TODO: extent to indicate whether a meta change or actual data
 */
class RsGxsMsgChange : RsGxsChange
{
public:
    std::map<std::string, std::vector<std::string> > msgChangeMap;
};



#endif // RSGXSSERVICE_H
