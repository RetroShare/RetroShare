/*******************************************************************************
 * plugins/FeedReader/gui/PreviewFeedDialog.cpp                                *
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

//#include <QPainter>
#include <QMenu>
#include <QKeyEvent>

#include "PreviewFeedDialog.h"
#include "ui_PreviewFeedDialog.h"
#include "FeedReaderNotify.h"
#include "FeedReaderStringDefs.h"
#include "util/HandleRichText.h"
#include "gui/settings/rsharesettings.h"

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
	QDialog(parent, Qt::Window), mFeedReader(feedReader), mNotify(notify), ui(new Ui::PreviewFeedDialog)
{
	ui->setupUi(this);

	ui->feedNameLabel->clear();

	/* connect signals */
	connect(ui->previousPushButton, SIGNAL(clicked()), this, SLOT(previousMsg()));
	connect(ui->nextPushButton, SIGNAL(clicked()), this, SLOT(nextMsg()));
	connect(ui->structureButton, SIGNAL(toggled(bool)), this, SLOT(showStructureFrame()));
	connect(ui->xpathUseListWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(xpathListCustomPopupMenu(QPoint)));
	connect(ui->xpathRemoveListWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(xpathListCustomPopupMenu(QPoint)));
	connect(ui->xpathUseListWidget->itemDelegate(), SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(xpathCloseEditor(QWidget*,QAbstractItemDelegate::EndEditHint)));
	connect(ui->xpathRemoveListWidget->itemDelegate(), SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(xpathCloseEditor(QWidget*,QAbstractItemDelegate::EndEditHint)));
	connect(ui->transformationTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(transformationTypeChanged()));

	connect(mNotify, SIGNAL(feedChanged(QString,int)), this, SLOT(feedChanged(QString,int)));
	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));

	ui->transformationTypeComboBox->addItem(FeedReaderStringDefs::transforationTypeString(RS_FEED_TRANSFORMATION_TYPE_NONE), RS_FEED_TRANSFORMATION_TYPE_NONE);
	ui->transformationTypeComboBox->addItem(FeedReaderStringDefs::transforationTypeString(RS_FEED_TRANSFORMATION_TYPE_XPATH), RS_FEED_TRANSFORMATION_TYPE_XPATH);
	ui->transformationTypeComboBox->addItem(FeedReaderStringDefs::transforationTypeString(RS_FEED_TRANSFORMATION_TYPE_XSLT), RS_FEED_TRANSFORMATION_TYPE_XSLT);

	ui->xsltTextEdit->setPlaceholderText(tr("XSLT is used on focus lost or when Ctrl+Enter is pressed"));

//	ui->documentTreeWidget->setItemDelegate(new PreviewItemDelegate(ui->documentTreeWidget));
	showStructureFrame();

	/* Set initial size the splitter */
//	QList<int> sizes;
//	sizes << 300 << 300 << 150; // Qt calculates the right sizes
//	ui->splitter->setSizes(sizes);

	if (mFeedReader->addPreviewFeed(feedInfo, mFeedId)) {
		setFeedInfo("");
	} else {
		setFeedInfo(tr("Cannot create preview"));
	}
	setTransformationInfo("");

	/* fill xpath/xslt expressions */
	ui->transformationTypeComboBox->setCurrentIndex(ui->transformationTypeComboBox->findData(feedInfo.transformationType));

	QListWidgetItem *item;
	std::string xpath;
	foreach(xpath, feedInfo.xpathsToUse){
		item = new QListWidgetItem(QString::fromUtf8(xpath.c_str()));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->xpathUseListWidget->addItem(item);
	}
	foreach(xpath, feedInfo.xpathsToRemove){
		item = new QListWidgetItem(QString::fromUtf8(xpath.c_str()));
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		ui->xpathRemoveListWidget->addItem(item);
	}

	ui->xsltTextEdit->setPlainText(QString::fromUtf8(feedInfo.xslt.c_str()));

	updateMsgCount();

	ui->xpathUseListWidget->installEventFilter(this);
	ui->xpathUseListWidget->viewport()->installEventFilter(this);
	ui->xpathRemoveListWidget->installEventFilter(this);
	ui->xpathRemoveListWidget->viewport()->installEventFilter(this);
	ui->xsltTextEdit->installEventFilter(this);

	/* load settings */
	processSettings(true);
}

PreviewFeedDialog::~PreviewFeedDialog()
{
	/* save settings */
	processSettings(false);

	disconnect(mNotify);
	disconnect(mNotify);

	if (!mFeedId.empty()) {
		mFeedReader->removeFeed(mFeedId);
	}

	delete ui;
}

void PreviewFeedDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("PreviewFeedDialog"));

	if (load) {
		// load settings
		QByteArray geometry = Settings->value("Geometry").toByteArray();
		if (!geometry.isEmpty()) {
			restoreGeometry(geometry);
		}
	} else {
		// save settings
		Settings->setValue("Geometry", saveGeometry());
	}

	Settings->endGroup();
}

bool PreviewFeedDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent) {
			if (keyEvent->key() == Qt::Key_Delete) {
				/* Delete pressed */
				if (obj == ui->xpathUseListWidget || obj == ui->xpathRemoveListWidget) {
					QListWidget *listWidget = dynamic_cast<QListWidget*>(obj);
					if (listWidget) {
						QListWidgetItem *item = listWidget->currentItem();
						if (item) {
							delete(item);
							processTransformation();
						}
						return true; // eat event
					}
				}
			}
			if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) && (keyEvent->modifiers() & Qt::ControlModifier)) {
				/* Ctrl+Enter pressed */
				if (obj == ui->xsltTextEdit) {
					processTransformation();
					return true; // eat event
				}
			}
		}
	}
	if (event->type() == QEvent::Drop) {
		processTransformation();
	}
	if (event->type() == QEvent::FocusOut) {
		if (obj == ui->xsltTextEdit) {
			processTransformation();
		}
	}
	/* pass the event on to the parent class */
	return QDialog::eventFilter(obj, event);
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

void PreviewFeedDialog::showStructureFrame()
{
	bool show = ui->structureButton->isChecked();
	RsFeedTransformationType transformationType = (RsFeedTransformationType) ui->transformationTypeComboBox->itemData(ui->transformationTypeComboBox->currentIndex()).toInt();

	ui->structureTreeFrame->setVisible(show);

	switch (transformationType) {
	case RS_FEED_TRANSFORMATION_TYPE_NONE:
		ui->msgTextOrg->hide();
		ui->structureTreeWidgetOrg->hide();
		ui->transformationFrame->hide();
		ui->xpathFrame->hide();
		ui->xsltFrame->hide();
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XPATH:
		ui->msgTextOrg->setVisible(show);
		ui->structureTreeWidgetOrg->show();
		ui->transformationFrame->setVisible(show);
		ui->xpathFrame->show();
		ui->xsltFrame->hide();
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XSLT:
		ui->msgTextOrg->setVisible(show);
		ui->structureTreeWidgetOrg->show();
		ui->transformationFrame->setVisible(show);
		ui->xpathFrame->hide();
		ui->xsltFrame->show();
		break;
	}

	if (ui->msgTextOrg->isVisible()) {
		QString msgTxt = RsHtml().formatText(ui->msgTextOrg->document(), QString::fromUtf8(mDescription.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);
		ui->msgTextOrg->setHtml(msgTxt);
	} else {
		ui->msgTextOrg->clear();
	}
	fillStructureTree(false);
	fillStructureTree(true);
}

void PreviewFeedDialog::transformationTypeChanged()
{
	showStructureFrame();
	processTransformation();
}

void PreviewFeedDialog::xpathListCustomPopupMenu(QPoint /*point*/)
{
	QListWidgetItem *item = NULL;

	if (sender() == ui->xpathUseListWidget) {
		item = ui->xpathUseListWidget->currentItem();
	} else if (sender() == ui->xpathRemoveListWidget) {
		item = ui->xpathRemoveListWidget->currentItem();
	} else {
		return;
	}

	QMenu contextMnu(this);

	QAction *action = contextMnu.addAction(QIcon(), tr("Add"), this, SLOT(addXPath()));
	action->setData(QVariant::fromValue(sender()));

	action = contextMnu.addAction(QIcon(), tr("Edit"), this, SLOT(editXPath()));
	action->setData(QVariant::fromValue(sender()));
	if (!item) {
		action->setEnabled(false);
	}

	action = contextMnu.addAction(QIcon(), tr("Delete"), this, SLOT(removeXPath()));
	action->setData(QVariant::fromValue(sender()));
	if (!item) {
		action->setEnabled(false);
	}

	contextMnu.exec(QCursor::pos());
}


void PreviewFeedDialog::xpathCloseEditor(QWidget */*editor*/, QAbstractItemDelegate::EndEditHint /*hint*/)
{
	processTransformation();
}

void PreviewFeedDialog::addXPath()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	QObject *source = action->data().value<QObject*>();

	QListWidget *listWidget;
	if (source == ui->xpathUseListWidget) {
		listWidget = ui->xpathUseListWidget;
	} else if (source == ui->xpathRemoveListWidget) {
		listWidget = ui->xpathRemoveListWidget;
	} else {
		return;
	}

	QListWidgetItem *item = new QListWidgetItem();
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	listWidget->addItem(item);

	listWidget->editItem(item);
}

void PreviewFeedDialog::editXPath()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	QObject *source = action->data().value<QObject*>();

	QListWidget *listWidget;
	if (source == ui->xpathUseListWidget) {
		listWidget = ui->xpathUseListWidget;
	} else if (source == ui->xpathRemoveListWidget) {
		listWidget = ui->xpathRemoveListWidget;
	} else {
		return;
	}

	listWidget->editItem(listWidget->currentItem());
}

void PreviewFeedDialog::removeXPath()
{
	QAction *action = dynamic_cast<QAction*>(sender());
	if (!action) {
		return;
	}

	QObject *source = action->data().value<QObject*>();

	QListWidget *listWidget;
	if (source == ui->xpathUseListWidget) {
		listWidget = ui->xpathUseListWidget;
	} else if (source == ui->xpathRemoveListWidget) {
		listWidget = ui->xpathRemoveListWidget;
	} else {
		return;
	}

	QListWidgetItem *item = listWidget->currentItem();
	if (item) {
		delete(item);
	}

	processTransformation();
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

void PreviewFeedDialog::setFeedInfo(const QString &info)
{
	ui->feedInfoLabel->setText(info);
	ui->feedInfoLabel->setVisible(!info.isEmpty());
}

void PreviewFeedDialog::setTransformationInfo(const QString &info)
{
	ui->transformationInfoLabel->setText(info);
	ui->transformationInfoLabel->setVisible(!info.isEmpty());
}

void PreviewFeedDialog::fillFeedInfo(const FeedInfo &feedInfo)
{
	QString name = feedInfo.name.empty() ? tr("No name") : QString::fromUtf8(feedInfo.name.c_str());

	QString workState = FeedReaderStringDefs::workState(feedInfo.workstate);
	if (!workState.isEmpty()) {
		name += QString(" (%1)").arg(workState);
	}
	ui->feedNameLabel->setText(name);

	setFeedInfo(FeedReaderStringDefs::errorString(feedInfo));
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
		ui->msgTextOrg->clear();
		mDescription.clear();
		mDescriptionTransformed.clear();
		return;
	}

	ui->msgTitle->setText(QString::fromUtf8(msgInfo.title.c_str()));

	/* store description */
	mDescription = msgInfo.description;

	if (ui->msgTextOrg->isVisible()) {
		QString msgTxt = RsHtml().formatText(ui->msgTextOrg->document(), QString::fromUtf8(mDescription.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);
		ui->msgTextOrg->setHtml(msgTxt);
	}

	showStructureFrame();

	/* process xpath */
	processTransformation();
}

static void buildNodeText(HTMLWrapper &html, xmlNodePtr node, QString &text)
{
	switch (node->type) {
	case XML_ELEMENT_NODE:
		text = QString("<%1 ").arg(QString::fromUtf8(html.nodeName(node).c_str()));

		for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
			QString value = QString::fromUtf8(html.getAttr(node, attr).c_str());
			if (value.length() > 100) {
				value = value.left(100) + "...";
			}
			text += QString("%1=\"%2\" ").arg(QString::fromUtf8(html.attrName(attr).c_str()), value);
		}
		text = text.trimmed() + ">";

		if (node->children && !node->children->next && node->children->type == XML_TEXT_NODE) {
			/* only one text node as child */
			std::string content;
			if (html.getContent(node->children, content, false)) {
				text += QString::fromUtf8(content.c_str());
			} else {
				text += QApplication::translate("PreviewFeedDialog", "Error getting content");
			}
			text += QString("<%1>").arg(QString::fromUtf8(html.nodeName(node).c_str()));

			xmlUnlinkNode(node->children);
			xmlFreeNode(node->children);
		}
		break;
	case XML_TEXT_NODE:
	case XML_COMMENT_NODE:
		{
			if (node->type == XML_COMMENT_NODE) {
				text = "<!-- ";
			}

			std::string content;
			if (html.getContent(node, content, false)) {
				text += QString::fromUtf8(content.c_str());
			} else {
				text += QApplication::translate("PreviewFeedDialog", "Error getting content");
			}

			if (node->type == XML_COMMENT_NODE) {
				text += " -->";
			}
		}
		break;
	case XML_ATTRIBUTE_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_DOCUMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_DTD_NODE:
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
#ifdef LIBXML_DOCB_ENABLED
	case XML_DOCB_DOCUMENT_NODE:
#endif
		break;
	}
}

static void examineChildElements(QTreeWidget *treeWidget, HTMLWrapper &html, QList<xmlNodePtr> &nodes, QTreeWidgetItem *parentItem)
{
	int childIndex = 0;
	int childCount;

	QList<QPair<xmlNodePtr, QTreeWidgetItem*> > nodeItems;
	foreach (xmlNodePtr node, nodes) {
		QString text;
		buildNodeText(html, node, text);

		QList<QTreeWidgetItem*> itemsToDelete;
		QTreeWidgetItem *item = NULL;

		childCount = parentItem->childCount();
		for (int index = childIndex; index < childCount; ++index) {
			QTreeWidgetItem *childItem = parentItem->child(index);
			if (childItem->text(0) == text) {
				/* reuse item */
				item = childItem;
				break;
			}
			itemsToDelete.push_back(childItem);
		}

		if (item) {
			/* delete old items */
			foreach (QTreeWidgetItem *item, itemsToDelete) {
				delete(item);
			}
			++childIndex;
		} else {
			item = new QTreeWidgetItem;
			item->setText(0, text);
			parentItem->insertChild(childIndex, item);
			item->setExpanded(true);

			++childIndex;
		}

		nodeItems.push_back(QPair<xmlNodePtr, QTreeWidgetItem*>(node, item));
	}

	/* delete not used items */
	while (childIndex < parentItem->childCount()) {
		delete(parentItem->child(childIndex));
	}

	QList<QPair<xmlNodePtr, QTreeWidgetItem*> >::iterator nodeItem;
	for (nodeItem = nodeItems.begin(); nodeItem != nodeItems.end(); ++nodeItem) {
		QList<xmlNodePtr> childNodes;
		for (xmlNodePtr childNode = nodeItem->first->children; childNode; childNode = childNode->next) {
			childNodes.push_back(childNode);
		}
		examineChildElements(treeWidget, html, childNodes, nodeItem->second);
	}

//	QLabel *label = new QLabel(text);
//	label->setTextFormat(Qt::PlainText);
//	label->setWordWrap(true);
//	treeWidget->setItemWidget(item, 0, label);
}

void PreviewFeedDialog::fillStructureTree(bool transform)
{
	if (transform && ui->structureTreeWidget->isVisible()) {
		if (mDescriptionTransformed.empty()) {
			ui->structureTreeWidget->clear();
		} else {
			HTMLWrapper html;
			if (html.readHTML(mDescriptionTransformed.c_str(), "")) {
				xmlNodePtr root = html.getRootElement();
				if (root) {
					QList<xmlNodePtr> nodes;
					nodes.push_back(root);
					examineChildElements(ui->structureTreeWidget, html, nodes, ui->structureTreeWidget->invisibleRootItem());
					ui->structureTreeWidget->resizeColumnToContents(0);
				}
			} else {
				QTreeWidgetItem *item = new QTreeWidgetItem;
				item->setText(0, tr("Error parsing document") + ": " + QString::fromUtf8(html.lastError().c_str()));
				ui->structureTreeWidget->addTopLevelItem(item);
			}
		}
	}

	if (!transform && ui->structureTreeWidgetOrg->isVisible()) {
		if (mDescription.empty()) {
			ui->structureTreeWidgetOrg->clear();
		} else {
			HTMLWrapper html;
			if (html.readHTML(mDescription.c_str(), "")) {
				xmlNodePtr root = html.getRootElement();
				if (root) {
					QList<xmlNodePtr> nodes;
					nodes.push_back(root);
					examineChildElements(ui->structureTreeWidgetOrg, html, nodes, ui->structureTreeWidgetOrg->invisibleRootItem());
					ui->structureTreeWidgetOrg->resizeColumnToContents(0);
				}
			} else {
				QTreeWidgetItem *item = new QTreeWidgetItem;
				item->setText(0, tr("Error parsing document") + ": " + QString::fromUtf8(html.lastError().c_str()));
				ui->structureTreeWidgetOrg->addTopLevelItem(item);
			}
		}
	}
}

RsFeedTransformationType PreviewFeedDialog::getData(std::list<std::string> &xpathsToUse, std::list<std::string> &xpathsToRemove, std::string &xslt)
{
	xpathsToUse.clear();
	xpathsToRemove.clear();

	int row;
	int rowCount = ui->xpathUseListWidget->count();
	for (row = 0; row < rowCount; ++row) {
		xpathsToUse.push_back(ui->xpathUseListWidget->item(row)->text().toUtf8().constData());
	}

	rowCount = ui->xpathRemoveListWidget->count();
	for (row = 0; row < rowCount; ++row) {
		xpathsToRemove.push_back(ui->xpathRemoveListWidget->item(row)->text().toUtf8().constData());
	}

	xslt = ui->xsltTextEdit->toPlainText().toUtf8().constData();

	return (RsFeedTransformationType) ui->transformationTypeComboBox->itemData(ui->transformationTypeComboBox->currentIndex()).toInt();
}

void PreviewFeedDialog::processTransformation()
{
	std::list<std::string> xpathsToUse;
	std::list<std::string> xpathsToRemove;
	std::string xslt;

	RsFeedTransformationType transformationType = getData(xpathsToUse, xpathsToRemove, xslt);

	mDescriptionTransformed = mDescription;
	std::string errorString;

	RsFeedReaderErrorState result = RS_FEED_ERRORSTATE_OK;
	switch (transformationType) {
	case RS_FEED_TRANSFORMATION_TYPE_NONE:
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XPATH:
		result = mFeedReader->processXPath(xpathsToUse, xpathsToRemove, mDescriptionTransformed, errorString);
		break;
	case RS_FEED_TRANSFORMATION_TYPE_XSLT:
		result = mFeedReader->processXslt(xslt, mDescriptionTransformed, errorString);
		break;
	}
	setTransformationInfo(FeedReaderStringDefs::errorString(result, errorString));

	/* fill message */
	QString msgTxt = RsHtml().formatText(ui->msgText->document(), QString::fromUtf8(mDescriptionTransformed.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS);
	ui->msgText->setHtml(msgTxt);

	/* fill structure */
	fillStructureTree(true);
}
