/*
 * Connect Progress Dialog
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

#ifndef _CONNECT_PROGRESS_DIALOG_H
#define _CONNECT_PROGRESS_DIALOG_H

#include <QDialog>

#include "ui_ConnectProgressDialog.h"

#include <stdint.h>

class ConnectProgressDialog : public QDialog
{
    Q_OBJECT

public:
    static void showProgress(const std::string& id);

private:
    ConnectProgressDialog(const std::string& id, QWidget *parent = 0, Qt::WFlags flags = 0);
    ~ConnectProgressDialog();

    static ConnectProgressDialog *instance(const std::string& peer_id);

private slots:
        void updateStatus();

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
        std::string mId;
        time_t mInitTS;

        time_t mContactTS;
        uint32_t mContactState;

        uint32_t mDhtStatus;

        time_t mLookupTS;
        uint32_t mLookupStatus;

        time_t mUdpTS;
        uint32_t mUdpStatus;

    /** Qt Designer generated object */
    Ui::ConnectProgressDialog *ui;
};

#endif
