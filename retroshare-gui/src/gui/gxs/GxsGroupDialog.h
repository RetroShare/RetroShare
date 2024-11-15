/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupDialog.h                                 *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
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

#ifndef _GXS_GROUP_DIALOG_H
#define _GXS_GROUP_DIALOG_H

#include "ui_GxsGroupDialog.h"

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

/*** Group flags affect what is visually enabled that gets input into the grpMeta ***/

#define GXS_GROUP_FLAGS_NAME              0x00000001
#define GXS_GROUP_FLAGS_ICON              0x00000002
#define GXS_GROUP_FLAGS_DESCRIPTION       0x00000004
#define GXS_GROUP_FLAGS_DISTRIBUTION      0x00000008
#define GXS_GROUP_FLAGS_PUBLISHSIGN       0x00000010
#define GXS_GROUP_FLAGS_SHAREKEYS         0x00000020
#define GXS_GROUP_FLAGS_PERSONALSIGN      0x00000040
#define GXS_GROUP_FLAGS_COMMENTS          0x00000080
#define GXS_GROUP_FLAGS_EXTRA             0x00000100
#define GXS_GROUP_FLAGS_ANTI_SPAM         0x00000200
#define GXS_GROUP_FLAGS_ADDADMINS         0x00000400

/*** Default flags are used to determine privacy of group, signatures required ***
 *** whether publish or id and whether comments are allowed or not             ***/

#define GXS_GROUP_DEFAULTS_DISTRIB_MASK               0x0000000f
#define GXS_GROUP_DEFAULTS_PUBLISH_MASK               0x000000f0
#define GXS_GROUP_DEFAULTS_PERSONAL_MASK              0x00000f00
#define GXS_GROUP_DEFAULTS_COMMENTS_MASK              0x0000f000

#define GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC             0x00000001
#define GXS_GROUP_DEFAULTS_DISTRIB_GROUP              0x00000002
#define GXS_GROUP_DEFAULTS_DISTRIB_LOCAL              0x00000004

#define GXS_GROUP_DEFAULTS_PUBLISH_OPEN               0x00000010
#define GXS_GROUP_DEFAULTS_PUBLISH_THREADS            0x00000020
#define GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED           0x00000040
#define GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED          0x00000080

#define GXS_GROUP_DEFAULTS_PERSONAL_PGP               0x00000100
#define GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED          0x00000200
#define GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB           0x00000400
// independent from other PERSONAL FLAGS. If Group requires a AuthorId.
#define GXS_GROUP_DEFAULTS_PERSONAL_GROUP             0x00000800

#define GXS_GROUP_DEFAULTS_COMMENTS_YES               0x00001000
#define GXS_GROUP_DEFAULTS_COMMENTS_NO                0x00002000


#define GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP         0x00100000
#define GXS_GROUP_DEFAULTS_ANTISPAM_TRACK             0x00200000
#define GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP_KNOWN   0x00400000

/*!
 * The aim of this dialog is to be convenient to encapsulate group
 * creation code for several GXS services such forums, channels
 * and posted
 * The functionality provided are for group creation are:
 * - Specifying the authentication type of the group
 * - Specifying group image
 * -
 * The main limitation is that it will not deal with the actual service GXS Group
 * data structure, but the meta structure which is the same across GXS services
 * The long term plan is perhap logic structure (i.e. code) will be moved into each GXS \n
 * service for better customisation of group creation, or perhaps not!
 */
class GxsGroupDialog : public QDialog
{
	Q_OBJECT

public:
	enum Mode {
		MODE_CREATE,
		MODE_SHOW,
		MODE_EDIT
	};

	enum UiType {
		UITYPE_SERVICE_HEADER,
		UITYPE_KEY_SHARE_CHECKBOX,
		UITYPE_ADD_ADMINS_CHECKBOX,
		UITYPE_CONTACTS_DOCK,
		UITYPE_BUTTONBOX_OK
	};

public:

	/*!
	 * Constructs a GxsGroupDialog for creating group
	 * @param enableFlags This determines what options are enabled such as Icon, Description, publish type and key sharing
	 * @param defaultFlags This deter
	 * @param parent The parent dialog
	 * @param mode
	 */
	GxsGroupDialog(uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent = NULL);

	/*!
	 * Contructs a GxsGroupDialog for display a group or editing
	 * @param grpMeta This is used to fill out the dialog
	 * @param mode This determines whether the dialog starts in show or edit mode (Edit not supported yet)
	 * @param parent
	 */
	GxsGroupDialog(Mode mode, RsGxsGroupId groupId, uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent = NULL);

	~GxsGroupDialog();

	uint32_t mode() { return mMode; }

private:
	void newGroup();
	void init();
	void initMode();

	// Functions that can be overloaded for specific stuff.

protected slots:
	void submitGroup();
	void addGroupLogo();
	void filterComboBoxChanged(int);

protected:
	virtual void showEvent(QShowEvent*);

	virtual void initUi() = 0;
	virtual QPixmap serviceImage() = 0;
	virtual QIcon serviceWindowIcon();

	/*!
	 * Inject Extra Widget for additional Group configuration options.
	 * NB: These are only inserted for createMode currently.
	 * @param widget Addtional widget which is added to extraFrame.
	 */
        void injectExtraWidget(QWidget *widget);

    /*!
     * \brief setUiToolTip/setUiText
     * 		Sets the text and tooltip of some parts of the UI
     * \param uiType	widget to set
     * \param text		text to set
     */
	void setUiToolTip(UiType uiType, const QString &text);
	void setUiText   (UiType uiType, const QString &text);

    /*!
     * It is up to the service to retrieve its own group data, which derives from RsGxsGenericGroupData. That data will be passed down
     * to the service itself for specific tasks.
     * \param grpId  Id of the group to retrieve
     * \param data   Generic group data for this group. /!\ The pointer should be deleted by the client when released.
     * \return       True if everything does fine.
     */
    virtual bool service_getGroupData(const RsGxsGroupId& grpId,RsGxsGenericGroupData *& data) = 0;

	/*!
	 * It is up to the service to do the actual group creation
	 * Service can also modify initial meta going into group
	 * @param meta The deriving GXS service should set their grp meta to this value
	 */
	virtual bool service_createGroup(RsGroupMetaData& meta) = 0;

	/*!
	 * It is up to the service to do the actual group editing
	 * @param meta The deriving GXS service should set their grp meta to this value
	 */
	virtual bool service_updateGroup(const RsGroupMetaData& editedMeta) = 0;

    /*!
     * Should be overloaded by the service in order to extract meaningful information from the group data (that is usually group-specific).
     * One of them however, common to all groups is the description. So it is returned by this method so that the GxsGroupDialog updates it.
     * \param data            Generic group data, to be dynamic_cast by the client to specific service-level group data
     * \param mode            Editing mode (?)
     * \param description     Description string for the group. Common to all services, but still present in the service-specific data part.
     * \return
     */
	virtual bool service_loadGroup(const RsGxsGenericGroupData *data, Mode mode, QString &description) = 0;

	/*!
	 * This returns a group logo from the ui \n
	 * Should be calleld by deriving service
	 * @return The logo for the service
	 */
	QPixmap getLogo();

	/*!
	 * This sets a group logo into the ui \n
	 * Should be calleld by deriving service
	 * @param pixmap
	 */
	void setLogo(const QPixmap &pixmap);

	/*!
	 * This returns a group name string from the ui
	 * @return group name string
	 */
	QString getName();

	/*!
	 * This returns a group description string from the ui
	 * @return group description string
	 */
	QString getDescription();

    /*!
     * \brief getSelectedModerators
     * 			Returns the set of ids that hve been selected as moderators.
     */
	void getSelectedModerators(std::set<RsGxsId>& ids);
	void setSelectedModerators(const std::set<RsGxsId>& ids);

private slots:
	/* actions to take.... */
	void cancelDialog();

	// set private forum key share list
	void setShareList();
	void setAdminsList();

	void updateCircleOptions();

private:
	bool setCircleParameters(RsGroupMetaData &meta);

	void setGroupSignFlags(uint32_t signFlags);
	uint32_t getGroupSignFlags();

	void setAllReadonly();
	void setupReadonly();
	void setupDefaults();
	void setupVisibility();
	void clearForm();
	void createGroup();
	void editGroup();
	void sendShareList(std::string forumId);
	void loadNewGroupId(const uint32_t &token);

	// loading existing Groups.
	void loadGroup(const RsGxsGroupId &groupId);
	void updateFromExistingMeta(const QString &description);

	bool prepareGroupMetaData(RsGroupMetaData &meta, QString &reason);

	std::list<std::string> mShareList;
	QPixmap mPicture;
	RsGroupMetaData mGrpMeta;

	Mode mMode;
	uint32_t mEnabledFlags;
	uint32_t mReadonlyFlags;
	uint32_t mDefaultsFlags;

protected:
	/** Qt Designer generated object */
	Ui::GxsGroupDialog ui;
};

#endif
