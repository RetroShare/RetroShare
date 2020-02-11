/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdTreeWidgetItem.h                            *
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

#ifndef _GXS_ID_TREEWIDGETITEM_H
#define _GXS_ID_TREEWIDGETITEM_H

#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

#include "gui/common/RSTreeWidgetItem.h"
#include "gui/gxs/GxsIdDetails.h"

/*****
 * NOTE: When the tree item is created within a thread you have to move the object
 * with "moveThreadTo()" QObject into the main thread.
 * The tick signal to fill the id cannot be processed when the thread is finished.
 ***/

class GxsIdRSTreeWidgetItem : public QObject, public RSTreeWidgetItem
{
	Q_OBJECT

public:
    GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,bool auto_tooltip=true,QTreeWidget *parent = NULL);

	void setId(const RsGxsId &id, int column, bool retryWhenFailed);
	bool getId(RsGxsId &id);

	int idColumn() const { return mColumn; }
    void processResult(bool success);
    uint32_t iconTypeMask() const { return mIconTypeMask ;}

	void setAvatar(const RsGxsImage &avatar);
	virtual QVariant data(int column, int role) const;
	void forceUpdate();
    
    	void setBannedState(bool b) { mBannedState = b; }	// does not actually change the state, but used instead by callbacks to leave a trace
    	void updateBannedState() ;				// checks reputation, and update is needed

        bool autoTooltip() const { return mAutoTooltip; }
private slots:
	void startProcess();

private:
	void init();

	RsGxsId mId;
	int mColumn;
	bool mIdFound;
	bool mBannedState ;
	bool mRetryWhenFailed;
    bool mAutoTooltip;
	RsReputationLevel mReputationLevel;
	uint32_t mIconTypeMask;
	RsGxsImage mAvatar;
};

// This class is responsible of rendering authors of type RsGxsId in tree views. Used in forums, messages, etc.

class GxsIdTreeItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    GxsIdTreeItemDelegate()
    {
        mLoading = false;
        mReloadPeriod = 0;
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
		QStyleOptionViewItemV4 opt = option;
		initStyleOption(&opt, index);

		// disable default icon
		opt.icon = QIcon();
		const QRect r = option.rect;

        RsGxsId id(index.data(Qt::UserRole).toString().toStdString());
        QString str;
        QList<QIcon> icons;
        QString comment;

        QFontMetricsF fm(option.font);
        float f = fm.height();

		QIcon icon ;

		if(!GxsIdDetails::MakeIdDesc(id, true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
        {
			icon = GxsIdDetails::getLoadingIcon(id);
            launchAsyncLoading();
        }
		else
			icon = *icons.begin();

		QPixmap pix = icon.pixmap(r.size());

        return QSize(1.2*(pix.width() + fm.width(str)),std::max(1.1*pix.height(),1.4*fm.height()));
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex& index) const override
	{
		if(!index.isValid())
        {
            std::cerr << "(EE) attempt to draw an invalid index." << std::endl;
            return ;
        }

		QStyleOptionViewItemV4 opt = option;
		initStyleOption(&opt, index);

		// disable default icon
		opt.icon = QIcon();
		// draw default item
		QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

		const QRect r = option.rect;

        RsGxsId id(index.data(Qt::UserRole).toString().toStdString());
        QString str;
        QString comment;

        QFontMetricsF fm(painter->font());
        float f = fm.height();

		QIcon icon ;

        if(id.isNull())
        {
            str = tr("[Notification]");
            icon = QIcon(":/icons/logo_128.png");
        }
        else if(! computeNameIconAndComment(id,str,icon,comment))
			if(mReloadPeriod > 3)
			{
				str = tr("[Unknown]");
				icon = QIcon();
			}
			else
			{
				icon = GxsIdDetails::getLoadingIcon(id);
				launchAsyncLoading();
			}

		QPixmap pix = icon.pixmap(r.size());
		const QPoint p = QPoint(r.height()/2.0, (r.height() - pix.height())/2);

		// draw pixmap at center of item
		painter->drawPixmap(r.topLeft() + p, pix);
		painter->drawText(r.topLeft() + QPoint(r.height()+ f/2.0 + f/2.0,f*1.0), str);
	}

    void launchAsyncLoading() const
    {
        if(mLoading)
            return;

        mLoading = true;
        ++mReloadPeriod;

        QTimer::singleShot(1000,this,SLOT(reload()));
    }

    static bool computeName(const RsGxsId& id,QString& name)
    {
        QList<QIcon> icons;
        QString comment;

        if(rsPeers->isFriend(RsPeerId(id)))		// horrible trick because some widgets still use locations as IDs (e.g. messages)
			name = QString::fromUtf8(rsPeers->getPeerName(RsPeerId(id)).c_str()) ;
        else if(!GxsIdDetails::MakeIdDesc(id, false, name, icons, comment,GxsIdDetails::ICON_TYPE_NONE))
            return false;

        return true;
    }

    static bool computeNameIconAndComment(const RsGxsId& id,QString& name,QIcon& icon,QString& comment)
    {
        QList<QIcon> icons;

        if(rsPeers->isFriend(RsPeerId(id)))		// horrible trick because some widgets still use locations as IDs (e.g. messages)
        {
			name = QString::fromUtf8(rsPeers->getPeerName(RsPeerId(id)).c_str()) ;
            icon = QIcon(":/icons/avatar_128.png");
        }
        else if(!GxsIdDetails::MakeIdDesc(id, true, name, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
            return false;
		else
			icon = *icons.begin();

        return true;
    }

private slots:
    void reload() { mLoading=false; emit commitData(NULL) ;  }

private:
    mutable bool mLoading;
    mutable int mReloadPeriod;
};


#endif
