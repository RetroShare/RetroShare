/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdChooser.h                                   *
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

#ifndef _GXS_ID_CHOOSER_H
#define _GXS_ID_CHOOSER_H

#include "retroshare/rsevents.h"
#include "gui/common/RSComboBox.h"

#include "retroshare/rsgxsifacetypes.h"

// This class implement a basic RS functionality which is that ComboBox displaying Id
// should update regularly. They also should update only when visible, to save CPU time.
//

class RsGxsUpdateBroadcastBase;

#define IDCHOOSER_ID_REQUIRED   0x0001
#define IDCHOOSER_ANON_DEFAULT  0x0002
#define IDCHOOSER_NO_CREATE     0x0004
#define IDCHOOSER_NON_ANONYMOUS 0x0008

class GxsIdChooser : public RSComboBox
{
	Q_OBJECT

public:
	GxsIdChooser(QWidget *parent = nullptr);
	virtual ~GxsIdChooser();

	void setFlags(uint32_t flags) ;
    uint32_t flags() const { return mFlags ; }

	enum ChosenId_Ret {None, KnowId, UnKnowId, NoId} ;
	void loadIds(uint32_t chooserFlags, const RsGxsId &defId);

	void setDefaultId(const RsGxsId &defId);
    const RsGxsId defaultId() const { return mDefaultId ; }

	bool setChosenId(const RsGxsId &gxsId);
	ChosenId_Ret getChosenId(RsGxsId &gxsId);

	void setEntryEnabled(int index, bool enabled);
    
	void setIdConstraintSet(const std::set<RsGxsId>& s) ;
	bool isInConstraintSet(const RsGxsId& id) const ;
        
	uint32_t countEnabledEntries() const ;
signals:
    // emitted after first load of own ids
    void idsLoaded();

protected:
	virtual void showEvent(QShowEvent *event);
    void updateDisplay(bool reset);

private slots:
	void fillDisplay(bool complete);
	void myCurrentIndexChanged(int index);
	void indexActivated(int index);

private:
    void loadPrivateIds();
    void setDefaultItem();
    void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

	uint32_t mFlags;
	RsGxsId mDefaultId;
	bool mFirstLoad;
    uint32_t mAllowedCount ;

    std::set<RsGxsId> mConstraintIdsSet ; // leave empty if all allowed
//    RsGxsUpdateBroadcastBase *mBase;

    RsEventsHandlerId_t mEventHandlerId;
};

#endif
