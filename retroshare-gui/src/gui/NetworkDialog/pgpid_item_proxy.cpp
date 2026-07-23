/*******************************************************************************
 * retroshare-gui/src/gui/NetworkDialog/pgpid_item_proxy.cpp                   *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "pgpid_item_proxy.h"

#include "gui/common/StatusDefs.h"

#include <QStringList>

//TODO: include only required headers here
#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

bool pgpid_item_proxy::lessThan(const QModelIndex &left, const QModelIndex &right) const 
{
    if(left.column() == pgpid_item_model::PGP_ITEM_MODEL_COLUMN_LAST_USED)
		return left.data(Qt::EditRole).toUInt() < right.data(Qt::EditRole).toUInt();
	else
		return left.data(Qt::DisplayRole).toString().toUpper() < right.data(Qt::DisplayRole).toString().toUpper();
}


pgpid_item_proxy::pgpid_item_proxy(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

void pgpid_item_proxy::use_only_trusted_keys(bool val)
{
    only_trusted_keys = val;
    invalidateFilter();
}

void pgpid_item_proxy::setFilterText(const QString &text)
{
    mFilterText = text;
    invalidateFilter();
}

RsPgpId pgpid_item_proxy::pgpIdOfRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return RsPgpId(sourceModel()->data(sourceModel()->index(sourceRow, pgpid_item_model::PGP_ITEM_MODEL_COLUMN_PEERID, sourceParent)).toString().toStdString());
}

// Lists every searchable field of a profile: its name and PGP id, then for each of its locations the node
// name and id, the known addresses, the live connection type, and for hidden nodes the Tor/I2P type and
// address. Several of these fields are shown in no column at all, hence the labels: see data().

QVector<QPair<QString,QString> > pgpid_item_proxy::searchFields(const RsPgpId &pgp_id) const
{
    QVector<QPair<QString,QString> > fields;

    if(!rsPeers)
        return fields;

    RsPeerDetails detail;

    if(!rsPeers->getGPGDetails(pgp_id, detail))
        return fields;

    fields.push_back(qMakePair(tr("Name")  ,QString::fromUtf8(detail.name.c_str())));
    fields.push_back(qMakePair(tr("PGP ID"),QString::fromStdString(detail.gpg_id.toStdString())));

    // Node ids and addresses only exist for profiles we are actually friends with: getAssociatedSSLIds()
    // only walks the friend list. For the other keys of the ring, the search is name and PGP id only.

    std::list<RsPeerId> ssl_ids;

    if(!rsPeers->getAssociatedSSLIds(pgp_id, ssl_ids))
        return fields;

    for(auto ssl_id(ssl_ids.begin()); ssl_id != ssl_ids.end(); ++ssl_id)
    {
        RsPeerDetails node;

        if(!rsPeers->getPeerDetails(*ssl_id, node))
            continue;

        QString location = QString::fromUtf8(node.location.c_str());

        if(location.isEmpty())
            location = QString::fromStdString(node.id.toStdString());

        auto add = [&fields,&location](const QString& label,const QString& value)
        {
            if(!value.isEmpty())
                fields.push_back(qMakePair(QString("%1 [%2]").arg(label,location),value));
        };

        add(tr("Node name")         ,QString::fromUtf8(node.location.c_str()));
        add(tr("Node ID")           ,QString::fromStdString(node.id.toStdString()));
        add(tr("Local address")     ,QString::fromStdString(node.localAddr));
        add(tr("External address")  ,QString::fromStdString(node.extAddr));
        add(tr("Connected address") ,QString::fromStdString(node.connectAddr));

        for(auto iter(node.ipAddressList.begin()); iter != node.ipAddressList.end(); ++iter)
            add(tr("IP history"),QString::fromStdString(*iter));

        // Live connection type: "TCP-in", "Tor-out", "I2P-in"... Only meaningful while connected, which is
        // why the hidden node type below is searched separately.

        if(node.state & RS_PEER_STATE_CONNECTED)
            add(tr("Connection"),StatusDefs::connectStateIpString(node));

        if(node.isHiddenNode)
        {
            if(node.hiddenType & RS_HIDDEN_TYPE_TOR)
                add(tr("Hidden node type"),QString("Tor"));
            else if(node.hiddenType & RS_HIDDEN_TYPE_I2P)
                add(tr("Hidden node type"),QString("I2P"));

            add(tr("Hidden address"),QString::fromStdString(node.hiddenNodeAddress));
        }
    }

    return fields;
}

bool pgpid_item_proxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    RsPgpId peer_id = pgpIdOfRow(sourceRow, sourceParent);

    if(only_trusted_keys)
    {
        if(!rsPeers)
            return false;
        RsPeerDetails details;
        if(!rsPeers->getGPGDetails(peer_id, details))
            return false;
        if(details.validLvl < RS_TRUST_LVL_MARGINAL)
            return false;
    }

    // Multi-field search: if no text, accept all (that passed trusted-keys filter)
    if(mFilterText.isEmpty())
        return true;

    const QVector<QPair<QString,QString> > fields = searchFields(peer_id);

    for(auto field(fields.begin()); field != fields.end(); ++field)
        if(field->second.contains(mFilterText, Qt::CaseInsensitive))
            return true;

    return false;
}

// Node ids, IP addresses and connection types are searched but shown in no column, so a hit on one of them
// looks like a false positive. When a search is active, tell the user which fields actually matched, and
// leave the source model's own tooltips alone otherwise.

QVariant pgpid_item_proxy::data(const QModelIndex &index, int role) const
{
    if(role != Qt::ToolTipRole || mFilterText.isEmpty())
        return QSortFilterProxyModel::data(index, role);

    const QModelIndex source_index = mapToSource(index);
    const QVector<QPair<QString,QString> > fields = searchFields(pgpIdOfRow(source_index.row(), source_index.parent()));
    QStringList lines;

    for(auto field(fields.begin()); field != fields.end(); ++field)
        if(field->second.contains(mFilterText, Qt::CaseInsensitive))
            lines << QString("%1: %2").arg(field->first,field->second);

    if(lines.empty())
        return QSortFilterProxyModel::data(index, role);

    return QVariant(tr("Matched by search:") + "\n" + lines.join('\n'));
}
