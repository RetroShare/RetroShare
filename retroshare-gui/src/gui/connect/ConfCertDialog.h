/*******************************************************************************
 * gui/connect/ConfCertDialog.h                                                *
 *                                                                             *
 * Copyright (C) 2006 Crypton         <retroshare.project@gmail.com>           *
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

#ifndef _CONFCERTDIALOG_H
#define _CONFCERTDIALOG_H

#include <QDialog>

#include "ui_ConfCertDialog.h"
#include <retroshare/rstypes.h>
#include <retroshare/rspeers.h>

class ConfCertDialog : public QDialog
{
    Q_OBJECT

public:

	enum enumPage { PageDetails = 0, PageTrust = 1, PageCertificate = 2 };

    template<class ID_CLASS> static void showIt(const ID_CLASS& id, enumPage page)
    {
        ConfCertDialog *confdialog = instance(id);

        switch (page) {
        case PageDetails:
            confdialog->ui.stabWidget->setCurrentIndex(PageDetails);
            break;
        case PageTrust:
            confdialog->ui.stabWidget->setCurrentIndex(PageTrust);
            break;
        case PageCertificate:
            confdialog->ui.stabWidget->setCurrentIndex(PageCertificate);
            break;
        }

        confdialog->load();
        confdialog->show();
        confdialog->raise();
        confdialog->activateWindow();

        /* window will destroy itself! */
    }
    static void loadAll();
    static QString getCertificateDescription(const RsPeerDetails& det, bool signatures_included, bool use_short_format,RetroshareInviteFlags invite_flags);

signals:
    void configChanged();

private:
    /** Default constructor */
    ConfCertDialog(const RsPeerId &id,const RsPgpId& pgp_id, QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    /** Default destructor */
    ~ConfCertDialog();

    static ConfCertDialog *instance(const RsPeerId &peer_id);
    static ConfCertDialog *instance(const RsPgpId &pgp_id);

    void load();

private slots:
    void applyDialog();
    void loadInvitePage();

    void showHelpDialog();
    /** Called when a child window requests the given help <b>topic</b>. */
    void showHelpDialog(const QString &topic);

private:
    RsPeerId peerId;
    RsPgpId  pgpId;

    QString nameAndLocation;

	static QMap<RsPeerId, ConfCertDialog*> instances_ssl;
	static QMap<RsPgpId,  ConfCertDialog*> instances_pgp;

    /** Qt Designer generated object */
    Ui::ConfCertDialog ui;
};

#endif
