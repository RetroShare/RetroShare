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

#ifndef MRK_IDENTITY_DIALOG_H
#define MRK_IDENTITY_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_IdDialog.h"

#include <retroshare/rsidentity.h>

#include <map>

#include "gui/Identity/IdEditDialog.h"
#include "util/TokenQueue.h"

class IdDialog : public MainPage, public TokenResponse
{
  Q_OBJECT

public:
	IdDialog(QWidget *parent = 0);

void    loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:


	void ListTypeToggled(bool checked);
	void checkUpdate();
	void OpenOrShowAddDialog();
	void OpenOrShowEditDialog();

	void updateSelection();

private:

	void blankSelection();
	void requestIdDetails(std::string &id);
	void insertIdDetails(uint32_t token);

	void requestIdList();
	void requestIdData(std::list<std::string> &ids);
	void insertIdList(uint32_t token);

	void requestRepList(const RsGxsGroupId &aboutId);
	void insertRepList(uint32_t token);

	void requestIdEdit(std::string &id);
	void showIdEdit(uint32_t token);

	IdEditDialog *mEditDialog;

	TokenQueue *mIdQueue;

	/* UI - from Designer */
	Ui::IdDialog ui;

};

#endif

