/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef _GXS_ID_CHOOSER_H
#define _GXS_ID_CHOOSER_H

#include <QComboBox>
#include <QPushButton>
#include "util/TokenQueue.h"
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsifacetypes.h>

// This class implement a basic RS functionality which is that ComboBox displaying Id
// should update regularly. They also should update only when visible, to save CPU time.
//

class RsGxsIfaceHelper;
class RsGxsUpdateBroadcastBase;

#define IDCHOOSER_ID_REQUIRED   0x0001
#define IDCHOOSER_ANON_DEFAULT  0x0002
#define IDCHOOSER_NO_CREATE     0x0004

class GxsIdChooser : public QComboBox, public TokenResponse
{
	Q_OBJECT

public:
	GxsIdChooser(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = NULL);
	GxsIdChooser(QWidget *parent = NULL);
	virtual ~GxsIdChooser();

	void setFlags(uint32_t flags) ;

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);//TokenResponse

	enum ChosenId_Ret {None, KnowId, UnKnowId, NoId} ;
	void loadIds(uint32_t chooserFlags, RsGxsId defId);
	void setDefaultId(RsGxsId defId) {mDefaultId=defId;}
	void setDefaultId(std::string defIdName) {mDefaultIdName=defIdName;}

	bool setChosenId(RsGxsId &gxsId);
	ChosenId_Ret getChosenId(RsGxsId &gxsId);

protected:
	virtual void showEvent(QShowEvent *event);
	void updateDisplay(bool complete);

private slots:
	void fillDisplay(bool complete);
	void myCurrentIndexChanged(int index);
	void indexActivated(int index);

private:
	void requestIdList() ;
	void loadPrivateIds(uint32_t token);
	void setDefaultItem();

	uint32_t mFlags;
	RsGxsId mDefaultId;
	std::string mDefaultIdName;
	bool mFirstLoad;
	QPushButton* addNewCxsId;

	TokenQueue *mIdQueue;
	RsGxsUpdateBroadcastBase *mBase;
};

#endif
