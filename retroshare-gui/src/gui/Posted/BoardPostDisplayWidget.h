/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardPostDisplayWidget.h                      *
 *                                                                             *
 * Copyright (C) 2019 by Retroshare Team       <retroshare.project@gmail.com>  *
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

#pragma once

#include <QMetaType>
#include <QWidget>

#include <retroshare/rsposted.h>

namespace Ui {
class BoardPostDisplayWidget_card;
class BoardPostDisplayWidget_compact;
}

class QPushButton;
class QFrame;
class QLabel;
class QToolButton;
class QTextEdit;
class ClickableLabel;
class GxsIdLabel;

struct RsPostedPost;

class BoardPostDisplayWidgetBase: public QWidget
{
	Q_OBJECT

public:
    enum DisplayFlags: uint8_t {
        SHOW_NONE                  = 0x00,
        SHOW_COMMENTS              = 0x01,
        SHOW_NOTES                 = 0x02,
    };

    enum DisplayMode: uint8_t {
        DISPLAY_MODE_UNKNOWN       = 0x00,
        DISPLAY_MODE_COMPACT       = 0x01,
        DISPLAY_MODE_CARD          = 0x02,
    };

    BoardPostDisplayWidgetBase(const RsPostedPost& post,uint8_t display_flags,QWidget *parent);
    virtual ~BoardPostDisplayWidgetBase() {}

	static const char *DEFAULT_BOARD_IMAGE;

protected slots:
	/* GxsGroupFeedItem */

    virtual void setup();    // to be overloaded by the different views

    virtual QToolButton *voteUpButton() =0;
    virtual QToolButton *commentButton() =0;
    virtual QToolButton *voteDownButton() =0;
    virtual QLabel      *newLabel() =0;
    virtual QLabel      *titleLabel()=0;
    virtual GxsIdLabel  *fromLabel()=0;
    virtual QLabel      *dateLabel()=0;
    virtual QLabel      *scoreLabel() =0;
    virtual QLabel      *notes() =0;
    virtual QLabel      *pictureLabel()=0;
    virtual QToolButton *readButton() =0;
    virtual QToolButton *shareButton() =0;
    virtual QFrame      *feedFrame() =0;

    void loadComments(bool e);
    void readToggled();
    void setReadStatus(bool isNew, bool isUnread) ;
    void makeUpVote() ;
    void makeDownVote() ;
	void setCommentsSize(int comNb) ;
    void handleShareButtonClicked() ;
    void handleCopyLinkClicked() ;


signals:
    void changeReadStatusRequested(const RsGxsMessageId&,bool);
    void vote(const RsGxsGrpMsgIdPair& msgId, bool up_or_down);
    void expand(RsGxsMessageId,bool);
    void commentsRequested(const RsGxsMessageId&,bool);
    void thumbnailOpenned();
    void shareButtonClicked();
    void copylinkClicked();

protected:
	RsPostedPost mPost;
    uint8_t      mDisplayFlags;
};

class BoardPostDisplayWidget_compact : public BoardPostDisplayWidgetBase
{
    Q_OBJECT

public:
    BoardPostDisplayWidget_compact(const RsPostedPost& post, uint8_t display_flags, QWidget *parent);
    virtual ~BoardPostDisplayWidget_compact();

    static const char *DEFAULT_BOARD_IMAGE;

    QToolButton *voteUpButton()   override;
    QToolButton *commentButton()  override;
    QToolButton *voteDownButton() override;
    QLabel      *newLabel()       override;
    GxsIdLabel  *fromLabel()      override;
    QLabel      *dateLabel()      override;
    QLabel      *titleLabel()     override;
    QLabel      *scoreLabel()     override;
    QLabel      *notes()          override;
    QLabel      *pictureLabel()   override;
    QToolButton *readButton()     override;
    QToolButton *shareButton()    override;
    QFrame      *feedFrame()      override;

public slots:
    void viewPicture() ;

protected slots:
    /* GxsGroupFeedItem */
    void doExpand(bool) ;

protected:
    void setup() override;    // to be overloaded by the different views

private:
    /** Qt Designer generated object */
    Ui::BoardPostDisplayWidget_compact *ui;
};

class BoardPostDisplayWidget_card : public BoardPostDisplayWidgetBase
{
    Q_OBJECT

public:
    BoardPostDisplayWidget_card(const RsPostedPost& post,uint8_t display_flags,QWidget *parent=nullptr);
    virtual ~BoardPostDisplayWidget_card();

    static const char *DEFAULT_BOARD_IMAGE;

    QToolButton *voteUpButton()   override;
    QToolButton *commentButton()  override;
    QToolButton *voteDownButton() override;
    QLabel      *newLabel()       override;
    GxsIdLabel  *fromLabel()      override;
    QLabel      *dateLabel()      override;
    QLabel      *titleLabel()     override;
    QLabel      *scoreLabel()     override;
    QLabel      *notes()          override;
    QToolButton *readButton()     override;
    QToolButton *shareButton()    override;
    QLabel      *pictureLabel()   override;
    QFrame      *feedFrame()      override;

protected slots:
    /* GxsGroupFeedItem */

protected:
    void setup() override;    // to be overloaded by the different views

private:
    /** Qt Designer generated object */
    Ui::BoardPostDisplayWidget_card *ui;
};
