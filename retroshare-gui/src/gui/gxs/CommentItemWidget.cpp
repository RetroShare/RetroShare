/*******************************************************************************
 * retroshare-gui/src/gui/gxs/CommentItemWidget.cpp                             *
 *                                                                             *
 * YouTube-style comment item widget for issue #1722                           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                            *
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
#include "ui_CommentItemWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QDebug>

CommentItemWidget::CommentItemWidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::CommentItemWidget), mLevel(0), mUpvoteActive(false), mDownvoteActive(false)
{
	ui->setupUi(this);
	setupStyle();
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
	setStyleSheet(QString("QWidget#CommentItemWidget { background-color: transparent; }"));

	// Style the upvote/downvote buttons
	ui->upvoteButton->setIcon(QIcon(":/icons/png/thumbs-up.png"));
	ui->downvoteButton->setIcon(QIcon(":/icons/png/thumbs-down.png"));
}

void CommentItemWidget::setAuthorName(const QString &name)
{
	ui->authorLabel->setText(QString("<a href=\"author:%1\" style=\"color: #3ea6ff;\">%2</a>")
	        .arg(QString::fromStdString(mAuthorId.toStdString()), name));
}

void CommentItemWidget::setAuthorAvatar(const QPixmap &avatar)
{
	ui->avatarLabel->setPixmap(avatar.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CommentItemWidget::setCommentText(const QString &text)
{
	ui->commentTextLabel->setText(text);
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
		ui->upvoteButton->setStyleSheet("QPushButton { color: #3ea6ff; }");
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
		ui->downvoteButton->setStyleSheet("QPushButton { color: #ff0000; }");
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
