/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCreatePostDialog.h                      *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef POSTEDCREATEPOSTDIALOG_H
#define POSTEDCREATEPOSTDIALOG_H

#include <QDialog>
#include <gui/common/HashBox.h>
#include "retroshare/rsposted.h"
#include "util/RichTextEdit.h"

class TokenQueue;

namespace Ui {
	class PostedCreatePostDialog;
}

class PostedCreatePostDialog : public QDialog
{
    Q_OBJECT

public:
	/*!
	 * @param tokenQ parent callee token
	 * @param posted
	 */
	explicit PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted* posted, const RsGxsGroupId& grpId, QWidget *parent = 0);
	~PostedCreatePostDialog();

private:
	QString imagefilename;
	QByteArray imagebytes;
	const int MAXMESSAGESIZE = 199000;

private slots:
	void createPost();
	void addPicture();
	void on_postButton_clicked();
	void on_imageButton_clicked();
	void on_linkButton_clicked();
	void on_removeButton_clicked();
	void fileHashingFinished(QList<HashedFile> hashedFiles);

private:
	void processSettings(bool load);

	QString mLink;
	QString mNotes;
	TokenQueue* mTokenQueue;
	RsPosted* mPosted;
	RsGxsGroupId mGrpId;

	Ui::PostedCreatePostDialog *ui;
};

#endif // POSTEDCREATEPOSTDIALOG_H
