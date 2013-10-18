/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _CONFCERTDIALOG_H
#define _CONFCERTDIALOG_H

#include <QDialog>

#include "ui_ConfCertDialog.h"

class ConfCertDialog : public QDialog
{
    Q_OBJECT

public:
    enum enumPage { PageDetails, PageTrust, PageCertificate };

    static void showIt(const std::string& id, enumPage page);
    static void loadAll();

signals:
    void configChanged();

private:
    /** Default constructor */
    ConfCertDialog(const std::string& id, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    /** Default destructor */
    ~ConfCertDialog();

    static ConfCertDialog *instance(const std::string& peer_id);

    void load();

private slots:
    void applyDialog();
    void makeFriend();
    void denyFriend();
    void signGPGKey();
    void loadInvitePage();
    void setServiceFlags();

    void showHelpDialog();
    /** Called when a child window requests the given help <b>topic</b>. */
    void showHelpDialog(const QString &topic);

private:
    std::string mId;
    
    /** Qt Designer generated object */
    Ui::ConfCertDialog ui;
};

#endif
