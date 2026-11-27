/*******************************************************************************
 * gui/HomeChannelsWidget.h                                                    *
 *                                                                             *
 * Copyright (C) 2026 RetroShare Team <retroshare.project@gmail.com>           *
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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <QFrame>
#include <QPixmap>
#include <QWidget>

#include <retroshare/rsevents.h>
#include <retroshare/rsgxschannels.h>

class QGridLayout;
class QLabel;
class QLineEdit;
class QScrollArea;
class QSlider;
class QToolButton;

/*!
 * \brief The HomeChannelCard class
 * One card of the Home page grid: a thumbnail filling the top of the card,
 * with the title and the details in their own labels below, like on video sites.
 */
class HomeChannelCard : public QFrame
{
	Q_OBJECT

public:
	HomeChannelCard(const QPixmap& thumbnail, QWidget *parent = nullptr);

	// Adds a text row below the thumbnail. Rows have a fixed height so that all the cards of the grid line up.
	// rightText, if any, is shown right aligned on the same row (e.g. a post count).
	QLabel *addRow(const QString& text, int maxLines, bool bold, bool secondary, const QString& rightText = QString());

	RsGxsGroupId   mGroupId;
	RsGxsMessageId mMsgId;
	QString        mTitle;
	bool           mSubscribed = false;
	bool           mUnread = false;

	QLabel        *mImageLabel = nullptr;
	QPixmap        mThumbnail;		// without the "NEW" badge

	QLabel        *mTimeLabel = nullptr;	// "5 minutes ago", refreshed when the page is shown
	rstime_t       mTimeTs = 0;
	bool           mTimeIsUpdate = false;	// "Updated 5 minutes ago"

signals:
	void clicked(HomeChannelCard *card);
	void contextMenuRequested(HomeChannelCard *card, const QPoint& globalPos);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private:
	int  mWidth;
	bool mPressed;
};

/*!
 * \brief The HomeChannelsWidget class
 * Grid of channel content shown on the Home page (see issue #1424).
 *
 * In LatestPosts mode it shows the most recent posts of all the channels
 * the user is subscribed to, newest first, like the start page of a video
 * site. In PopularChannels mode it shows the channels the user is not
 * subscribed to yet, ordered by the number of friends subscribed to them.
 *
 * Every card opens the corresponding post or channel when clicked, and has
 * a context menu to copy its RetroShare link.
 *
 * Updates are driven by rsEvents, without polling nor timers:
 * - data is loaded in a background thread when the widget is shown,
 * - new/updated posts and channels, subscriptions... reload it while visible
 *   (one load at a time: events received during a load cause a single new load),
 * - read status changes only update the "NEW" badge of the cards concerned,
 * - hidden widgets just remember that they need a reload when shown again.
 */
class HomeChannelsWidget : public QWidget
{
	Q_OBJECT

public:
	enum Mode { LatestPosts, PopularChannels };

	explicit HomeChannelsWidget(Mode mode, QWidget *parent = nullptr);
	~HomeChannelsWidget() override;

	// One displayed entry. Filled in the loader thread, displayed in the GUI thread.
	struct Entry
	{
		Entry() : mPublishTs(0), mUnread(false), mSubscribed(false), mPopularity(0), mPostCount(0) {}

		RsGxsGroupId   mGroupId;
		RsGxsMessageId mMsgId;		// null in PopularChannels mode
		std::string    mTitle;		// post title, or channel name
		std::string    mChannelName;
		rstime_t       mPublishTs;	// post date, or channel last post date
		bool           mUnread;
		bool           mSubscribed;
		uint32_t       mPopularity;	// number of friends subscribed
		uint32_t       mPostCount;
		RsGxsImage     mImage;		// post thumbnail, or channel image
	};

	// "5 minutes ago", "3 days ago"...
	static QString timeAgo(rstime_t ts);

public slots:
	void reload();

protected:
	void showEvent(QShowEvent *event) override;
	bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
	void openCard(HomeChannelCard *card);
	void showCardMenu(HomeChannelCard *card, const QPoint& globalPos);
	void startSearch();
	void zoomChanged(int percent);

private:
	void handleEvent(std::shared_ptr<const RsEvent> event);
	void requestReload();
	void updateReadStatus(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId);
	void setCardUnread(HomeChannelCard *card, bool unread);
	void refreshTimes();
	static QString timeText(rstime_t ts, bool isUpdate);
	void fillGrid();	// builds the cards from mEntries
	void updateTitle();
	void layoutCards(bool force);
	QPixmap decodeImage(const RsGxsImage& image) const;
	QPixmap makeThumbnail(const QPixmap& source) const;
	QPixmap addNewBadge(const QPixmap& thumbnail) const;
	QSize thumbnailSize() const;

	static void openContent(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId);
	// An empty query loads the default content. Otherwise:
	// - LatestPosts: subscribed channel posts whose title or channel name contains the query
	// - PopularChannels: all known channels (subscribed or not) whose name contains the query
	// Images of the entries whose key (see imageKey()) is in cachedImages are not fetched.
	static void loadLatestPosts(std::vector<Entry>& entries, size_t maxCount, const QString& query, const std::set<std::string>& cachedImages);
	static void loadPopularChannels(std::vector<Entry>& entries, size_t maxCount, const QString& query, const std::set<std::string>& cachedImages);
	static std::string imageKey(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId);

	Mode mMode;

	QLineEdit    *mSearchEdit;
	QToolButton  *mSearchButton;
	QString       mQuery;		// current search, empty for the default view
	bool          mScrollToTop;	// a new search starts at the top, updates keep the scroll position

	QLabel       *mTitleLabel;
	QLabel       *mStatusLabel;
	QSlider      *mZoomSlider;
	QLabel       *mZoomLabel;
	int           mZoom;		// size of the cards in percent, saved in the settings
	QScrollArea  *mScrollArea;
	QWidget      *mGridWidget;
	QGridLayout  *mGrid;

	std::vector<HomeChannelCard*> mCards;
	int mColumns;

	bool     mDirty;	// data needs to be (re)loaded
	bool     mLoading;	// a loader thread is running
	uint32_t mGeneration;	// drops results of obsolete loads

	// Displayed entries (without their image data), to rebuild the cards when zooming.
	std::vector<Entry> mEntries;

	// Original pictures of the displayed cards, so that reloads (e.g. when friend
	// counts change) and zooming don't fetch and decode the same pictures again.
	std::map<std::string, QPixmap> mImageCache;

	RsEventsHandlerId_t mEventHandlerId;
};
