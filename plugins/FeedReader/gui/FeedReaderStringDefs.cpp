/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
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

#include <QApplication>
#include <QMessageBox>

#include "FeedReaderStringDefs.h"

bool FeedReaderStringDefs::showError(QWidget *parent, RsFeedAddResult result, const QString &title, const QString &text)
{
	QString error;

	switch (result) {
	case RS_FEED_ADD_RESULT_SUCCESS:
		/* no error */
		return false;
	case RS_FEED_ADD_RESULT_FEED_NOT_FOUND:
		error = QApplication::translate("FeedReaderStringDefs", "Feed not found.");
		break;
	case RS_FEED_ADD_RESULT_PARENT_NOT_FOUND:
		error = QApplication::translate("FeedReaderStringDefs", "Parent not found.");
		break;
	case RS_FEED_ADD_RESULT_PARENT_IS_NO_FOLDER:
		error = QApplication::translate("FeedReaderStringDefs", "Parent is no folder.");
		break;
	case RS_FEED_ADD_RESULT_FEED_IS_FOLDER:
		error = QApplication::translate("FeedReaderStringDefs", "Feed is a folder.");
		break;
	case RS_FEED_ADD_RESULT_FEED_IS_NO_FOLDER:
		error = QApplication::translate("FeedReaderStringDefs", "Feed is no folder.");
		break;
	default:
		error = QApplication::translate("FeedReaderStringDefs", "Unknown error occured.");
	}

	QMessageBox::critical(parent, title, text + "\n" + error);

	return true;
}

QString FeedReaderStringDefs::workState(FeedInfo::WorkState state)
{
	switch (state) {
	case FeedInfo::WAITING:
		return "";
	case FeedInfo::WAITING_TO_DOWNLOAD:
		return QApplication::translate("FeedReaderStringDefs", "Waiting for download");
	case FeedInfo::DOWNLOADING:
		return QApplication::translate("FeedReaderStringDefs", "Downloading");
	case FeedInfo::WAITING_TO_PROCESS:
		return QApplication::translate("FeedReaderStringDefs", "Waiting for process");
	case FeedInfo::PROCESSING:
		return QApplication::translate("FeedReaderStringDefs", "Processing");
	}

	return "";
}

QString FeedReaderStringDefs::errorString(const FeedInfo &feedInfo)
{
	QString errorState;
	switch (feedInfo.errorState) {
	case RS_FEED_ERRORSTATE_OK:
		break;

	/* download */
	case RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR:
		errorState = QApplication::translate("FeedReaderStringDefs", "Internal download error");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_ERROR:
		errorState = QApplication::translate("FeedReaderStringDefs", "Download error");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE:
		errorState = QApplication::translate("FeedReaderStringDefs", "Unknown content type");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND:
		errorState = QApplication::translate("FeedReaderStringDefs", "Download not found");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE:
		errorState = QApplication::translate("FeedReaderStringDefs", "Unknown response code");
		break;

	/* process */
	case RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR:
		errorState = QApplication::translate("FeedReaderStringDefs", "Internal process error");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT:
		errorState = QApplication::translate("FeedReaderStringDefs", "Unknown XML format");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_CREATE:
		errorState = QApplication::translate("FeedReaderStringDefs", "Can't create forum");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_FOUND:
		errorState = QApplication::translate("FeedReaderStringDefs", "Forum not found");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_ADMIN:
		errorState = QApplication::translate("FeedReaderStringDefs", "You are not admin of the forum");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_ANONYMOUS:
		errorState = QApplication::translate("FeedReaderStringDefs", "The forum is no anonymous forum");
		break;

	default:
		errorState = QApplication::translate("FeedReaderStringDefs", "Unknown error");
	}

	if (!feedInfo.errorString.empty()) {
		errorState += QString(" (%1)").arg(QString::fromUtf8(feedInfo.errorString.c_str()));
	}

	return errorState;
}
