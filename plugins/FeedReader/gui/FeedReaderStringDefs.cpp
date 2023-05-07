/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderStringDefs.cpp                             *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
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

#include <QApplication>
#include <QMessageBox>

#include "FeedReaderStringDefs.h"

bool FeedReaderStringDefs::showError(QWidget *parent, RsFeedResult result, const QString &title, const QString &text)
{
	QString error;

	switch (result) {
	case RS_FEED_RESULT_SUCCESS:
		/* no error */
		return false;
	case RS_FEED_RESULT_FEED_NOT_FOUND:
		error = QApplication::translate("FeedReaderStringDefs", "Feed not found.");
		break;
	case RS_FEED_RESULT_PARENT_NOT_FOUND:
		error = QApplication::translate("FeedReaderStringDefs", "Parent not found.");
		break;
	case RS_FEED_RESULT_PARENT_IS_NO_FOLDER:
		error = QApplication::translate("FeedReaderStringDefs", "Parent is no folder.");
		break;
	case RS_FEED_RESULT_FEED_IS_FOLDER:
		error = QApplication::translate("FeedReaderStringDefs", "Feed is a folder.");
		break;
	case RS_FEED_RESULT_FEED_IS_NO_FOLDER:
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

	return QApplication::translate("FeedReaderStringDefs", "Unknown");
}

QString FeedReaderStringDefs::errorString(const FeedInfo &feedInfo)
{
	return errorString(feedInfo.errorState, feedInfo.errorString);
}

QString FeedReaderStringDefs::errorString(RsFeedReaderErrorState errorState, const std::string &errorString)
{
	QString errorText;
	switch (errorState) {
	case RS_FEED_ERRORSTATE_OK:
		break;

	/* download */
	case RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Internal download error");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Download error");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE:
		errorText = QApplication::translate("FeedReaderStringDefs", "Unknown content type");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND:
		errorText = QApplication::translate("FeedReaderStringDefs", "Download not found");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE:
		errorText = QApplication::translate("FeedReaderStringDefs", "Unknown response code");
		break;
	case RS_FEED_ERRORSTATE_DOWNLOAD_BLOCKED:
		errorText = QApplication::translate("FeedReaderStringDefs", "Download blocked");
		break;

	/* process */
	case RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Internal process error");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT:
		errorText = QApplication::translate("FeedReaderStringDefs", "Unknown XML format");
		break;
//	case RS_FEED_ERRORSTATE_PROCESS_FORUM_CREATE:
//		errorText = QApplication::translate("FeedReaderStringDefs", "Can't create forum");
//		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_FOUND:
		errorText = QApplication::translate("FeedReaderStringDefs", "Forum not found");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_ADMIN:
		errorText = QApplication::translate("FeedReaderStringDefs", "You are not admin of the forum");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_AUTHOR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Forum has no author");
		break;
//	case RS_FEED_ERRORSTATE_PROCESS_POSTED_CREATE:
//		errorText = QApplication::translate("FeedReaderStringDefs", "Can't create board");
//		break;
	case RS_FEED_ERRORSTATE_PROCESS_POSTED_NOT_FOUND:
		errorText = QApplication::translate("FeedReaderStringDefs", "Board not found");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_ADMIN:
		errorText = QApplication::translate("FeedReaderStringDefs", "You are not admin of the board");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_AUTHOR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Board has no author");
		break;

	case RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Can't read html");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XPATH_INTERNAL_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "Internal XPath error");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XPATH_WRONG_EXPRESSION:
		errorText = QApplication::translate("FeedReaderStringDefs", "Wrong XPath expression");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XPATH_NO_RESULT:
		errorText = QApplication::translate("FeedReaderStringDefs", "Empty XPath result");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XSLT_FORMAT_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "XSLT format error");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XSLT_TRANSFORM_ERROR:
		errorText = QApplication::translate("FeedReaderStringDefs", "XSLT transformation error");
		break;
	case RS_FEED_ERRORSTATE_PROCESS_XSLT_NO_RESULT:
		errorText = QApplication::translate("FeedReaderStringDefs", "Empty XSLT result");
		break;

	default:
		errorText = QApplication::translate("FeedReaderStringDefs", "Unknown error");
	}

	if (!errorString.empty()) {
		errorText += QString(" (%1)").arg(QString::fromUtf8(errorString.c_str()));
	}

	return errorText;
}

QString FeedReaderStringDefs::transforationTypeString(RsFeedTransformationType transformationType)
{
	switch (transformationType) {
	case RS_FEED_TRANSFORMATION_TYPE_NONE:
		return QApplication::translate("FeedReaderStringDefs", "No transformation");
	case RS_FEED_TRANSFORMATION_TYPE_XPATH:
		return QApplication::translate("FeedReaderStringDefs", "XPath");
	case RS_FEED_TRANSFORMATION_TYPE_XSLT:
		return QApplication::translate("FeedReaderStringDefs", "XSLT");
	}

	return QApplication::translate("FeedReaderStringDefs", "Unknown");
}
