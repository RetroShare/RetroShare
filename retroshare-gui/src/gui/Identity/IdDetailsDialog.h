/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdDetailsDialog.h                           *
 *                                                                             *
 *  Copyright (C) 2014 - 2010 RetroShare Team <retroshare.project@gmail.com>   *
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

#ifndef _IDDETAILSDIALOG_H
#define _IDDETAILSDIALOG_H

#include <QDialog>

#include "util/TokenQueue.h"
#include <retroshare/rsidentity.h>

namespace Ui {
class IdDetailsDialog;
}

class UIStateHelper;

class IdDetailsDialog : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	/** Default constructor */
	IdDetailsDialog(const RsGxsGroupId &id, QWidget *parent = 0);
	/** Default destructor */
	~IdDetailsDialog();

	/* TokenResponse */
	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void modifyReputation();
	void toggleAutoBanIdentities(bool b);
	
	static QString inviteMessage();
	void sendInvite();
private :
	void requestIdDetails();
	void insertIdDetails(uint32_t token);
	
  void requestRepList();
	void insertRepList(uint32_t token);

private:
	RsGxsGroupId mId;
	TokenQueue *mIdQueue;
	UIStateHelper *mStateHelper;

	/** Qt Designer generated object */
	Ui::IdDetailsDialog *ui;
};

#endif
