/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdChooser.cpp                                 *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie     <retroshare.project@gmail.com>     *
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

#include "GxsIdChooser.h"
#include "GxsIdDetails.h"
#include "RsGxsUpdateBroadcastBase.h"
#include "gui/Identity/IdEditDialog.h"
#include "util/misc.h"

#include <retroshare/rspeers.h>

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <algorithm>

#include <iostream>

#define ROLE_SORT Qt::UserRole + 1 // Qt::UserRole is reserved for data
#define ROLE_TYPE Qt::UserRole + 2 //

/* Used for sorting too */
#define TYPE_NO_ID       1
#define TYPE_FOUND_ID    2
#define TYPE_UNKNOWN_ID  3
#define TYPE_CREATE_ID   4

#define BANNED_ICON ":/icons/yellow_biohazard64.png"

#define IDCHOOSER_REFRESH  1

//#define IDCHOOSER_DEBUG

/** Constructor */
GxsIdChooser::GxsIdChooser(QWidget *parent)
    : QComboBox(parent), mFlags(IDCHOOSER_ANON_DEFAULT)
{
	mBase = new RsGxsUpdateBroadcastBase(rsIdentity, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));

	/* Initialize ui */
	setSizeAdjustPolicy(QComboBox::AdjustToContents);

	mFirstLoad = true;
    	mAllowedCount = 0 ;

	mDefaultId.clear() ;

	/* Enable sort with own role */
	QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(model());
	proxy->setDynamicSortFilter(false);
	model()->setParent(proxy);
	setModel(proxy);

	proxy->setSortRole(ROLE_SORT);

	/* Connect signals */
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(myCurrentIndexChanged(int)));
	connect(this, SIGNAL(activated(int)), this, SLOT(indexActivated(int)));
}

void GxsIdChooser::setFlags(uint32_t flags)
{
	mFlags = flags ;
	updateDisplay(true);
}

GxsIdChooser::~GxsIdChooser()
{
}

void GxsIdChooser::fillDisplay(bool complete)
{
	updateDisplay(complete);
	update(); // Qt flush
}

void GxsIdChooser::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	QComboBox::showEvent(event);
}

void GxsIdChooser::setIdConstraintSet(const std::set<RsGxsId>& s) 
{
    mConstraintIdsSet = s ;
    
	updateDisplay(true);
	update(); // Qt flush
}
void GxsIdChooser::loadIds(uint32_t chooserFlags, const RsGxsId &defId)
{
	mFlags = chooserFlags;
	mDefaultId = defId;
	clear();
	mFirstLoad = true;
    
	updateDisplay(true);
	update(); // Qt flush
}

void GxsIdChooser::setDefaultId(const RsGxsId &defId)
{
	mDefaultId = defId;
}

static void loadPrivateIdsCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	GxsIdChooser *chooser = dynamic_cast<GxsIdChooser*>(object);

	if (!chooser)
		return;

    // this prevents the objects that depend on what's in the combo-box to activate and
    // perform any change.Only user-changes should cause this.
    chooser->blockSignals(true) ;

	QString text = GxsIdDetails::getNameForType(type, details);
	QString id = QString::fromStdString(details.mId.toStdString());

	/* Find and replace text of exisiting item */
	int index = chooser->findData(id);
	if (index >= 0) {
		chooser->setItemText(index, text);
	} else {
		/* Add new item */
		chooser->addItem(text, id);
		index = chooser->count() - 1;
	}

	QList<QIcon> icons;

	switch (type) {
	case GXS_ID_DETAILS_TYPE_EMPTY:
	case GXS_ID_DETAILS_TYPE_FAILED:
		break;

	case GXS_ID_DETAILS_TYPE_LOADING:
		icons.push_back(GxsIdDetails::getLoadingIcon(details.mId));
		break;

	case GXS_ID_DETAILS_TYPE_DONE:
		GxsIdDetails::getIcons(details, icons, GxsIdDetails::ICON_TYPE_AVATAR);
		break;
        
	case GXS_ID_DETAILS_TYPE_BANNED:
		icons.push_back(QIcon(BANNED_ICON)) ;
		break;
	}

	chooser->setItemData(index, QString("%1_%2").arg((type == GXS_ID_DETAILS_TYPE_DONE) ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID).arg(text), ROLE_SORT);
	chooser->setItemData(index, (type == GXS_ID_DETAILS_TYPE_DONE) ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID, ROLE_TYPE);
	chooser->setItemIcon(index, icons.empty() ? QIcon() : icons[0]);

    	//std::cerr << "ID=" << details.mId << ", chooser->flags()=" << chooser->flags() << ", flags=" << details.mFlags ;
        
    	if((chooser->flags() & IDCHOOSER_NON_ANONYMOUS) && !(details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
        {
            //std::cerr << " - disabling ID - entry = " << index << std::endl;
            chooser->setEntryEnabled(index,false) ;
        }
        
        if(!chooser->isInConstraintSet(details.mId))
            chooser->setEntryEnabled(index,false) ;
        
    chooser->model()->sort(0);

	// now restore the current item. Problem is, we cannot use the ID position because it may have changed.
#ifdef IDCHOOSER_DEBUG
    std::cerr << "GxsIdChooser: default ID = " << chooser->defaultId() << std::endl;
#endif
	if(!chooser->defaultId().isNull())
	{
		for(int indx=0;indx<chooser->count();++indx)
			if(RsGxsId(chooser->itemData(indx).toString().toStdString()) == chooser->defaultId())
			{
				chooser->setCurrentIndex(indx);
#ifdef IDCHOOSER_DEBUG
				std::cerr << "GxsIdChooser-003 " << (void*)chooser << " setting current index to " << indx << " because it has ID=" << chooser->defaultId() << std::endl;
#endif
				break;
			}
	}
	else
    {
		RsGxsId id;
        GxsIdChooser::ChosenId_Ret cid = chooser->getChosenId(id) ;

        if(cid == GxsIdChooser::UnKnowId || cid == GxsIdChooser::KnowId)
            chooser->setDefaultId(id) ;
    }

    chooser->blockSignals(false) ;
}

bool GxsIdChooser::isInConstraintSet(const RsGxsId& id) const 
{
            if(mConstraintIdsSet.empty())	// special case: empty set means no constraint
            	return true ;
            
            return mConstraintIdsSet.find(id) != mConstraintIdsSet.end() ;
}
void GxsIdChooser::setEntryEnabled(int indx,bool /*enabled*/)
{
    removeItem(indx) ;
    
#ifdef TO_REMOVE
//    bool disable = !enabled ;
//    
//    QSortFilterProxyModel* model = qobject_cast<QSortFilterProxyModel*>(QComboBox::model());
//    //QStandardItem* item = model->item(index);
//    
//    QModelIndex ii = model->index(indx,0);
//    
//    // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
//    //model->setFlags(ii,disable ? (model->flags(ii) & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)) : (Qt::ItemIsSelectable|Qt::ItemIsEnabled));
//    
//    uint32_t v = enabled?(1|32):(0);
//    
//    std::cerr << "GxsIdChooser::setEnabledEntry: i=" << indx << ", v=" << v << std::endl;
//    
//    // clear item data in order to use default color
//    //model->setData(ii,disable ? (QComboBox::palette().color(QPalette::Disabled, QPalette::Text)) : QVariant(),  Qt::TextColorRole);
//    model->setData(ii,QVariant(v),Qt::UserRole-1) ;
//    
//    std::cerr << "model data after operation: " <<  model->data(ii,Qt::UserRole-1).toUInt() << std::endl;
#endif
}

uint32_t GxsIdChooser::countEnabledEntries() const
{
    return count() ;
    
#ifdef TO_REMOVE
//    uint32_t res = 0 ;
//    QSortFilterProxyModel* model = qobject_cast<QSortFilterProxyModel*>(QComboBox::model());
//
//    for(uint32_t i=0;i<model->rowCount();++i)
//    {
//	    QModelIndex ii = model->index(i,0);
//	    uint32_t v = model->data(ii,Qt::UserRole-1).toUInt() ;
//        
//        	std::cerr << "GxsIdChooser::countEnabledEntries(): i=" << i << ", v=" << v << std::endl;
//	    if(v > 0)
//		    ++res ;
//    }
//
//    return res ;
#endif
}

void GxsIdChooser::loadPrivateIds()
{
	if (mFirstLoad) {
		//whileBlocking doesn't work here.
		bool prev = this->blockSignals(true);
		clear();
		this->blockSignals(prev);
	}

	std::list<RsGxsId> ids;
    rsIdentity->getOwnIds(ids);

	//rsIdentity->getDefaultId(defId);
	// Prefer to use an application specific default???

	if (mFirstLoad) {
		if (!(mFlags & IDCHOOSER_ID_REQUIRED)) {
			/* add No Signature option */
			QString str = tr("No Signature");
			QString id = "";

			addItem(str, id);
			setItemData(count() - 1, QString("%1_%2").arg(TYPE_NO_ID).arg(str), ROLE_SORT);
			setItemData(count() - 1, TYPE_NO_ID, ROLE_TYPE);
		}
	} else {
		for (int idx = 0; idx < count(); ++idx) {
			QVariant type = itemData(idx, ROLE_TYPE);
			switch (type.toInt()) {
			case TYPE_NO_ID:
			case TYPE_CREATE_ID:
				break;
			case TYPE_FOUND_ID:
			case TYPE_UNKNOWN_ID:
				{
					QVariant var = itemData(idx);
					RsGxsId gxsId = RsGxsId(var.toString().toStdString());
					std::list<RsGxsId>::iterator lit = std::find(ids.begin(), ids.end(), gxsId);
					if (lit == ids.end()) {
						removeItem(idx);
						idx--;
					}
				}
			}
		}
	}

	for (std::list<RsGxsId>::iterator it = ids.begin(); it != ids.end(); ++it) {
	    GxsIdDetails::process(*it, loadPrivateIdsCallback, this); /* add to Chooser */
	}

	if (mFirstLoad) {
		if (!(mFlags & IDCHOOSER_NO_CREATE)) {
			/* add Create Identity option */
			QString str = tr("Create new Identity");
			QString id = "";

			addItem(QIcon(":/images/identity/identity_create_32.png"), str, id);
			setItemData(count() - 1, QString("%1_%2").arg(TYPE_CREATE_ID).arg(str), ROLE_SORT);
			setItemData(count() - 1, TYPE_CREATE_ID, ROLE_TYPE);
            
                    
		}
        setDefaultItem();
        emit idsLoaded();
    }

	mFirstLoad = false;
}

void GxsIdChooser::setDefaultItem()
{
	int def = -1;

	if ((mFlags & IDCHOOSER_ANON_DEFAULT) && !(mFlags & IDCHOOSER_ID_REQUIRED)) {
		def = findData(TYPE_NO_ID, ROLE_TYPE);
	} else {
		if (!mDefaultId.isNull()) {
			QString id = QString::fromStdString(mDefaultId.toStdString());
			def = findData(id);
		}
	}

	if (def >= 0) {
		setCurrentIndex(def);
        std::cerr << "GxsIdChooser-002" << (void*)this << " setting current index to " << def << std::endl;
	}
}

bool GxsIdChooser::setChosenId(const RsGxsId &gxsId)
{
	QString id = QString::fromStdString(gxsId.toStdString());

	/* Find text of exisiting item */
	int index = findData(id);
	if (index >= 0) {
		setCurrentIndex(index);
        std::cerr << "GxsIdChooser-001" << (void*)this << " setting current index to " << index << std::endl;
		return true;
	}
	return false;
}

GxsIdChooser::ChosenId_Ret GxsIdChooser::getChosenId(RsGxsId &gxsId)
{
	if (count() < 1) {
		return None;
	}

	int idx = currentIndex();

	QVariant var = itemData(idx);
	gxsId = RsGxsId(var.toString().toStdString());
	QVariant type = itemData(idx, ROLE_TYPE);
	switch (type.toInt()) {
	case TYPE_NO_ID:
		return NoId;
	case TYPE_FOUND_ID:
		return KnowId;
	case TYPE_UNKNOWN_ID:
		return UnKnowId;
	case TYPE_CREATE_ID:
		break;
	}

	return None;
}

void GxsIdChooser::myCurrentIndexChanged(int index)
{
	Q_UNUSED(index);

	QFontMetrics fm = QFontMetrics(font());
	QString text = currentText();
	if (width() < fm.boundingRect(text).width()) {
		setToolTip(text);
	} else {
		setToolTip("");
	}
	QVariant var = itemData(index);
	RsGxsId gxsId(var.toString().toStdString());

    if(!gxsId.isNull())
        mDefaultId = gxsId;
}

void GxsIdChooser::indexActivated(int index)
{
	int type = itemData(index, ROLE_TYPE).toInt();
	if (type == TYPE_CREATE_ID) {
		IdEditDialog dlg(this);
		dlg.setupNewId(false, !(mFlags & IDCHOOSER_NON_ANONYMOUS));
		if (dlg.exec() == QDialog::Accepted) {
			setDefaultId(RsGxsId(dlg.groupId()));
		}
	}
}

void GxsIdChooser::updateDisplay(bool reset)
{
    if(reset)
        mFirstLoad = true ;

	/* Update identity list */
    loadPrivateIds();
}
