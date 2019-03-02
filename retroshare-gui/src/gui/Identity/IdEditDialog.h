/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdEditDialog.h                              *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef IDEDITDIALOG_H
#define IDEDITDIALOG_H

#include <inttypes.h>

#include "util/TokenQueue.h"
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsifacetypes.h>
#include <QDialog>

class UIStateHelper;

namespace Ui {
class IdEditDialog;
}

class IdEditDialog : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	IdEditDialog(QWidget *parent = 0);
	~IdEditDialog();

	void setupNewId(bool pseudo, bool enable_anon = true);
	void setupExistingId(const RsGxsGroupId &keyId);
    	void enforceNoAnonIds() ;

	RsGxsGroupId groupId() { return mGroupId; }

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void idTypeToggled(bool checked);
	void submit();

	void changeAvatar();
    void removeAvatar();

	void addRecognTag();
	void checkNewTag();
	void rmTag1();
	void rmTag2();
	void rmTag3();
	void rmTag4();
	void rmTag5();

private:
	void createId();
	void updateId();
	void updateIdType(bool pseudo);
	void loadExistingId(uint32_t token);
	void setAvatar(const QPixmap &avatar);
	void idCreated(uint32_t token);

	void loadRecognTags();
	// extract details.
	bool tagDetails(const RsGxsId &id, const std::string &name, const std::string &tag, QString &desc);
	void rmTag(int idx);
	
protected:
	Ui::IdEditDialog *ui;
	bool mIsNew;
	UIStateHelper *mStateHelper;

	RsGxsIdGroup mEditGroup;

	TokenQueue *mIdQueue;
	RsGxsGroupId mGroupId;

	QPixmap mAvatar; // Avatar from identity (not calculated)
};

#endif
