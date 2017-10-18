#include "pgpid_item_proxy.h"

//TODO: include only required headers here
#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>


//TODO: set this defines in one place
// Defines for key list columns
#define COLUMN_CHECK 0
#define COLUMN_PEERNAME    1
#define COLUMN_I_AUTH_PEER 2
#define COLUMN_PEER_AUTH_ME 3
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5
#define COLUMN_COUNT 6



pgpid_item_proxy::pgpid_item_proxy(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

void pgpid_item_proxy::use_only_trusted_keys(bool val)
{
    only_trusted_keys = val;
    filterChanged();
}

bool pgpid_item_proxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(only_trusted_keys)
    {
        if(!rsPeers)
            return false;
        RsPgpId peer_id (sourceModel()->data(sourceModel()->index(sourceRow, COLUMN_PEERID, sourceParent)).toString().toStdString());
        RsPeerDetails details;
        if(!rsPeers->getGPGDetails(peer_id, details))
            return false;
        if(details.validLvl < RS_TRUST_LVL_MARGINAL)
            return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
