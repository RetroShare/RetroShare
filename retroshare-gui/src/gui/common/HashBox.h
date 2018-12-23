/*******************************************************************************
 * gui/common/HashBox.h                                                        *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef HASHBOX_H
#define HASHBOX_H

#include <QScrollArea>
#include <stdint.h>
#include <retroshare/rsfiles.h>

namespace Ui {
	class HashBox;
}

class AttachFileItem;
class QVBoxLayout;

class HashedFile
{
public:
	enum Flags {
		NoFlag   = 0,
		Picture  = 1
	};

public:
	QString filename;
	QString filepath;
	uint64_t size;
    RsFileHash hash;
	Flags flag;

public:
	HashedFile();
};

class HashBox : public QScrollArea
{
	Q_OBJECT

public:
	explicit HashBox(QWidget *parent = 0);
	~HashBox();

	void setAutoHide(bool autoHide);
	void addAttachments(const QStringList& files,TransferRequestFlags tfl, HashedFile::Flags flag = HashedFile::NoFlag);

	void setDropWidget(QWidget* widget);
	void setDefaultTransferRequestFlags(TransferRequestFlags flags) { mDefaultTransferFlags = flags ; }

protected:
	bool eventFilter(QObject *object, QEvent *event);

private slots:
	void fileFinished(AttachFileItem* file);
	void checkAttachmentReady();

signals:
	void fileHashingStarted();
	void fileHashingFinished(QList<HashedFile> hashedFiles);

private:
	class HashingInfo
	{
	public:
		AttachFileItem* item;
		HashedFile::Flags flag;
	};

	QList<HashingInfo> mHashingInfos;
	bool mAutoHide;
	QWidget* dropWidget;
	Ui::HashBox *ui;
	TransferRequestFlags mDefaultTransferFlags ;
};

#endif // HASHBOX_H
