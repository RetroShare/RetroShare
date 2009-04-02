/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#ifndef FORM_FILESEND_H
#define FORM_FILESEND_H

#include <QtGui>

#include "ui_form_fileSend.h"
#include "src/FileTransferSend.h"
class form_fileSend : public QDialog, public Ui::form_FileSend
{
	Q_OBJECT
	public:
	form_fileSend(cFileTransferSend* FileTransfer);

	private slots:
	void slot_allreadySendedSizeChanged(quint64 value);
	void slot_FileTransferFinishedOK();
	void slot_FileTransferError();
	void slot_FileTransferAccepted(bool t);
	void slot_FileTransferAborted();
	void slot_Button();

	private:
	void init();
	cFileTransferSend* FileTransfer;
};
#endif