/*******************************************************************************
 * retroshare-gui/src/gui/gxs/CommentItemWidget.cpp                            *
 *                                                                             *
 * Copyright 2026 by RetroShare Team   <retroshare.project@gmail.com>          *
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

#include "CommentItemWidget.h"
#include "CommentText.h"
#include "ui_CommentItemWidget.h"
#include "GxsIdDetails.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QDebug>
#include <QMouseEvent>
#include <QAbstractTextDocumentLayout>
#include <QtMath>

CommentItemWidget::CommentItemWidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::CommentItemWidget), mViewRepliesButton(nullptr),
	  mLevel(0), mUpvoteActive(false), mDownvoteActive(false), mSelected(false), mRepliesExpanded(false), mReplyCount(0)
{
	ui->setupUi(this);
	setupStyle();
	mFullSizeButton = new QPushButton(tr("View full size"), this);
	mFullSizeButton->setFlat(false);
	mFullSizeButton->setCursor(Qt::PointingHandCursor);
	mFullSizeButton->hide();
	ui->contentLayout->insertWidget(2, mFullSizeButton, 0, Qt::AlignLeft);
	connect(mFullSizeButton, &QPushButton::clicked, this, [this]() {
		CommentText::showFullSize(this, mCommentText);
	});
	ui->commentTextBrowser->setFrameShape(QFrame::NoFrame);
	ui->commentTextBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->commentTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->commentTextBrowser->setStyleSheet("background: transparent;");
	ui->commentTextBrowser->document()->setDocumentMargin(0);
	ui->commentTextBrowser->viewport()->installEventFilter(this);
	connect(ui->commentTextBrowser->document()->documentLayout(),
	        &QAbstractTextDocumentLayout::documentSizeChanged, this,
	        [this](const QSizeF &size) {
		ui->commentTextBrowser->setFixedHeight(qMax(1, qCeil(size.height())));
	});

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	mViewRepliesButton = new QPushButton(this);
	mViewRepliesButton->setFlat(true);
	mViewRepliesButton->setCursor(Qt::PointingHandCursor);
	mViewRepliesButton->hide();
	connect(mViewRepliesButton, &QPushButton::clicked, this, &CommentItemWidget::on_viewRepliesButton_clicked);

	// Insert the view-replies button into the content layout, below the actions row
	QVBoxLayout *contentLay = qobject_cast<QVBoxLayout*>(ui->contentLayout);
	if (contentLay) {
		QHBoxLayout *repliesButtonLay = new QHBoxLayout();
		repliesButtonLay->setContentsMargins(0, 0, 0, 0);
		repliesButtonLay->addWidget(mViewRepliesButton);
		repliesButtonLay->addStretch();
		contentLay->addLayout(repliesButtonLay);
	}
}

CommentItemWidget::~CommentItemWidget()
{
	delete ui;
}

void CommentItemWidget::setMsgId(const RsGxsMessageId &id)
{
	mMsgId = id;
}

void CommentItemWidget::setAuthorId(const RsGxsId &id)
{
	mAuthorId = id;
}

void CommentItemWidget::setupStyle()
{
	// Style the upvote/downvote buttons
	ui->upvoteButton->setIcon(QIcon(":/icons/png/thumbs-up.png"));
	ui->downvoteButton->setIcon(QIcon(":/icons/png/thumbs-down.png"));
}

void CommentItemWidget::setAuthorName(const QString &name)
{
	mAuthorName = name;
	ui->authorLabel->setText(name);
}

void CommentItemWidget::setAuthorAvatar(const QPixmap &avatar)
{
	if (!avatar.isNull())
		ui->avatarLabel->setPixmap(avatar.scaled(ui->avatarLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

void CommentItemWidget::setCommentText(const QString &text)
{
	mCommentText = text;
	mFullSizeButton->setVisible(CommentText::setPreview(
	        ui->commentTextBrowser->document(), text,
	        ui->commentTextBrowser->viewport()->width()));
}

void CommentItemWidget::setDateTime(const QString &datetime)
{
	ui->dateTimeLabel->setText(datetime);
}

void CommentItemWidget::setScore(int score)
{
	ui->scoreLabel->setText(QString::number(score));
}

void CommentItemWidget::setUpvote(bool upvoted)
{
	mUpvoteActive = upvoted;
	ui->upvoteButton->setChecked(upvoted);
	if (upvoted) {
		ui->downvoteButton->setChecked(false);
		ui->downvoteButton->setStyleSheet("");
	} else {
		ui->upvoteButton->setStyleSheet("");
	}
}

void CommentItemWidget::setDownvote(bool downvoted)
{
	mDownvoteActive = downvoted;
	ui->downvoteButton->setChecked(downvoted);
	if (downvoted) {
		ui->upvoteButton->setChecked(false);
		ui->upvoteButton->setStyleSheet("");
	} else {
		ui->downvoteButton->setStyleSheet("");
	}
}

void CommentItemWidget::setReplyCount(int count)
{
	if (count > 0) {
		ui->replyButton->setText(QString("Reply (%1)").arg(count));
	} else {
		ui->replyButton->setText("Reply");
	}
}

void CommentItemWidget::setLevel(int level)
{
	mLevel = level;
	// Apply indentation for nested replies
	int leftMargin = 6 + (level * 24);
	layout()->setContentsMargins(leftMargin, 4, 6, 4);
}

void CommentItemWidget::on_upvoteButton_clicked()
{
	setUpvote(!mUpvoteActive);
	emit upvoteClicked(mMsgId);
}

void CommentItemWidget::on_downvoteButton_clicked()
{
	setDownvote(!mDownvoteActive);
	emit downvoteClicked(mMsgId);
}

void CommentItemWidget::on_replyButton_clicked()
{
	emit replyClicked(mMsgId);
}

void CommentItemWidget::on_authorLabel_linkActivated(const QString &link)
{
	Q_UNUSED(link);
	emit authorClicked(mAuthorId);
}

void CommentItemWidget::setViewRepliesCount(int count)
{
	mReplyCount = count;
	if (count <= 0) {
		mViewRepliesButton->hide();
		return;
	}
	mRepliesExpanded = false;
	mViewRepliesButton->setIcon(QIcon(":/icons/png/down-arrow.png"));
	mViewRepliesButton->setText(tr(" View %1 repl%2").arg(count).arg(count == 1 ? "y" : "ies"));
	mViewRepliesButton->show();
}

void CommentItemWidget::on_viewRepliesButton_clicked()
{
	mRepliesExpanded = !mRepliesExpanded;
	if (mRepliesExpanded){
		mViewRepliesButton->setIcon(QIcon(":/icons/png/up-arrow.png"));
		mViewRepliesButton->setText(tr(" Hide %1 repl%2").arg(mReplyCount).arg(mReplyCount == 1 ? "y" : "ies"));
	}else{
		mViewRepliesButton->setIcon(QIcon(":/icons/png/down-arrow.png"));
		mViewRepliesButton->setText(tr(" View %1 repl%2").arg(mReplyCount).arg(mReplyCount == 1 ? "y" : "ies"));
	}
	emit viewRepliesToggled(mMsgId, mRepliesExpanded);
}

void CommentItemWidget::setSelected(bool selected)
{
	mSelected = selected;
	if (selected)
		setStyleSheet("QWidget#CommentItemWidget { background-color: #e8f0fe; border-left: 3px solid #3ea6ff; }");
	else
		setStyleSheet("QWidget#CommentItemWidget { background-color: transparent; }");
}

void CommentItemWidget::mousePressEvent(QMouseEvent *event)
{
	QWidget::mousePressEvent(event);
	emit commentSelected(mMsgId);
}

bool CommentItemWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::Resize
	        && watched == ui->commentTextBrowser->viewport()) {
		auto resize = static_cast<QResizeEvent*>(event);
		if (resize->size().width() != resize->oldSize().width())
			setCommentText(mCommentText);
	}
	if (event->type() == QEvent::MouseButtonPress
	        && watched == ui->commentTextBrowser->viewport())
		emit commentSelected(mMsgId);
	return QWidget::eventFilter(watched, event);
}
