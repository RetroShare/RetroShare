/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


#ifndef _GXS_GROUP_DIALOG_H
#define _GXS_GROUP_DIALOG_H

#include "ui_GxsGroupDialog.h"

#include "util/TokenQueue.h"


/********
 * Notes:
 *
 * This is a generic Group Dialog, which is expected to be overloaded to include
 * specific data for a specific service.
 *
 * To acheive this, we have a GxsGroupExtension widget
 *  - programmatically added to the GUI.
 *  - called to fill in fields in the Data Structures.
 *
 * To enable extension, we have a bunch of flags so various options can 
 *    be shown / disabled for each specific service.
 * 	- Enabled.
 * 	- Defaults.
 * 	- ReadOnly
 *
 * This form will be used for Create/Edit & View of Group Info.
 */

class GxsGroupExtension: public QWidget
{
public:
	GxsGroupExtension() : QWidget() { return; }

};


#define GXS_GROUP_FLAGS_ICON			0x00000001
#define GXS_GROUP_FLAGS_DESCRIPTION		0x00000002
#define GXS_GROUP_FLAGS_DISTRIBUTION		0x00000004
#define GXS_GROUP_FLAGS_PUBLISHSIGN		0x00000008
#define GXS_GROUP_FLAGS_SHAREKEYS		0x00000010
#define GXS_GROUP_FLAGS_PERSONALSIGN		0x00000020
#define GXS_GROUP_FLAGS_COMMENTS		0x00000040

#define GXS_GROUP_FLAGS_EXTRA			0x00000100

#define GXS_GROUP_DEFAULTS_DISTRIB_MASK		0x0000000f
#define GXS_GROUP_DEFAULTS_PUBLISH_MASK		0x000000f0
#define GXS_GROUP_DEFAULTS_PERSONAL_MASK	0x00000f00
#define GXS_GROUP_DEFAULTS_COMMENTS_MASK	0x0000f000

#define GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC	0x00000001
#define GXS_GROUP_DEFAULTS_DISTRIB_GROUP	0x00000002
#define GXS_GROUP_DEFAULTS_DISTRIB_LOCAL	0x00000004

#define GXS_GROUP_DEFAULTS_PUBLISH_OPEN		0x00000010
#define GXS_GROUP_DEFAULTS_PUBLISH_THREADS	0x00000020
#define GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED	0x00000040
#define GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED	0x00000080

#define GXS_GROUP_DEFAULTS_PERSONAL_PGP		0x00000100
#define GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED	0x00000200
#define GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB	0x00000400

#define GXS_GROUP_DEFAULTS_COMMENTS_YES		0x00001000
#define GXS_GROUP_DEFAULTS_COMMENTS_NO		0x00002000

#define GXS_GROUP_DIALOG_CREATE_MODE		1
#define GXS_GROUP_DIALOG_SHOW_MODE		2
#define GXS_GROUP_DIALOG_EDIT_MODE		3

class GxsGroupDialog : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	GxsGroupDialog(RsTokenService *service, QWidget *parent = 0);

	void setFlags(uint32_t enabledFlags, uint32_t readonlyFlags, uint32_t defaultFlags);
	void setMode(uint32_t mode);

	/*** Open Window -> in Correct Mode */
	void newGroup(); 
	void existingGroup(std::string groupId, uint32_t mode);

        // Callback for all Loads.
virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);


	// Functions that can be overloaded for specific stuff.

protected slots:
	void submitGroup();
	void addGroupLogo(); 

protected:

	// Functions to be overloaded.

	// Group Creation.
	virtual bool service_NewGroup();
	virtual bool service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta);
	virtual bool service_CompleteCreateGroup(const RsGroupMetaData &meta);

	// Group Edit / Display.
	virtual bool service_ExistingGroup(); // Initial Setup.
	virtual void service_loadExistingGroup(const uint32_t &token);

	bool loadExistingGroupMetaData(const RsGroupMetaData &meta);

	
private slots:
	/* actions to take.... */
	void cancelDialog();

	// set private forum key share list
	void setShareList();

private:

	void setGroupSignFlags(uint32_t signFlags);
	uint32_t getGroupSignFlags();

	void setupDefaults();
	void setupVisibility();
	void clearForm();

	void createGroup();
	
	virtual void completeCreateGroup(const RsGroupMetaData &newForumMeta);


	void sendShareList(std::string forumId);

	void loadNewGroupId(const uint32_t &token);

	std::list<std::string> mShareList;

	QPixmap picture;

	RsTokenService *mRsService;
	TokenQueue *mTokenQueue;

	uint32_t mMode;

	uint32_t mEnabledFlags;
	uint32_t mReadonlyFlags;
	uint32_t mDefaultsFlags;

	/** Qt Designer generated object */
	Ui::GxsGroupDialog ui;
};

#endif

