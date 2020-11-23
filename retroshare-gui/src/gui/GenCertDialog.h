/*******************************************************************************
 * gui/GenCertDialog.h                                                         *
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

#ifndef _GENCERTDIALOG_H
#define _GENCERTDIALOG_H

#include "ui_GenCertDialog.h"

class QMouseEvent ;

class GenCertDialog : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
	GenCertDialog(bool onlyGenerateIdentity, QWidget *parent = 0);

	virtual ~GenCertDialog() ;
	virtual void mouseMoveEvent(QMouseEvent *e) ;
	QString getGXSNickname() {return mGXSNickname;}
private slots:
	void genPerson();
	bool importIdentity();
	void exportIdentity();
	void setupState();
    void switchReuseExistingNode();
	void grabMouse();
	void updateCheckLabels();
	void useBobChecked(bool checked);

private:
	void initKeyList();

	/** Qt Designer generated object */
	Ui::GenCertDialog ui;

	bool genNewGPGKey;
	bool haveGPGKeys;
	bool mOnlyGenerateIdentity;
    bool mAllFieldsOk ;
    bool mEntropyOk ;
	QString mGXSNickname;

	QTimer *entropy_timer ;
};

#endif
