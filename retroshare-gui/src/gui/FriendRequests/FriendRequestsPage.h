// SPDX-FileCopyrightText: 2024 RetroShare Team
// SPDX-License-Identifier: AGPL-3.0-only
//
// FriendRequestsPage.h
// retroshare-gui/src/gui/FriendRequests/

#pragma once

#include <QWidget>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QListView>
#include <QAbstractListModel>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>

#include "retroshare/rsfriendrequest.h"

// ── Model ──────────────────────────────────────────────────────────────────

class FriendRequestModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum CustomRoles {
        SslIdRole       = Qt::UserRole + 1,
        PgpIdRole,
        PgpNameRole,
        FingerprintRole,
        FirstSeenRole,
        LastSeenRole,
        AttemptCountRole,
        RejectedRole,
    };

    explicit FriendRequestModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void refresh(bool showRejected = false);

private:
    std::vector<RsFriendRequestEntry> mEntries;
};

// ── Delegate ───────────────────────────────────────────────────────────────

class FriendRequestDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FriendRequestDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

signals:
    void acceptClicked(const QString& sslId);
    void rejectClicked(const QString& sslId);

protected:
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

private:
    QRect acceptButtonRect(const QStyleOptionViewItem& option) const;
    QRect rejectButtonRect(const QStyleOptionViewItem& option) const;

    static constexpr int ROW_HEIGHT    = 72;
    static constexpr int BTN_WIDTH     = 80;
    static constexpr int BTN_HEIGHT    = 28;
    static constexpr int AVATAR_SIZE   = 44;
    static constexpr int MARGIN        = 12;
};

// ── Main page widget ───────────────────────────────────────────────────────

/**
 * @brief FriendRequestsPage
 *
 * Drop-in QWidget panel showing all pending friend requests.
 * Place it inside the main window's tab bar or as a docked panel.
 *
 * Usage (added as a sub-tab of the Network tab, see FriendsDialog):
 *   ui.tabWidget->addTab(friendRequestsPage = new FriendRequestsPage(),
 *                        QIcon(IMAGE_FRIENDREQUESTS), tr("Friend Requests"));
 *   // reflect the pending count in the tab label:
 *   connect(friendRequestsPage, &FriendRequestsPage::pendingCountChanged,
 *           this, [this](int count){ ... ui.tabWidget->setTabText(idx, ...); });
 */
class FriendRequestsPage : public QWidget
{
    Q_OBJECT
public:
    explicit FriendRequestsPage(QWidget* parent = nullptr);
    ~FriendRequestsPage() override = default;

signals:
    /// Emitted whenever the pending (non-rejected) count changes.
    /// Connect this to the tab badge / notification dot.
    void pendingCountChanged(int count);

public slots:
    void refresh();

private slots:
    void onAccept(const QString& sslId);
    void onReject(const QString& sslId);
    void onShowRejectedToggled(bool checked);
    void onSearchTextChanged(const QString& text);
    void onClearRejected();

private:
    void buildUi();
    void applyFilter(const QString& text);

    // ── Widgets ──────────────────────────────────────────────────────────
    QLabel*         mTitleLabel       = nullptr;
    QLabel*         mBadgeLabel       = nullptr;  // round count badge
    QLineEdit*      mSearchEdit       = nullptr;
    QCheckBox*      mShowRejectedBox  = nullptr;
    QPushButton*    mClearRejectedBtn = nullptr;
    QListView*      mListView         = nullptr;
    QLabel*         mEmptyLabel       = nullptr;  // "No pending requests" placeholder

    FriendRequestModel*   mModel      = nullptr;
    FriendRequestDelegate* mDelegate  = nullptr;

    QTimer*         mRefreshTimer     = nullptr;
    int             mLastCount        = -1;       // for change detection

    static constexpr int REFRESH_INTERVAL_MS = 5000;
};
