/*******************************************************************************
 * gui/common/HashBox.cpp                                                      *
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

#include <QMessageBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>
#include <QMimeData>

#include <iostream>

#include "gui/feeds/AttachFileItem.h"

#include "HashBox.h"
#include "ui_HashBox.h"

HashedFile::HashedFile()
{
	size = 0;
	flag = NoFlag;
}

HashBox::HashBox(QWidget *parent) :
	QScrollArea(parent),
	ui(new Ui::HashBox)
{
	dropWidget = NULL;
	mAutoHide = false;
	mDefaultTransferFlags = TransferRequestFlags(0u) ;

	ui->setupUi(this);
}

HashBox::~HashBox()
{
	delete ui;
}

void HashBox::setAutoHide(bool autoHide)
{
	mAutoHide = autoHide;
	setVisible(!mAutoHide);
}

void HashBox::setDropWidget(QWidget* widget)
{
	if (dropWidget) {
		dropWidget->removeEventFilter(this);
	}

	dropWidget = widget;

	if (dropWidget) {
		widget->installEventFilter(this);
	}
}

static void showFormats(const std::string& event, const QStringList& formats)
{
	std::cerr << event << "() Formats" << std::endl;
	QStringList::const_iterator it;
	for (it = formats.begin(); it != formats.end(); ++it) {
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}
}

bool HashBox::eventFilter(QObject* object, QEvent* event)
{
	if (object == dropWidget) {
		if (event->type() == QEvent::DragEnter) {
			QDragEnterEvent* dragEnterEvent = static_cast<QDragEnterEvent*>(event);
			if (dragEnterEvent) {
				/* print out mimeType */
				showFormats("HashBox::dragEnterEvent", dragEnterEvent->mimeData()->formats());

				if (dragEnterEvent->mimeData()->hasUrls()) {
					std::cerr << "HashBox::dragEnterEvent() Accepting Urls" << std::endl;
					dragEnterEvent->acceptProposedAction();
				} else {
					std::cerr << "HashBox::dragEnterEvent() No Urls" << std::endl;
				}
			}
		} else if (event->type() == QEvent::Drop) {
			QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
			if (dropEvent) {
				if (Qt::CopyAction & dropEvent->possibleActions()) {
					/* print out mimeType */
					showFormats("HashBox::dropEvent", dropEvent->mimeData()->formats());

					QStringList files;

					if (dropEvent->mimeData()->hasUrls()) {
						std::cerr << "HashBox::dropEvent() Urls:" << std::endl;

						QList<QUrl> urls = dropEvent->mimeData()->urls();
						QList<QUrl>::iterator uit;
						for (uit = urls.begin(); uit != urls.end(); ++uit) {
							QString localpath = uit->toLocalFile();
							std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
							std::cerr << "or As Local File: " << localpath.toStdString() << std::endl;

							if (localpath.isEmpty() == false) {
								//Check that the file does exist and is not a directory
								QDir dir(localpath);
								if (dir.exists()) {
									std::cerr << "HashBox::dropEvent() directory not accepted." << std::endl;
									QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."), QMessageBox::Information, QMessageBox::Ok, 0, 0, this);
									mb.exec();
								} else if (QFile::exists(localpath)) {
									files.push_back(localpath);
								} else {
									std::cerr << "HashBox::dropEvent() file does not exists."<< std::endl;
									QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."), QMessageBox::Information, QMessageBox::Ok, 0, 0, this);
									mb.exec();
								}
							}
						}
					}

					addAttachments(files,mDefaultTransferFlags);

					dropEvent->setDropAction(Qt::CopyAction);
					dropEvent->accept();
				} else {
					std::cerr << "HashBox::dropEvent() Rejecting uncopyable DropAction" << std::endl;
				}
			}
		}
	}
	// pass the event on to the parent class
	return QScrollArea::eventFilter(object, event);
}

void HashBox::addAttachments(const QStringList& files,TransferRequestFlags tfl, HashedFile::Flags flag)
{
	/* add a AttachFileItem to the attachment section */
	std::cerr << "HashBox::addExtraFile() hashing file." << std::endl;

	if (files.isEmpty()) {
		return;
	}

	if (mAutoHide) {
		show();
	}

	QStringList::ConstIterator it;
	for (it = files.constBegin(); it != files.constEnd(); ++it) {
		/* add widget in for new destination */
		AttachFileItem* file = new AttachFileItem(*it,tfl);
		QObject::connect(file, SIGNAL(fileFinished(AttachFileItem*)), this, SLOT(fileFinished(AttachFileItem*)));

		HashingInfo hashingInfo;
		hashingInfo.item = file;
		hashingInfo.flag = flag;
		mHashingInfos.push_back(hashingInfo);
		ui->verticalLayout->addWidget(file, 1, 0);
	}
	QApplication::processEvents();

	// workaround for Qt bug, the size from the first call to QScrollArea::sizeHint() is stored in QWidgetItemV2 and
	// QScrollArea::sizeHint() is never called again so that widgetResizable of QScrollArea doesn't work
	// the next line clears the member QScrollArea::widgetSize for recalculation of the added children in QScrollArea::sizeHint()
	setWidget(takeWidget());
	// the next line set the cache to dirty
	updateGeometry();

	emit fileHashingStarted();

	checkAttachmentReady();
}

void HashBox::fileFinished(AttachFileItem* file)
{
	std::cerr << "HashBox::fileHashingFinished() started." << std::endl;

	//check that the file is ok
	if (file->getState() == AFI_STATE_ERROR) {
		std::cerr << "HashBox::fileHashingFinished error file is not hashed.";
		return;
	}

	checkAttachmentReady();
}

void HashBox::checkAttachmentReady()
{
	if (mHashingInfos.isEmpty()) {
		return;
	}

	QList<HashingInfo>::iterator it;
	for (it = mHashingInfos.begin(); it != mHashingInfos.end(); ++it) {
		if (it->item->isHidden()) {
			continue;
		}
		if (!it->item->done()) {
			break;
		}
	}

	if (it != mHashingInfos.end()) {
		/* repeat... */
		QTimer::singleShot(500, this, SLOT(checkAttachmentReady()));
		return;
	}

	if (mAutoHide) {
		hide();
	}

	QList<HashedFile> hashedFiles;
	for (it = mHashingInfos.begin(); it != mHashingInfos.end(); ++it) {
		HashingInfo& hashingInfo = *it;
		if (hashingInfo.item->done()) {
			HashedFile hashedFile;
			hashedFile.filename = hashingInfo.item->FileName();
			hashedFile.filepath = hashingInfo.item->FilePath();
			hashedFile.hash = hashingInfo.item->FileHash();
			hashedFile.size = hashingInfo.item->FileSize();
			hashedFile.flag = hashingInfo.flag;
			hashedFiles.push_back(hashedFile);

			ui->verticalLayout->removeWidget(hashingInfo.item);
			hashingInfo.item->deleteLater();
		}
	}
	mHashingInfos.clear();

	QApplication::processEvents();

	// workaround for Qt bug, the size from the first call to QScrollArea::sizeHint() is stored in QWidgetItemV2 and
	// QScrollArea::sizeHint() is never called again so that widgetResizable of QScrollArea doesn't work
	// the next line clears the member QScrollArea::widgetSize for recalculation of the removed children in QScrollArea::sizeHint()
	setWidget(takeWidget());
	// the next line set the cache to dirty
	updateGeometry();

	emit fileHashingFinished(hashedFiles);

	auto ev = std::make_shared<RsSharedDirectoriesEvent>();
    ev->mEventCode = RsSharedDirectoriesEventCode::HASHING_PROCESS_FINISHED;
	if(rsEvents)
		rsEvents->postEvent(ev);
}
