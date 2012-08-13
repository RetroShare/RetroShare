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

#include <QPainter>

#include "PreviewFeedDialog.h"
#include "ui_PreviewFeedDialog.h"
#include "FeedReaderNotify.h"
#include "FeedReaderStringDefs.h"
#include "util/HandleRichText.h"

#include "interface/rsFeedReader.h"
#include "retroshare/rsiface.h"
#include "util/HTMLWrapper.h"

#include <algorithm>

// not yet functional
//PreviewItemDelegate::PreviewItemDelegate(QTreeWidget *parent) : QItemDelegate(parent)
//{
//	connect(parent->header(), SIGNAL(sectionResized(int,int,int)), SLOT(sectionResized(int,int,int)));
//}

//void PreviewItemDelegate::sectionResized(int logicalIndex, int /*oldSize*/, int /*newSize*/)
//{
//	QHeaderView *header = dynamic_cast<QHeaderView*>(sender());
//	if (header) {
//		QTreeWidget *treeWidget = dynamic_cast<QTreeWidget*>(header->parent());
//		if (treeWidget) {
//			QModelIndex index = treeWidget->model()->index(0, logicalIndex, QModelIndex());
//			emit sizeHintChanged(index);
//		}
//	}
//}

//void PreviewItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
//{
//	QPen pen(painter->pen());
//	QFont font(painter->font());
//	QPalette::ColorGroup colorgroup(option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled);

//	QTextOption textOption;
//	textOption.setWrapMode(QTextOption::WrapAnywhere);
//	textOption.setAlignment(option.displayAlignment);

//	QRect textRect = rect.adjusted(5, 0, -5, 0); // remove width padding
//	textRect.setTop(qMin(rect.top(), option.rect.top()));
//	textRect.setHeight(qMax(rect.height(), option.rect.height()));

//	if (option.state & QStyle::State_Selected) {
//		painter->fillRect(rect, option.palette.brush(colorgroup, QPalette::Highlight));
//		painter->setPen(option.palette.color(colorgroup, QPalette::HighlightedText));
//	} else {
//		painter->setPen(option.palette.color(colorgroup, QPalette::Text));
//	}

//	if (option.state & QStyle::State_Editing) {
//		painter->save();
//		painter->setPen(option.palette.color(colorgroup, QPalette::Text));
//		painter->drawRect(rect.adjusted( 0, 0, -1, -1));
//		painter->restore();
//	}

//	painter->setFont(option.font);
//	painter->drawText(textRect, text, textOption);

//	// reset painter
//	painter->setFont(font);
//	painter->setPen(pen);
//}

//void PreviewItemDelegate::drawFocus(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect) const
//{
//	if (option.state & QStyle::State_HasFocus) {
//		QRect textRect(rect);
//		textRect.setTop(qMin(rect.top(), option.rect.top()));
//		textRect.setHeight(qMax(rect.height(), option.rect.height()));

//		QStyleOptionFocusRect optionFocusRect;
//		optionFocusRect.QStyleOption::operator=(option);
//		optionFocusRect.rect = textRect;
//		optionFocusRect.state |= QStyle::State_KeyboardFocusChange;
//		QPalette::ColorGroup colorgroup = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
//		optionFocusRect.backgroundColor = option.palette.color(colorgroup, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Background);
//		QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &optionFocusRect, painter);
//	}
//}

//QSize PreviewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
//{
////	static bool inSizeHint = false;

////	if (inSizeHint) {
////		return QSize();
////	}
////	inSizeHint = true;

//	//d->viewport->width()
//	QSize size = QItemDelegate::sizeHint(option, index);
//	size.setHeight(50);

////	QTreeWidget *treeWidget = dynamic_cast<QTreeWidget*>(parent());
////	if (treeWidget) {
////		size.setWidth(treeWidget->header()->sectionSize(index.column()));

////		QString text = index.data(Qt::DisplayRole).toString();
////		QRect displayRect = textRectangle(NULL, QRect(0, 0, size.width(), size.height()), option.font, text);
////		QRect displayRect = treeWidget->visualRect(index);
////		int width = treeWidget->columnWidth(index.column());
////		int height = option.fontMetrics.boundingRect(QRect(0, 0, size.width(), 0), Qt::TextWrapAnywhere | Qt::TextLongestVariant, text).height();

////		if (height > size.height()) {
////			size.setHeight(height);
////		}
////	}

////	inSizeHint = false;

//	return size;
//}

PreviewFeedDialog::PreviewFeedDialog(RsFeedReader *feedReader, FeedReaderNotify *notify, const FeedInfo &feedInfo, QWidget *parent) :
	QDialog(parent), mFeedReader(feedReader), mNotify(notify), ui(new Ui::PreviewFeedDialog)
{
	ui->setupUi(this);

	ui->feedNameLabel->clear();
	ui->feedInfoLabel->clear();

	/* connect signals */
	connect(ui->previousPushButton, SIGNAL(clicked()), this, SLOT(previousMsg()));
	connect(ui->nextPushButton, SIGNAL(clicked()), this, SLOT(nextMsg()));
	connect(ui->documentButton, SIGNAL(toggled(bool)), this, SLOT(showDocumentFrame(bool)));

	connect(mNotify, SIGNAL(notifyFeedChanged(QString,int)), this, SLOT(feedChanged(QString,int)));
	connect(mNotify, SIGNAL(notifyMsgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));

//	ui->documentTreeWidget->setItemDelegate(new PreviewItemDelegate(ui->documentTreeWidget));
	showDocumentFrame(false);

	if (!mFeedReader->addPreviewFeed(feedInfo, mFeedId)) {
		setInfo(tr("Cannot create preview"));
	}

	updateMsgCount();
}

PreviewFeedDialog::~PreviewFeedDialog()
{
	disconnect(mNotify);
	disconnect(mNotify);

	if (!mFeedId.empty()) {
		mFeedReader->removeFeed(mFeedId);
	}

	delete ui;
}

void PreviewFeedDialog::feedChanged(const QString &feedId, int type)
{
	if (feedId.isEmpty()) {
		return;
	}

	if (feedId.toStdString() != mFeedId) {
		return;
	}

	if (type == NOTIFY_TYPE_DEL) {
		/* feed deleted */
		mFeedId.clear();
		return;
	}

	if (type == NOTIFY_TYPE_ADD || type == NOTIFY_TYPE_MOD) {
		FeedInfo feedInfo;
		if (!mFeedReader->getFeedInfo(mFeedId, feedInfo)) {
			return;
		}
		fillFeedInfo(feedInfo);
	}
}

void PreviewFeedDialog::msgChanged(const QString &feedId, const QString &msgId, int type)
{
	if (feedId.isEmpty() || msgId.isEmpty()) {
		return;
	}

	if (feedId.toStdString() != mFeedId) {
		return;
	}

	switch (type) {
	case NOTIFY_TYPE_ADD:
		if (mMsgId.empty()) {
			mMsgId = msgId.toStdString();
			updateMsg();
		}
		break;
	case NOTIFY_TYPE_MOD:
		if (mMsgId == msgId.toStdString()) {
			updateMsg();
		}
		break;
	case NOTIFY_TYPE_DEL:
		if (mMsgId == msgId.toStdString()) {
			std::list<std::string>::iterator it = std::find(mMsgIds.begin(), mMsgIds.end(), mMsgId);
			if (it != mMsgIds.end()) {
				++it;
				if (it != mMsgIds.end()) {
					mMsgId = *it;
				} else {
					--it;
					if (it != mMsgIds.begin()) {
						--it;
						mMsgId = *it;
					} else {
						mMsgId.clear();
					}
				}
				updateMsg();
			}
		}
		break;
	}

	/* calculate message count */
	mMsgIds.clear();
	mFeedReader->getFeedMsgIdList(mFeedId, mMsgIds);

	updateMsgCount();
}

void PreviewFeedDialog::showDocumentFrame(bool show)
{
	ui->documentFrame->setVisible(show);
	ui->documentButton->setChecked(show);

	if (show) {
		ui->documentButton->setToolTip(tr("Hide tree"));
		ui->documentButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));

		fillDocumentTree();
	} else {
		ui->documentButton->setToolTip(tr("Show tree"));
		ui->documentButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
	}
}

int PreviewFeedDialog::getMsgPos()
{
	int pos = -1;
	std::list<std::string>::iterator it;
	for (it = mMsgIds.begin(); it != mMsgIds.end(); ++it) {
		++pos;
		if (*it == mMsgId) {
			break;
		}
	}

	return pos;
}

void PreviewFeedDialog::setInfo(const QString &info)
{
	ui->feedInfoLabel->setText(info);

	ui->infoLabel->setVisible(!info.isEmpty());
	ui->feedInfoLabel->setVisible(!info.isEmpty());
}

void PreviewFeedDialog::fillFeedInfo(const FeedInfo &feedInfo)
{
	QString name = feedInfo.name.empty() ? tr("No name") : QString::fromUtf8(feedInfo.name.c_str());

	QString workState = FeedReaderStringDefs::workState(feedInfo.workstate);
	if (!workState.isEmpty()) {
		name += QString(" (%1)").arg(workState);
	}
	ui->feedNameLabel->setText(name);

	setInfo(FeedReaderStringDefs::errorString(feedInfo));
}

void PreviewFeedDialog::previousMsg()
{
	std::list<std::string>::iterator it = std::find(mMsgIds.begin(), mMsgIds.end(), mMsgId);
	if (it != mMsgIds.end()) {
		if (it != mMsgIds.begin()) {
			--it;
			mMsgId = *it;
			updateMsg();
			updateMsgCount();
		}
	}
}

void PreviewFeedDialog::nextMsg()
{
	std::list<std::string>::iterator it = std::find(mMsgIds.begin(), mMsgIds.end(), mMsgId);
	if (it != mMsgIds.end()) {
		++it;
		if (it != mMsgIds.end()) {
			mMsgId = *it;
			updateMsg();
			updateMsgCount();
		}
	}
}

void PreviewFeedDialog::updateMsgCount()
{
	int pos = getMsgPos();
	ui->messageCountLabel->setText(QString("%1/%2").arg(pos + 1).arg(mMsgIds.size()));

	ui->previousPushButton->setEnabled(pos > 0);
	ui->nextPushButton->setEnabled(pos + 1 < (int) mMsgIds.size());
}

void PreviewFeedDialog::updateMsg()
{
	FeedMsgInfo msgInfo;
	if (mMsgId.empty() || !mFeedReader->getMsgInfo(mFeedId, mMsgId, msgInfo)) {
		ui->msgTitle->clear();
		ui->msgText->clear();
		mDescription.clear();
		return;
	}

	mDescription = msgInfo.description;
	QString msgTxt = RsHtml().formatText(ui->msgText->document(), QString::fromUtf8(mDescription.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);

	ui->msgText->setHtml(msgTxt);
	ui->msgTitle->setText(QString::fromUtf8(msgInfo.title.c_str()));

	ui->documentTreeWidget->clear();
	fillDocumentTree();
}

static void examineChildElements(QTreeWidget *treeWidget, HTMLWrapper &html, xmlNodePtr node, QTreeWidgetItem *parentItem)
{
	QTreeWidgetItem *item = new QTreeWidgetItem;
	QString text;
	if (node->type == XML_ELEMENT_NODE) {
		text = QString("<%1 ").arg(QString::fromUtf8(html.nodeName(node).c_str()));

		for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
			QString value = QString::fromUtf8(html.getAttr(node, attr).c_str());
			if (value.length() > 100) {
				value = value.left(100) + "...";
			}
			text += QString("%1=\"%2\" ").arg(QString::fromUtf8(html.attrName(attr).c_str()), value);
		}
		text = text.trimmed() + ">";
	} else {
		std::string content;
		if (html.getContent(node, content)) {
			text = QString::fromUtf8(content.c_str());
		} else {
			text = QApplication::translate("PreviewFeedDialog", "Error getting content");
		}
	}
	item->setText(0, text);
	parentItem->addChild(item);

//	QLabel *label = new QLabel(text);
//	label->setTextFormat(Qt::PlainText);
//	label->setWordWrap(true);
//	treeWidget->setItemWidget(item, 0, label);

	item->setExpanded(true);

	for (xmlNodePtr child = node->children; child; child = child->next) {
		examineChildElements(treeWidget, html, child, item);
	}
}

void PreviewFeedDialog::fillDocumentTree()
{
	if (!ui->documentTreeWidget->isVisible()) {
		return;
	}

	if (ui->documentTreeWidget->topLevelItemCount() > 0) {
		return;
	}

	if (mDescription.empty()) {
		return;
	}

	HTMLWrapper html;
	if (!html.readHTML(mDescription.c_str(), "")) {
		QTreeWidgetItem *item = new QTreeWidgetItem;
		item->setText(0, tr("Error parsing document"));
		ui->documentTreeWidget->addTopLevelItem(item);

		return;
	}

	xmlNodePtr root = html.getRootElement();
	if (!root) {
		return;
	}

	examineChildElements(ui->documentTreeWidget, html, root, ui->documentTreeWidget->invisibleRootItem());
}
