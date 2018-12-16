/*******************************************************************************
 * gui/connect/ConnectProgressDialog.h                                         *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie <retroshare.project@gmail.com>         *
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

#ifndef _CONNECT_PROGRESS_DIALOG_H
#define _CONNECT_PROGRESS_DIALOG_H

#include <QDialog>

#include "ui_ConnectProgressDialog.h"

#include <stdint.h>
#include <retroshare/rstypes.h>

class ConnectProgressDialog : public QDialog
{
    Q_OBJECT

public:
    static void showProgress(const RsPeerId& id);

private:
    ConnectProgressDialog(const RsPeerId& id, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~ConnectProgressDialog();

    static ConnectProgressDialog *instance(const RsPeerId& peer_id);

private slots:
        void updateStatus();
        void stopAndClose();

private:

        void initDialog();

        void updateNetworkStatus();
        void updateContactStatus();
        void updateDhtStatus();
        void updateLookupStatus();
        void updateUdpStatus();

	void setStatusMessage(uint32_t status, const QString &title, const QString &message);

	// good stuff.
	void sayInProgress();
	void sayConnected();

	// Peer errors.
	void sayDenied();
	void sayUdpDenied();
	void sayPeerOffline();
	void sayPeerNoDhtConfig();

	// Config errors.
	void sayDHTFailed();
	void sayDHTOffline();
	void sayInvalidPeer();

	// Unknown errors.
	void sayConnectTimeout();
	void sayLookupTimeout();
	void sayUdpTimeout();
	void sayUdpFailed();

        QTimer *mTimer;
        uint32_t mState;
        RsPeerId mId;
        time_t mInitTS;

        time_t mContactTS;
        uint32_t mContactState;

        uint32_t mDhtStatus;

        time_t mLookupTS;
        uint32_t mLookupStatus;

        time_t mUdpTS;
        uint32_t mUdpStatus;

		bool mAmIHiddenNode ;
		bool mIsPeerHiddenNode ;

    /** Qt Designer generated object */
    Ui::ConnectProgressDialog *ui;
};

#endif
