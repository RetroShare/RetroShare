/*
 * Avatar Dialog.
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

#ifndef _AVATARDIALOG_H
#define _AVATARDIALOG_H

#include "ui_AvatarDialog.h"

#include <inttypes.h>

#include "util/TokenQueue.h"
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsifacetypes.h>
#include <retroshare/rsgxscommon.h>
#include <QString>

namespace Ui {
class AvatarDialog;
}


class AvatarDialog : public QDialog
{
	Q_OBJECT

public:
	AvatarDialog(QWidget *parent = 0);
	~AvatarDialog();
	
    void setAvatar(const RsGxsImage &avatar);
    void getAvatar(RsGxsImage &avatar);

private slots:

    void changeAvatar();
    void removeAvatar();
    void saveAvatar();
    void loadOwnAvatar();

private:
	Ui::AvatarDialog ui;

};

#endif
