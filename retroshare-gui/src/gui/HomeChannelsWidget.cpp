/*******************************************************************************
 * gui/HomeChannelsWidget.cpp                                                  *
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

#include "HomeChannelsWidget.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <set>

#include <QContextMenuEvent>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QSlider>
#include <QTextLayout>
#include <QToolButton>
#include <QVBoxLayout>

#include <retroshare/rsgxsflags.h>
#include <util/rsdebug.h>
#include <util/rsthreads.h>

#include "gui/MainWindow.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxschannels/GxsChannelDialog.h"
#include "gui/gxschannels/GxsChannelPostThumbnail.h"
#include "settings/rsharesettings.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

// Number of entries displayed in each mode.
static const size_t MAX_LATEST_POSTS      = 48;
static const size_t MAX_POPULAR_CHANNELS  = 48;

// Spacing between the cards, in font heights
static const float CARD_SPACING = 1.0f;

// Zoom of the cards, in percent
static const int ZOOM_MIN     = 50;
static const int ZOOM_MAX     = 200;
static const int ZOOM_STEP    = 10;
static const int ZOOM_DEFAULT = 100;

static const char *CHANNEL_DEFAULT_IMAGE = ":/icons/png/channels.png";

/******************************************************************************/
/*                               HomeChannelCard                              */
/******************************************************************************/

// Elides a text on at most maxLines lines of the given width.
static QString elideLines(const QString& text, const QFont& font, int width, int maxLines)
{
	QFontMetrics fm(font);
	QTextLayout layout(text, font);
	layout.beginLayout();

	QStringList lines;
	int start = 0;

	for(int i = 0; i < maxLines; ++i)
	{
		QTextLine line = layout.createLine();

		if(!line.isValid())
			break;

		line.setLineWidth(width);

		if(i == maxLines - 1)	// last allowed line: elide everything that remains
		{
			lines << fm.elidedText(text.mid(start).simplified(), Qt::ElideRight, width);
			start = text.length();
			break;
		}

		lines << text.mid(line.textStart(), line.textLength()).trimmed();
		start = line.textStart() + line.textLength();
	}
	layout.endLayout();

	return lines.join("\n");
}

HomeChannelCard::HomeChannelCard(const QPixmap& thumbnail, QWidget *parent)
    : QFrame(parent), mWidth(thumbnail.width()), mPressed(false)
{
	setObjectName("homeChannelCard");
	setAttribute(Qt::WA_Hover);
	setCursor(Qt::PointingHandCursor);
	setStyleSheet("QFrame#homeChannelCard { border-radius: 8px; background: transparent; }"
	              "QFrame#homeChannelCard:hover { background: rgba(128,128,128,40); }");

	int pad = QFontMetrics(font()).height() / 3;

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(pad, pad, pad, pad);
	layout->setSpacing(pad / 2);

	QLabel *image = new QLabel(this);
	image->setPixmap(thumbnail);
	mImageLabel = image;
	mThumbnail = thumbnail;
	image->setFixedSize(thumbnail.size() / thumbnail.devicePixelRatio());
	mWidth = image->width();
	layout->addWidget(image);

	layout->addStretch();
	setFixedWidth(mWidth + 2 * pad);
}

QLabel *HomeChannelCard::addRow(const QString& text, int maxLines, bool bold, bool secondary, const QString& rightText)
{
	QVBoxLayout *layout = static_cast<QVBoxLayout*>(this->layout());

	QFont f = font();
	f.setBold(bold);
	if(secondary)
		f.setPointSizeF(f.pointSizeF() * 0.9);

	QFontMetrics fm(f);

	QLabel *right = nullptr;
	int rightWidth = 0;

	if(!rightText.isEmpty())
	{
		right = new QLabel(rightText, this);
		right->setFont(font());
		right->setAlignment(Qt::AlignRight | Qt::AlignTop);
		rightWidth = QFontMetrics(font()).horizontalAdvance(rightText) + fm.height() / 2;
	}

	QLabel *label = new QLabel(this);
	label->setFont(f);
	label->setTextFormat(Qt::PlainText);
	label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	label->setText(elideLines(text, f, mWidth - rightWidth, maxLines));
	label->setFixedHeight(maxLines * fm.lineSpacing());

	if(secondary)
	{
		// Dimmed text that follows the colour of the current theme
		QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(label);
		effect->setOpacity(0.7);
		label->setGraphicsEffect(effect);
	}

	// The layout ends with a stretch: insert the rows before it
	int pos = layout->count() - 1;

	if(right)
	{
		QHBoxLayout *row = new QHBoxLayout;
		row->setContentsMargins(0, 0, 0, 0);
		row->addWidget(label, 1);
		row->addWidget(right);
		layout->insertLayout(pos, row);
	}
	else
		layout->insertWidget(pos, label);

	return label;
}

void HomeChannelCard::mousePressEvent(QMouseEvent *event)
{
	mPressed = (event->button() == Qt::LeftButton);
	QFrame::mousePressEvent(event);
}

void HomeChannelCard::mouseReleaseEvent(QMouseEvent *event)
{
	bool click = mPressed && event->button() == Qt::LeftButton && rect().contains(event->pos());
	mPressed = false;

	QFrame::mouseReleaseEvent(event);

	if(click)
		emit clicked(this);
}

void HomeChannelCard::contextMenuEvent(QContextMenuEvent *event)
{
	emit contextMenuRequested(this, event->globalPos());
}

/******************************************************************************/
/*                             HomeChannelsWidget                             */
/******************************************************************************/

HomeChannelsWidget::HomeChannelsWidget(Mode mode, QWidget *parent)
    : QWidget(parent), mMode(mode), mScrollToTop(true), mZoom(ZOOM_DEFAULT), mColumns(0), mDirty(true), mLoading(false), mGeneration(0), mEventHandlerId(0)
{
	QVBoxLayout *layout = new QVBoxLayout(this);

	// Search bar on top, in the style of video sites: a rounded field with the search button attached on its right.

	int fh = QFontMetrics(font()).height();
	int barHeight = fh * 2.2;
	int radius = barHeight / 2;

	mSearchEdit = new QLineEdit(this);
	mSearchEdit->setObjectName("homeSearchEdit");
	mSearchEdit->setPlaceholderText(mMode == LatestPosts ? tr("Search posts in your channels") : tr("Search channels"));
	mSearchEdit->setClearButtonEnabled(true);
	mSearchEdit->setFixedHeight(barHeight);
	mSearchEdit->setMaximumWidth(40 * fh);
	mSearchEdit->setStyleSheet(QString(
	        "QLineEdit#homeSearchEdit { border: 1px solid palette(mid); border-right: none;"
	        " border-top-left-radius: %1px; border-bottom-left-radius: %1px;"
	        " padding-left: %2px; padding-right: %2px; background: palette(base); }"
	        "QLineEdit#homeSearchEdit:focus { border-color: palette(highlight); }").arg(radius).arg(radius / 2));

	mSearchButton = new QToolButton(this);
	mSearchButton->setObjectName("homeSearchButton");
	mSearchButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/svg/search.svg"));
	mSearchButton->setIconSize(QSize(fh * 1.2, fh * 1.2));
	mSearchButton->setToolTip(tr("Search"));
	mSearchButton->setCursor(Qt::PointingHandCursor);
	mSearchButton->setFixedSize(barHeight * 1.6, barHeight);
	mSearchButton->setStyleSheet(QString(
	        "QToolButton#homeSearchButton { border: 1px solid palette(mid);"
	        " border-top-right-radius: %1px; border-bottom-right-radius: %1px; background: palette(button); }"
	        "QToolButton#homeSearchButton:hover { background: palette(midlight); }").arg(radius));

	QHBoxLayout *searchBar = new QHBoxLayout;
	searchBar->setSpacing(0);
	searchBar->addStretch();
	searchBar->addWidget(mSearchEdit, 1);
	searchBar->addWidget(mSearchButton);
	searchBar->addStretch();
	layout->addLayout(searchBar);

	// Search while typing. Only one search runs at a time, so fast typing just
	// replaces the pending search by the latest text (see reload()).
	connect(mSearchEdit, SIGNAL(textChanged(QString)), this, SLOT(startSearch()));
	connect(mSearchEdit, SIGNAL(returnPressed()), this, SLOT(startSearch()));
	connect(mSearchButton, SIGNAL(clicked()), this, SLOT(startSearch()));

	QHBoxLayout *header = new QHBoxLayout;

	mTitleLabel = new QLabel(this);
	QFont f = mTitleLabel->font();
	f.setBold(true);
	f.setPointSizeF(f.pointSizeF() * 1.3);
	mTitleLabel->setFont(f);

	updateTitle();

	mStatusLabel = new QLabel(this);

	// Zoom slider, the value is saved separately for each view
	mZoom = Settings->value(mMode == LatestPosts ? "/HomePage/zoomLatestPosts" : "/HomePage/zoomPopularChannels", ZOOM_DEFAULT).toInt();
	mZoom = std::max(ZOOM_MIN, std::min(ZOOM_MAX, mZoom));

	QLabel *zoomIcon = new QLabel(tr("Zoom"), this);

	mZoomSlider = new QSlider(Qt::Horizontal, this);
	mZoomSlider->setRange(ZOOM_MIN / ZOOM_STEP, ZOOM_MAX / ZOOM_STEP);	// in steps, so that the value is always a multiple of ZOOM_STEP
	mZoomSlider->setPageStep(1);
	mZoomSlider->setValue(mZoom / ZOOM_STEP);
	mZoomSlider->setFixedWidth(10 * QFontMetrics(font()).height());
	mZoomSlider->setToolTip(tr("Zoom"));

	mZoomLabel = new QLabel(QString("%1%").arg(mZoom), this);
	mZoomLabel->setMinimumWidth(QFontMetrics(font()).horizontalAdvance("200%"));

	connect(mZoomSlider, &QSlider::valueChanged, this, [this](int steps) { zoomChanged(steps * ZOOM_STEP); });

	header->addWidget(mTitleLabel);
	header->addStretch();
	header->addWidget(mStatusLabel);
	header->addSpacing(QFontMetrics(font()).height());
	header->addWidget(zoomIcon);
	header->addWidget(mZoomSlider);
	header->addWidget(mZoomLabel);
	layout->addLayout(header);

	// Cards are laid out in a grid whose number of columns follows the width of the page.

	mGridWidget = new QWidget;
	mGrid = new QGridLayout(mGridWidget);
	int spacing = CARD_SPACING * QFontMetrics(font()).height();
	mGrid->setHorizontalSpacing(spacing);
	mGrid->setVerticalSpacing(spacing);
	mGrid->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	mScrollArea = new QScrollArea(this);
	mScrollArea->setWidgetResizable(true);
	mScrollArea->setFrameShape(QFrame::NoFrame);
	mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mScrollArea->setWidget(mGridWidget);
	mScrollArea->viewport()->installEventFilter(this);
	layout->addWidget(mScrollArea);

	// Events are sent by another thread
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject([=]() { handleEvent(event); }, this ); },
	            mEventHandlerId, RsEventType::GXS_CHANNELS );
}

HomeChannelsWidget::~HomeChannelsWidget()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void HomeChannelsWidget::updateTitle()
{
	if(!mQuery.isEmpty())
		mTitleLabel->setText(tr("Search results for \"%1\"").arg(mQuery));
	else if(mMode == LatestPosts)
		mTitleLabel->setText(tr("Latest posts from your subscribed channels"));
	else
		mTitleLabel->setText(tr("Popular channels you are not subscribed to"));
}

void HomeChannelsWidget::startSearch()
{
	QString query = mSearchEdit->text().simplified();

	if(query == mQuery)
		return;

	mQuery = query;
	mScrollToTop = true;
	updateTitle();
	reload();
}

QSize HomeChannelsWidget::thumbnailSize() const
{
	// 16:9 thumbnails for posts, 5:4 pictures for channels. Size follows the font so that it scales with DPI.
	int h = QFontMetrics(font()).height();

	int w = (mMode == LatestPosts ? 14 : 10) * h * mZoom / 100;

	if(mMode == LatestPosts)
		return QSize(w, w * 9 / 16);
	else
		return QSize(w, w * 4 / 5);
}

void HomeChannelsWidget::zoomChanged(int percent)
{
	if(percent == mZoom)
		return;

	mZoom = percent;
	mZoomLabel->setText(QString("%1%").arg(mZoom));
	Settings->setValue(mMode == LatestPosts ? "/HomePage/zoomLatestPosts" : "/HomePage/zoomPopularChannels", mZoom);

	// Rebuild the cards from the displayed entries and the cached pictures: nothing to load.
	if(!mCards.empty())
		fillGrid();
}

/* static */
QString HomeChannelsWidget::timeAgo(rstime_t ts)
{
	if(ts <= 0)
		return QString();

	qint64 secs = QDateTime::currentSecsSinceEpoch() - (qint64)ts;

	if(secs < 60)
		return tr("just now");

	qint64 mins = secs / 60;
	if(mins < 60)
		return tr("%n minute(s) ago", "", int(mins));

	qint64 hours = mins / 60;
	if(hours < 24)
		return tr("%n hour(s) ago", "", int(hours));

	qint64 days = hours / 24;
	if(days < 7)
		return tr("%n day(s) ago", "", int(days));
	if(days < 31)
		return tr("%n week(s) ago", "", int(days / 7));
	if(days < 365)
		return tr("%n month(s) ago", "", int(days / 30));

	return tr("%n year(s) ago", "", int(days / 365));
}

/* static */
QString HomeChannelsWidget::timeText(rstime_t ts, bool isUpdate)
{
	if(ts <= 0)
		return QString();

	return isUpdate ? tr("Updated %1").arg(timeAgo(ts)) : timeAgo(ts);
}

void HomeChannelsWidget::refreshTimes()
{
	for(auto card : mCards)
		if(card->mTimeLabel)
			card->mTimeLabel->setText(timeText(card->mTimeTs, card->mTimeIsUpdate));
}

void HomeChannelsWidget::handleEvent(std::shared_ptr<const RsEvent> event)
{
	const RsGxsChannelEvent *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

	if(!e)
		return;

	switch(e->mChannelEventCode)
	{
	case RsChannelEventCode::NEW_MESSAGE:
	case RsChannelEventCode::UPDATED_MESSAGE:
		if(mMode == LatestPosts)
			requestReload();
		break;

	case RsChannelEventCode::READ_STATUS_CHANGED:
		// No need to reload everything: only the "NEW" badges change.
		if(mMode == LatestPosts)
			updateReadStatus(e->mChannelGroupId, e->mChannelMsgId);
		break;

	case RsChannelEventCode::STATISTICS_CHANGED:
		// Friend and post counts of channels. They are only known once friends
		// have sent their statistics, i.e. some time after start. libretroshare
		// only notifies actual changes, so this settles by itself.
		if(mMode == PopularChannels)
			requestReload();
		break;

	case RsChannelEventCode::UPDATED_CHANNEL:
		mImageCache.erase(imageKey(e->mChannelGroupId, RsGxsMessageId()));	// the channel image may have changed
		requestReload();
		break;

	case RsChannelEventCode::NEW_CHANNEL:
	case RsChannelEventCode::SUBSCRIBE_STATUS_CHANGED:
	case RsChannelEventCode::DELETED_CHANNEL:
		requestReload();
		break;

	default:
		break;
	}
}

void HomeChannelsWidget::requestReload()
{
	mDirty = true;

	// Only reload what the user can see. Hidden widgets reload when shown.
	if(isVisible())
		reload();
}

void HomeChannelsWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	refreshTimes();

	// Also retry when nothing was found: at startup the Home page can be shown
	// before the channels are loaded.
	if(mDirty || mCards.empty())
		reload();
}

void HomeChannelsWidget::updateReadStatus(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId)
{
	// Posts of this channel that are displayed. A null msgId means the whole channel changed.

	std::set<RsGxsMessageId> displayed;

	for(auto card : mCards)
		if(card->mGroupId == groupId && (msgId.isNull() || card->mMsgId == msgId))
			displayed.insert(card->mMsgId);

	if(displayed.empty())
		return;

	RsThread::async([this, groupId, displayed]()
	{
		// Metadata only: no need to load the posts themselves
		std::vector<RsMsgMetaData> summaries;

		if(!rsGxsChannels->getContentSummaries(groupId, summaries))
			return;

		auto unread = new std::map<RsGxsMessageId, bool>();

		for(const auto& m : summaries)
			if(displayed.count(m.mMsgId))
				(*unread)[m.mMsgId] = IS_MSG_UNREAD(m.mMsgStatus) || IS_MSG_NEW(m.mMsgStatus);

		RsQThreadUtils::postToObject([this, groupId, unread]()
		{
			// Look the cards up again: they may have been replaced in the meantime
			for(auto card : mCards)
				if(card->mGroupId == groupId)
				{
					auto it = unread->find(card->mMsgId);

					if(it != unread->end())
						setCardUnread(card, it->second);
				}

			delete unread;
		}, this);
	});
}

void HomeChannelsWidget::setCardUnread(HomeChannelCard *card, bool unread)
{
	if(card->mUnread == unread)
		return;

	card->mUnread = unread;
	card->mImageLabel->setPixmap(unread ? addNewBadge(card->mThumbnail) : card->mThumbnail);
}

bool HomeChannelsWidget::eventFilter(QObject *obj, QEvent *event)
{
	if(obj == mScrollArea->viewport() && event->type() == QEvent::Resize)
		layoutCards(false);

	return QWidget::eventFilter(obj, event);
}

void HomeChannelsWidget::reload()
{
	if(mLoading)	// will be reloaded when the current load is done
	{
		mDirty = true;
		return;
	}

	mDirty = false;
	mLoading = true;
	uint32_t generation = ++mGeneration;
	Mode mode = mMode;
	QString query = mQuery;

	if(mCards.empty())
		mStatusLabel->setText(tr("Loading..."));

	std::set<std::string> cachedImages;
	for(const auto& c : mImageCache)
		cachedImages.insert(c.first);

	RsThread::async([this, mode, generation, query, cachedImages]()
	{
		auto entries = new std::vector<Entry>();

		if(mode == LatestPosts)
			loadLatestPosts(*entries, MAX_LATEST_POSTS, query, cachedImages);
		else
			loadPopularChannels(*entries, MAX_POPULAR_CHANNELS, query, cachedImages);

		RsQThreadUtils::postToObject([this, entries, generation, query]()
		{
			bool obsolete = (query != mQuery);	// the search changed while loading

			if(generation == mGeneration && !obsolete)
			{
				mEntries = std::move(*entries);
				fillGrid();
			}

			delete entries;

			mLoading = false;

			// Events received during the load are handled by a single new load.
			if(obsolete || (mDirty && isVisible()))
				reload();
		}, this);
	});
}

/* static */
std::string HomeChannelsWidget::imageKey(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId)
{
	return msgId.isNull() ? groupId.toStdString() : msgId.toStdString();
}

/* static */
void HomeChannelsWidget::loadLatestPosts(std::vector<Entry>& entries, size_t maxCount, const QString& query, const std::set<std::string>& cachedImages)
{
	std::list<RsGroupMetaData> groups;

	if(!rsGxsChannels->getChannelsSummaries(groups))
	{
		RsErr() << __PRETTY_FUNCTION__ << " cannot retrieve channel summaries" ;
		return;
	}

	// Subscribed channels, those with the most recent posts first, so that we
	// can stop as soon as no remaining channel can contribute a newer post.

	std::vector<const RsGroupMetaData*> subscribed;

	for(const auto& g : groups)
		if(IS_GROUP_SUBSCRIBED(g.mSubscribeFlags) || IS_GROUP_ADMIN(g.mSubscribeFlags))
			subscribed.push_back(&g);

	std::sort(subscribed.begin(), subscribed.end(),
	          [](const RsGroupMetaData *a, const RsGroupMetaData *b) { return a->mLastPost > b->mLastPost; });

	struct Candidate
	{
		const RsGroupMetaData *mGroup;
		RsMsgMetaData mMeta;
	};
	std::vector<Candidate> candidates;

	auto oldestKeptTs = [&]() -> rstime_t
	{
		return (candidates.size() < maxCount) ? 0 : candidates[maxCount-1].mMeta.mPublishTs;
	};

	for(const RsGroupMetaData *g : subscribed)
	{
		// mLastPost is a hint only (0 when unknown), so only use it to stop when it is set.
		if(g->mLastPost > 0 && g->mLastPost < oldestKeptTs())
			break;

		// When the channel name matches the search, all its posts match.
		bool channelMatches = query.isEmpty() || QString::fromUtf8(g->mGroupName.c_str()).contains(query, Qt::CaseInsensitive);

		std::vector<RsMsgMetaData> summaries;

		if(!rsGxsChannels->getContentSummaries(g->mGroupId, summaries))
			continue;

		// Keep posts only (comments and votes have a thread/parent id), and only
		// the latest version of edited posts. A new version of a post references
		// the original post in mOrigMsgId.

		std::map<RsGxsMessageId, const RsMsgMetaData*> latestVersions;

		for(const auto& m : summaries)
		{
			if(!m.mThreadId.isNull() || !m.mParentId.isNull())
				continue;

			const RsGxsMessageId& key = m.mOrigMsgId.isNull() ? m.mMsgId : m.mOrigMsgId;
			auto it = latestVersions.find(key);

			if(it == latestVersions.end() || it->second->mPublishTs < m.mPublishTs)
				latestVersions[key] = &m;
		}

		for(const auto& p : latestVersions)
			if(channelMatches || QString::fromUtf8(p.second->mMsgName.c_str()).contains(query, Qt::CaseInsensitive))
				candidates.push_back({ g, *p.second });

		std::sort(candidates.begin(), candidates.end(),
		          [](const Candidate& a, const Candidate& b) { return a.mMeta.mPublishTs > b.mMeta.mPublishTs; });

		if(candidates.size() > maxCount)
			candidates.resize(maxCount);
	}

	// Now fetch the actual posts, for the thumbnails, one request per channel.

	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > wanted;

	for(const auto& c : candidates)
		if(!cachedImages.count(imageKey(c.mGroup->mGroupId, c.mMeta.mMsgId)))
			wanted[c.mGroup->mGroupId].insert(c.mMeta.mMsgId);

	std::map<RsGxsMessageId, RsGxsImage> thumbnails;

	for(const auto& w : wanted)
	{
		std::vector<RsGxsChannelPost> posts;
		std::vector<RsGxsComment> comments;
		std::vector<RsGxsVote> votes;

		if(!rsGxsChannels->getChannelContent(w.first, w.second, posts, comments, votes))
			continue;

		for(auto& p : posts)
			thumbnails[p.mMeta.mMsgId] = std::move(p.mThumbnail);
	}

	entries.reserve(candidates.size());

	for(const auto& c : candidates)
	{
		Entry e;
		e.mGroupId     = c.mGroup->mGroupId;
		e.mMsgId       = c.mMeta.mMsgId;
		e.mTitle       = c.mMeta.mMsgName;
		e.mChannelName = c.mGroup->mGroupName;
		e.mPublishTs   = c.mMeta.mPublishTs;
		e.mUnread      = IS_MSG_UNREAD(c.mMeta.mMsgStatus) || IS_MSG_NEW(c.mMeta.mMsgStatus);
		e.mSubscribed  = true;
		e.mPopularity  = c.mGroup->mPop;
		e.mPostCount   = c.mGroup->mVisibleMsgCount;

		auto it = thumbnails.find(c.mMeta.mMsgId);
		if(it != thumbnails.end())
			e.mImage = std::move(it->second);

		entries.push_back(std::move(e));
	}
}

/* static */
void HomeChannelsWidget::loadPopularChannels(std::vector<Entry>& entries, size_t maxCount, const QString& query, const std::set<std::string>& cachedImages)
{
	std::list<RsGroupMetaData> groups;

	if(!rsGxsChannels->getChannelsSummaries(groups))
	{
		RsErr() << __PRETTY_FUNCTION__ << " cannot retrieve channel summaries" ;
		return;
	}

	std::vector<const RsGroupMetaData*> others;

	for(const auto& g : groups)
	{
		bool subscribed = IS_GROUP_SUBSCRIBED(g.mSubscribeFlags) || IS_GROUP_ADMIN(g.mSubscribeFlags);

		if(query.isEmpty())
		{
			if(!subscribed)
				others.push_back(&g);
		}
		else if(QString::fromUtf8(g.mGroupName.c_str()).contains(query, Qt::CaseInsensitive))
			others.push_back(&g);	// searching: all the known channels
	}

	// Most friends subscribed first, then the most active ones.
	std::sort(others.begin(), others.end(), [](const RsGroupMetaData *a, const RsGroupMetaData *b)
	{
		if(a->mPop != b->mPop) return a->mPop > b->mPop;
		if(a->mVisibleMsgCount != b->mVisibleMsgCount) return a->mVisibleMsgCount > b->mVisibleMsgCount;
		return a->mLastPost > b->mLastPost;
	});

	if(others.size() > maxCount)
		others.resize(maxCount);

	// Channel images

	std::list<RsGxsGroupId> ids;
	for(const auto *g : others)
		if(!cachedImages.count(imageKey(g->mGroupId, RsGxsMessageId())))
			ids.push_back(g->mGroupId);

	std::map<RsGxsGroupId, RsGxsImage> images;
	std::vector<RsGxsChannelGroup> infos;

	if(!ids.empty() && rsGxsChannels->getChannelsInfo(ids, infos))
		for(auto& info : infos)
			images[info.mMeta.mGroupId] = std::move(info.mImage);

	entries.reserve(others.size());

	for(const auto *g : others)
	{
		Entry e;
		e.mGroupId     = g->mGroupId;
		e.mTitle       = g->mGroupName;
		e.mChannelName = g->mGroupName;
		e.mPublishTs   = g->mLastPost;
		e.mPopularity  = g->mPop;
		e.mPostCount   = g->mVisibleMsgCount;
		e.mSubscribed  = IS_GROUP_SUBSCRIBED(g->mSubscribeFlags) || IS_GROUP_ADMIN(g->mSubscribeFlags);

		auto it = images.find(g->mGroupId);
		if(it != images.end())
			e.mImage = std::move(it->second);

		entries.push_back(std::move(e));
	}
}

QPixmap HomeChannelsWidget::decodeImage(const RsGxsImage& image) const
{
	QPixmap pix;

	if(image.mSize == 0 || !GxsIdDetails::loadPixmapFromData(image.mData, image.mSize, pix, GxsIdDetails::ORIGINAL) || pix.isNull())
		pix = FilesDefs::getPixmapFromQtResourcePath(mMode == LatestPosts ? ChannelPostThumbnailView::CHAN_DEFAULT_IMAGE : CHANNEL_DEFAULT_IMAGE);

	return pix;
}

QPixmap HomeChannelsWidget::makeThumbnail(const QPixmap& pix) const
{
	// The picture fills the whole thumbnail (cropped in the middle if needed),
	// with rounded corners, so that all the cards have the same size.

	QSize size = thumbnailSize();
	qreal dpr = devicePixelRatioF();

	QPixmap canvas(size * dpr);
	canvas.setDevicePixelRatio(dpr);
	canvas.fill(Qt::transparent);

	QPixmap scaled = pix.scaled(size * dpr, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	scaled.setDevicePixelRatio(dpr);
	QSizeF scaledSize = QSizeF(scaled.size()) / dpr;

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	qreal radius = QFontMetrics(font()).height() / 2.0;
	QPainterPath clip;
	clip.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), radius, radius);
	painter.setClipPath(clip);

	painter.drawPixmap(QPointF((size.width() - scaledSize.width()) / 2, (size.height() - scaledSize.height()) / 2), scaled);

	painter.end();

	return canvas;
}

QPixmap HomeChannelsWidget::addNewBadge(const QPixmap& thumbnail) const
{
	// "NEW" badge in the top left corner
	QPixmap canvas = thumbnail.copy();
	canvas.setDevicePixelRatio(thumbnail.devicePixelRatio());

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	qreal radius = QFontMetrics(font()).height() / 2.0;

	{
		QFont f = font();
		f.setBold(true);
		f.setPointSizeF(f.pointSizeF() * 0.8);
		painter.setFont(f);

		QString txt = tr("NEW");
		QFontMetrics fm(f);
		QRectF badge(radius / 2, radius / 2, fm.horizontalAdvance(txt) + fm.height(), fm.height() * 1.3);

		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(204, 0, 0));
		painter.drawRoundedRect(badge, badge.height() / 4, badge.height() / 4);
		painter.setPen(Qt::white);
		painter.drawText(badge, Qt::AlignCenter, txt);
	}

	painter.end();

	return canvas;
}

void HomeChannelsWidget::fillGrid()
{
	std::vector<Entry>& entries(mEntries);

	// Updates keep the scroll position, a new search starts at the top.
	int scrollPos = mScrollToTop ? 0 : mScrollArea->verticalScrollBar()->value();
	mScrollToTop = false;

	for(auto card : mCards)
	{
		mGrid->removeWidget(card);
		card->hide();
		card->deleteLater();
	}

	mCards.clear();

	std::map<std::string, QPixmap> newCache;	// only keeps the pictures still displayed

	for(auto& e : entries)
	{
		QString title   = QString::fromUtf8(e.mTitle.c_str());
		QString channel = QString::fromUtf8(e.mChannelName.c_str());

		// Original picture: just received, or already in the cache
		std::string key = imageKey(e.mGroupId, e.mMsgId);
		auto cached = mImageCache.find(key);

		QPixmap source = (cached != mImageCache.end() && e.mImage.mSize == 0) ? cached->second : decodeImage(e.mImage);
		newCache[key] = source;
		e.mImage.clear();	// the picture is in the cache now, no need to keep its data twice

		HomeChannelCard *card = new HomeChannelCard(makeThumbnail(source), mGridWidget);
		card->mGroupId = e.mGroupId;
		card->mMsgId   = e.mMsgId;
		card->mTitle   = title;
		card->mSubscribed = e.mSubscribed;
		card->mTimeTs = e.mPublishTs;
		card->mTimeIsUpdate = (mMode == PopularChannels);
		setCardUnread(card, e.mUnread);

		if(mMode == LatestPosts)
		{
			// Title on two lines, then the channel and the date, as on video sites
			card->addRow(title, 2, true, false);
			card->addRow(channel, 1, false, true);
			card->mTimeLabel = card->addRow(timeText(e.mPublishTs, false), 1, false, true);

			card->setToolTip(QString("<b>%1</b><br/>%2: %3<br/>%4: %5")
			                 .arg(title.toHtmlEscaped(), tr("Channel"), channel.toHtmlEscaped(),
			                      tr("Published"), DateTime::formatDateTime(e.mPublishTs)));
		}
		else
		{
			// Name with the number of posts on the right, then the activity
			QString pop = tr("%n friend(s) subscribed", "", int(e.mPopularity));

			card->addRow(title, 1, true, false, QString("(%1)").arg(e.mPostCount));
			card->addRow(e.mSubscribed ? tr("Subscribed") + QString(" ") + QChar(0x00B7) + " " + pop : pop, 1, false, true);
			card->mTimeLabel = card->addRow(timeText(e.mPublishTs, true), 1, false, true);

			QString tooltip = QString("<b>%1</b><br/>%2<br/>%3").arg(title.toHtmlEscaped(), pop, tr("%n post(s)", "", int(e.mPostCount)));
			if(e.mPublishTs > 0)
				tooltip += "<br/>" + tr("Last post: %1").arg(DateTime::formatDateTime(e.mPublishTs));

			card->setToolTip(tooltip);
		}

		connect(card, SIGNAL(clicked(HomeChannelCard*)), this, SLOT(openCard(HomeChannelCard*)));
		connect(card, SIGNAL(contextMenuRequested(HomeChannelCard*,QPoint)), this, SLOT(showCardMenu(HomeChannelCard*,QPoint)));

		mCards.push_back(card);
	}

	mImageCache = std::move(newCache);

	layoutCards(true);

	// The scroll range is only updated once the new layout is applied: restore the position after that.
	RsQThreadUtils::postToObject([this, scrollPos]() { mScrollArea->verticalScrollBar()->setValue(scrollPos); }, this);

	if(!mQuery.isEmpty())
		mStatusLabel->setText(entries.empty() ? tr("No results") : tr("%n result(s)", "", int(entries.size())));
	else if(entries.empty())
		mStatusLabel->setText(mMode == LatestPosts
		                      ? tr("No posts yet. Subscribe to some channels to see their latest posts here.")
		                      : tr("No other channels known yet."));
	else
		mStatusLabel->clear();
}

void HomeChannelsWidget::layoutCards(bool force)
{
	if(mCards.empty())
		return;

	int cardWidth = mCards.front()->maximumWidth();
	int spacing   = mGrid->horizontalSpacing();
	QMargins m    = mGrid->contentsMargins();
	int available = mScrollArea->viewport()->width() - m.left() - m.right();

	int columns = std::max(1, (available + spacing) / (cardWidth + spacing));

	if(!force && columns == mColumns)
		return;

	mColumns = columns;

	for(auto card : mCards)
		mGrid->removeWidget(card);

	for(size_t i = 0; i < mCards.size(); ++i)
	{
		mGrid->addWidget(mCards[i], i / columns, i % columns, Qt::AlignLeft | Qt::AlignTop);
		mCards[i]->show();
	}
}

void HomeChannelsWidget::openCard(HomeChannelCard *card)
{
	if(card)
		openContent(card->mGroupId, card->mMsgId);
}

/* static */
void HomeChannelsWidget::openContent(const RsGxsGroupId& groupId, const RsGxsMessageId& msgId)
{
	MainWindow::showWindow(MainWindow::Channels);

	GxsChannelDialog *channelDialog = dynamic_cast<GxsChannelDialog*>(MainWindow::getPage(MainWindow::Channels));

	if(channelDialog)
		channelDialog->navigate(groupId, msgId);
}

void HomeChannelsWidget::showCardMenu(HomeChannelCard *card, const QPoint& globalPos)
{
	if(!card)
		return;

	// Copy the data: the card may be deleted by a reload while the menu is open
	RsGxsGroupId groupId = card->mGroupId;
	RsGxsMessageId msgId = card->mMsgId;
	QString title = card->mTitle;

	QMenu menu(this);

	QAction *openAct = menu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/channels.png"),
	                                  msgId.isNull() ? tr("Open channel") : tr("Open post"));
	connect(openAct, &QAction::triggered, this, [groupId, msgId]() { openContent(groupId, msgId); });

	QAction *copyAct = menu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"), tr("Copy RetroShare Link"));
	connect(copyAct, &QAction::triggered, this, [groupId, msgId, title]()
	{
		RetroShareLink link = msgId.isNull()
		        ? RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_CHANNEL, groupId, title)
		        : RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_CHANNEL, groupId, msgId, title);

		if(link.valid())
			RSLinkClipboard::copyLinks(QList<RetroShareLink>() << link);
	});

	if(mMode == LatestPosts)
	{
		QAction *readAct = menu.addAction(tr("Mark as read"));
		connect(readAct, &QAction::triggered, this, [groupId, msgId]()
		{
			RsThread::async([groupId, msgId]()
			{
				rsGxsChannels->setMessageReadStatus(RsGxsGrpMsgIdPair(groupId, msgId), true);
			});
		});
	}
	else if(!card->mSubscribed)
	{
		QAction *subAct = menu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/add.png"), tr("Subscribe"));
		connect(subAct, &QAction::triggered, this, [groupId]()
		{
			RsThread::async([groupId]()
			{
				rsGxsChannels->subscribeToChannel(groupId, true);
			});
		});
	}

	menu.exec(globalPos);
}
