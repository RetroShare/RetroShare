/*
 * Retroshare Identity.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef IDEDITDIALOG_H
#define IDEDITDIALOG_H

#include "ui_IdEditDialog.h"

#include <inttypes.h>

#include "util/TokenQueue.h"
#include <retroshare/rsidentity.h>

class UIStateHelper;

class IdEditDialog : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	IdEditDialog(QWidget *parent = 0);

	void setupNewId(bool pseudo);
	void setupExistingId(std::string keyId);

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void idTypeToggled(bool checked);
	void submit();

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

	void loadRecognTags();
	// extract details.
	bool tagDetails(const std::string &id, const std::string &name, const std::string &tag, QString &desc);
	void rmTag(int idx);
	
protected:
	Ui::IdEditDialog ui;
	bool mIsNew;
	UIStateHelper *mStateHelper;
	
	RsGxsIdGroup mEditGroup;

	TokenQueue *mIdQueue;
};

#endif
