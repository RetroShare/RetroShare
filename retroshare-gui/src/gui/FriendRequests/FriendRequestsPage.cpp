// SPDX-FileCopyrightText: 2024 RetroShare Team
// SPDX-License-Identifier: AGPL-3.0-only
//
// FriendRequestsPage.cpp

#include "FriendRequestsPage.h"

#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QVBoxLayout>

#include "retroshare/rsfriendrequest.h"
#include "retroshare/rspeers.h"

// ── Palette ────────────────────────────────────────────────────────────────

static const QColor kAccentGreen ("#27ae60");
static const QColor kAccentRed   ("#e74c3c");
static const QColor kBgCard      ("#ffffff");
static const QColor kBgHover     ("#f4f6f8");
static const QColor kTextPrimary ("#2c3e50");
static const QColor kTextSub     ("#7f8c8d");
static const QColor kBorder      ("#e0e4e8");

// ══════════════════════════════════════════════════════════════════════════
// FriendRequestModel
// ══════════════════════════════════════════════════════════════════════════

FriendRequestModel::FriendRequestModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int FriendRequestModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(mEntries.size());
}

QVariant FriendRequestModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= (int)mEntries.size())
        return {};

    const auto& e = mEntries[static_cast<size_t>(index.row())];

    switch (role)
    {
    case Qt::DisplayRole:   // fallthrough — display = name
    case PgpNameRole:
        return QString::fromStdString(e.pgpName);
    case SslIdRole:
        // Store as hex string — RsPeerId::toHex() is the stable round-trip format.
        return QString::fromStdString(e.sslId.toStdString());
    case PgpIdRole:
        return QString::fromStdString(e.pgpId.toStdString());
    case FingerprintRole:
        return QString::fromStdString(e.pgpFingerprint);
    case FirstSeenRole:
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(e.firstSeen));
    case LastSeenRole:
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(e.lastSeen));
    case AttemptCountRole:
        return static_cast<int>(e.attemptCount);
    case RejectedRole:
        return e.rejected;
    default:
        return {};
    }
}

void FriendRequestModel::refresh(bool showRejected)
{
    beginResetModel();
    mEntries.clear();

    // [FIX audit-4] Guard against null rsFriendRequest (startup race).
    if (rsFriendRequest)
    {
        std::list<RsFriendRequestEntry> lst;
        if (showRejected)
            rsFriendRequest->getAllRequests(lst);
        else
            rsFriendRequest->getPendingRequests(lst);

        mEntries.assign(lst.begin(), lst.end());
    }

    endResetModel();
}

// ══════════════════════════════════════════════════════════════════════════
// FriendRequestDelegate
// ══════════════════════════════════════════════════════════════════════════

FriendRequestDelegate::FriendRequestDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

QSize FriendRequestDelegate::sizeHint(const QStyleOptionViewItem&,
                                       const QModelIndex&) const
{
    return QSize(0, ROW_HEIGHT);
}

QRect FriendRequestDelegate::acceptButtonRect(
        const QStyleOptionViewItem& opt) const
{
    // [FIX audit-5] Leave a MARGIN gap between the two buttons.
    // Layout (right-to-left): |MARGIN|rejectBtn|MARGIN|acceptBtn|MARGIN|
    int x = opt.rect.right() - 2 * BTN_WIDTH - 3 * MARGIN;
    int y = opt.rect.top() + (ROW_HEIGHT - BTN_HEIGHT) / 2;
    return QRect(x, y, BTN_WIDTH, BTN_HEIGHT);
}

QRect FriendRequestDelegate::rejectButtonRect(
        const QStyleOptionViewItem& opt) const
{
    int x = opt.rect.right() - BTN_WIDTH - MARGIN;
    int y = opt.rect.top() + (ROW_HEIGHT - BTN_HEIGHT) / 2;
    return QRect(x, y, BTN_WIDTH, BTN_HEIGHT);
}

void FriendRequestDelegate::paint(QPainter*                  painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex&          index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool hovered  = option.state & QStyle::State_MouseOver;
    const bool rejected = index.data(FriendRequestModel::RejectedRole).toBool();
    const QRect& r      = option.rect;

    // ── Background ──────────────────────────────────────────────────────
    QColor bg = hovered ? kBgHover : kBgCard;
    if (rejected) bg = QColor("#fdf5f5");
    painter->fillRect(r, bg);

    // Bottom separator
    painter->setPen(QPen(kBorder, 1));
    painter->drawLine(r.left() + MARGIN, r.bottom(),
                      r.right() - MARGIN, r.bottom());

    // ── Avatar circle ────────────────────────────────────────────────────
    // [FIX audit-4] AvatarDefs::getAvatar() expects an RsGxsId, not an
    // RsPgpId string. For a friend-request scenario we don't have a GXS id,
    // so we always use the initials fallback. This is safe and correct.
    QRect avatarRect(r.left() + MARGIN,
                     r.top() + (ROW_HEIGHT - AVATAR_SIZE) / 2,
                     AVATAR_SIZE, AVATAR_SIZE);

    {
        QString name     = index.data(FriendRequestModel::PgpNameRole).toString();
        QString initials = name.isEmpty() ? QStringLiteral("?")
                                          : name.left(1).toUpper();

        painter->setBrush(QColor("#3498db"));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(avatarRect);

        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        f.setPointSize(14);
        painter->setFont(f);
        painter->drawText(avatarRect, Qt::AlignCenter, initials);
    }

    // Red ring overlay for rejected entries
    if (rejected)
    {
        painter->setPen(QPen(kAccentRed.lighter(160), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(avatarRect.adjusted(-1, -1, 1, 1));
    }

    // ── Text block ───────────────────────────────────────────────────────
    int textX = avatarRect.right() + MARGIN;
    // [FIX audit-5] textW must reference the corrected acceptButtonRect.
    int textW = acceptButtonRect(option).left() - textX - MARGIN;

    // Name row
    QFont nameFont = painter->font();
    nameFont.setBold(true);
    nameFont.setPointSize(10);
    painter->setFont(nameFont);
    painter->setPen(rejected ? kTextSub : kTextPrimary);

    QRect nameRect(textX, r.top() + 12, textW, 20);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                      index.data(FriendRequestModel::PgpNameRole).toString());

    // Fingerprint row (elided)
    QFont subFont = painter->font();
    subFont.setBold(false);
    subFont.setPointSize(8);
    painter->setFont(subFont);
    painter->setPen(kTextSub);

    QString fp = index.data(FriendRequestModel::FingerprintRole).toString();
    if (fp.length() > 20)
        fp = fp.left(4) + QStringLiteral(" … ") + fp.right(8);

    QRect fpRect(textX, nameRect.bottom() + 2, textW, 16);
    painter->drawText(fpRect, Qt::AlignLeft | Qt::AlignVCenter, fp);

    // Meta row: last seen + attempt count
    QDateTime lastSeen = index.data(FriendRequestModel::LastSeenRole).toDateTime();
    int attempts       = index.data(FriendRequestModel::AttemptCountRole).toInt();

    // tr() with %n handles plural ("1 attempt" vs "3 attempts").
    QString meta = FriendRequestsPage::tr("Last seen: %1  ·  %n attempt(s)", "", attempts)
                   .arg(lastSeen.toString(QStringLiteral("dd MMM yyyy hh:mm")));

    QRect metaRect(textX, fpRect.bottom() + 2, textW, 14);
    painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter, meta);

    // ── Buttons (pending rows only) ──────────────────────────────────────
    if (!rejected)
    {
        auto drawBtn = [&](const QRect& br, const QString& label, const QColor& color)
        {
            QPainterPath path;
            path.addRoundedRect(br, 5, 5);
            painter->fillPath(path, color);
            painter->setPen(Qt::white);
            QFont btnFont = painter->font();
            btnFont.setBold(true);
            btnFont.setPointSize(8);
            painter->setFont(btnFont);
            painter->drawText(br, Qt::AlignCenter, label);
        };

        drawBtn(acceptButtonRect(option), FriendRequestsPage::tr("Accept"), kAccentGreen);
        drawBtn(rejectButtonRect(option), FriendRequestsPage::tr("Reject"), kAccentRed);
    }
    else
    {
        // "Rejected" pill badge
        QRect badgeRect(r.right() - 84 - MARGIN,
                        r.top() + (ROW_HEIGHT - 20) / 2, 84, 20);
        QPainterPath p;
        p.addRoundedRect(badgeRect, 10, 10);
        painter->fillPath(p, QColor("#fde8e8"));
        painter->setPen(kAccentRed);
        QFont badgeFont = painter->font();
        badgeFont.setPointSize(8);
        painter->setFont(badgeFont);
        painter->drawText(badgeRect, Qt::AlignCenter,
                          FriendRequestsPage::tr("Rejected"));
    }

    painter->restore();
}

bool FriendRequestDelegate::editorEvent(QEvent*                     event,
                                         QAbstractItemModel*         /*model*/,
                                         const QStyleOptionViewItem& option,
                                         const QModelIndex&          index)
{
    if (event->type() != QEvent::MouseButtonRelease)   return false;
    if (index.data(FriendRequestModel::RejectedRole).toBool()) return false;

    auto* me = static_cast<QMouseEvent*>(event);
    // me->pos() and option.rect are both in viewport coordinates — correct.
    QString sslId = index.data(FriendRequestModel::SslIdRole).toString();

    if (acceptButtonRect(option).contains(me->pos()))
    {
        emit acceptClicked(sslId);
        return true;
    }
    if (rejectButtonRect(option).contains(me->pos()))
    {
        emit rejectClicked(sslId);
        return true;
    }
    return false;
}

// ══════════════════════════════════════════════════════════════════════════
// FriendRequestsPage
// ══════════════════════════════════════════════════════════════════════════

FriendRequestsPage::FriendRequestsPage(QWidget* parent)
    : QWidget(parent)
{
    buildUi();

    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(REFRESH_INTERVAL_MS);
    connect(mRefreshTimer, &QTimer::timeout,
            this,          &FriendRequestsPage::refresh);
    mRefreshTimer->start();

    refresh();
}

void FriendRequestsPage::buildUi()
{
    setObjectName(QStringLiteral("FriendRequestsPage"));
    setStyleSheet(QStringLiteral(R"(
        #FriendRequestsPage {
            background: #f0f2f5;
        }
        QLineEdit#searchEdit {
            border: 1px solid #dde1e7;
            border-radius: 6px;
            padding: 6px 10px;
            background: white;
            font-size: 13px;
        }
        QListView {
            border: none;
            background: transparent;
        }
        QCheckBox {
            font-size: 12px;
            color: #555;
        }
        QPushButton#clearBtn {
            border: 1px solid #e74c3c;
            border-radius: 4px;
            color: #e74c3c;
            background: transparent;
            padding: 4px 10px;
            font-size: 11px;
        }
        QPushButton#clearBtn:hover {
            background: #fde8e8;
        }
    )"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header bar ───────────────────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setFixedHeight(60);
    header->setStyleSheet(
        QStringLiteral("background: white; border-bottom: 1px solid #e0e4e8;"));

    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(16, 0, 16, 0);

    mTitleLabel = new QLabel(tr("Friend Requests"), header);
    QFont titleFont = mTitleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(13);
    mTitleLabel->setFont(titleFont);
    mTitleLabel->setStyleSheet(QStringLiteral("color: #2c3e50;"));

    mBadgeLabel = new QLabel(header);
    mBadgeLabel->setFixedSize(22, 22);
    mBadgeLabel->setAlignment(Qt::AlignCenter);
    mBadgeLabel->setStyleSheet(QStringLiteral(
        "background: #e74c3c; color: white; border-radius: 11px;"
        "font-size: 11px; font-weight: bold;"));
    mBadgeLabel->hide();

    hLayout->addWidget(mTitleLabel);
    hLayout->addWidget(mBadgeLabel);
    hLayout->addStretch();
    root->addWidget(header);

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(48);
    toolbar->setStyleSheet(
        QStringLiteral("background: #f8f9fa; border-bottom: 1px solid #e0e4e8;"));

    auto* tLayout = new QHBoxLayout(toolbar);
    tLayout->setContentsMargins(12, 8, 12, 8);
    tLayout->setSpacing(10);

    mSearchEdit = new QLineEdit(toolbar);
    mSearchEdit->setObjectName(QStringLiteral("searchEdit"));
    mSearchEdit->setPlaceholderText(tr("Search by name or fingerprint…"));
    mSearchEdit->setClearButtonEnabled(true);
    connect(mSearchEdit, &QLineEdit::textChanged,
            this,        &FriendRequestsPage::onSearchTextChanged);

    mShowRejectedBox = new QCheckBox(tr("Show rejected"), toolbar);
    connect(mShowRejectedBox, &QCheckBox::toggled,
            this,             &FriendRequestsPage::onShowRejectedToggled);

    mClearRejectedBtn = new QPushButton(tr("Clear rejected"), toolbar);
    mClearRejectedBtn->setObjectName(QStringLiteral("clearBtn"));
    connect(mClearRejectedBtn, &QPushButton::clicked,
            this,              &FriendRequestsPage::onClearRejected);

    tLayout->addWidget(mSearchEdit, 1);
    tLayout->addWidget(mShowRejectedBox);
    tLayout->addWidget(mClearRejectedBtn);
    root->addWidget(toolbar);

    // ── List ─────────────────────────────────────────────────────────────
    auto* listContainer = new QWidget(this);
    auto* listLayout    = new QVBoxLayout(listContainer);
    listLayout->setContentsMargins(8, 8, 8, 8);
    listLayout->setSpacing(0);

    mModel    = new FriendRequestModel(this);
    mDelegate = new FriendRequestDelegate(this);

    mListView = new QListView(listContainer);
    mListView->setModel(mModel);
    mListView->setItemDelegate(mDelegate);
    mListView->setMouseTracking(true);
    mListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    mListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mListView->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    connect(mDelegate, &FriendRequestDelegate::acceptClicked,
            this,      &FriendRequestsPage::onAccept);
    connect(mDelegate, &FriendRequestDelegate::rejectClicked,
            this,      &FriendRequestsPage::onReject);

    mEmptyLabel = new QLabel(tr("No pending friend requests"), listContainer);
    mEmptyLabel->setAlignment(Qt::AlignCenter);
    mEmptyLabel->setStyleSheet(
        QStringLiteral("color: #aaa; font-size: 14px; padding: 40px;"));
    mEmptyLabel->hide();

    listLayout->addWidget(mListView);
    listLayout->addWidget(mEmptyLabel);
    root->addWidget(listContainer, 1);
}

// ── Slots ──────────────────────────────────────────────────────────────────

void FriendRequestsPage::refresh()
{
    // [FIX audit-4] Guard: rsFriendRequest may be null during startup.
    if (!rsFriendRequest) return;

    const bool showRejected = mShowRejectedBox && mShowRejectedBox->isChecked();
    mModel->refresh(showRejected);

    const int  count = static_cast<int>(rsFriendRequest->pendingCount());
    const bool empty = (mModel->rowCount() == 0);
    mListView->setVisible(!empty);
    mEmptyLabel->setVisible(empty);

    if (count > 0)
    {
        mBadgeLabel->setText(count > 99 ? QStringLiteral("99+")
                                        : QString::number(count));
        mBadgeLabel->show();
    }
    else
    {
        mBadgeLabel->hide();
    }

    if (count != mLastCount)
    {
        mLastCount = count;
        emit pendingCountChanged(count);
    }

    // Re-apply any active search filter after data reload.
    if (mSearchEdit && !mSearchEdit->text().isEmpty())
        applyFilter(mSearchEdit->text());
}

void FriendRequestsPage::onAccept(const QString& sslId)
{
    if (!rsFriendRequest) return;
    // RsPeerId constructed from the same toStdString() hex representation
    // stored in SslIdRole. The round-trip is stable.
    RsPeerId id(sslId.toStdString());
    rsFriendRequest->acceptRequest(id);
    refresh();
}

void FriendRequestsPage::onReject(const QString& sslId)
{
    if (!rsFriendRequest) return;
    RsPeerId id(sslId.toStdString());
    rsFriendRequest->rejectRequest(id);
    refresh();
}

void FriendRequestsPage::onShowRejectedToggled(bool /*checked*/)
{
    refresh();
}

void FriendRequestsPage::onSearchTextChanged(const QString& text)
{
    applyFilter(text);
}

void FriendRequestsPage::applyFilter(const QString& text)
{
    for (int row = 0; row < mModel->rowCount(); ++row)
    {
        const QModelIndex idx = mModel->index(row);
        const bool match =
            text.isEmpty()
            || idx.data(FriendRequestModel::PgpNameRole)
                   .toString().contains(text, Qt::CaseInsensitive)
            || idx.data(FriendRequestModel::FingerprintRole)
                   .toString().contains(text, Qt::CaseInsensitive);
        mListView->setRowHidden(row, !match);
    }
}

void FriendRequestsPage::onClearRejected()
{
    if (!rsFriendRequest) return;
    rsFriendRequest->clearRejected();
    refresh();
}
