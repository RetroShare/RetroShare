#include "BannedIdentitiesDialog.h"

#include "retroshare/rsidentity.h"
#include "retroshare/rsreputations.h"
#include "util/DateTime.h"
#include "util/misc.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <ctime>

namespace
{
enum Columns : int
{
	COLUMN_ID = 0,
	COLUMN_NICKNAME,
	COLUMN_IN_LOCAL_DB,
	COLUMN_BANNED_SINCE,
	COLUMN_LAST_SEEN,
	COLUMN_COUNT
};
}

BannedIdentitiesDialog::BannedIdentitiesDialog(QWidget* parent)
    : QDialog(parent), mTableWidget(nullptr), mRefreshButton(nullptr),
      mUnbanButton(nullptr), mCloseButton(nullptr)
{
	setupUi();
	refreshList();
}

void BannedIdentitiesDialog::setupUi()
{
	setWindowTitle(tr("Banned identities"));
	resize(860, 420);

	auto* mainLayout = new QVBoxLayout(this);

	mTableWidget = new QTableWidget(this);
	mTableWidget->setColumnCount(COLUMN_COUNT);
	mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mTableWidget->setAlternatingRowColors(true);
	mTableWidget->setSortingEnabled(true);
	mTableWidget->setHorizontalHeaderLabels(
	        { tr("Identity ID"), tr("Nickname"), tr("In local DB"),
	          tr("Banned since"), tr("Last seen") });
	mTableWidget->horizontalHeader()->setSectionResizeMode(
	        COLUMN_ID, QHeaderView::Stretch );
	mTableWidget->horizontalHeader()->setSectionResizeMode(
	        COLUMN_NICKNAME, QHeaderView::Stretch );
	mTableWidget->horizontalHeader()->setSectionResizeMode(
	        COLUMN_IN_LOCAL_DB, QHeaderView::ResizeToContents );
	mTableWidget->horizontalHeader()->setSectionResizeMode(
	        COLUMN_BANNED_SINCE, QHeaderView::ResizeToContents );
	mTableWidget->horizontalHeader()->setSectionResizeMode(
	        COLUMN_LAST_SEEN, QHeaderView::ResizeToContents );
	mainLayout->addWidget(mTableWidget);

	auto* buttonsLayout = new QHBoxLayout();
	mRefreshButton = new QPushButton(tr("Refresh"), this);
	mUnbanButton = new QPushButton(tr("Unban selected"), this);
	mCloseButton = new QPushButton(tr("Close"), this);
	buttonsLayout->addWidget(mRefreshButton);
	buttonsLayout->addWidget(mUnbanButton);
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(mCloseButton);
	mainLayout->addLayout(buttonsLayout);

	connect(mRefreshButton, &QPushButton::clicked,
	        this, &BannedIdentitiesDialog::refreshList);
	connect(mUnbanButton, &QPushButton::clicked,
	        this, &BannedIdentitiesDialog::unbanSelected);
	connect(mCloseButton, &QPushButton::clicked, this, &QDialog::accept);
}

void BannedIdentitiesDialog::setRowData(
        int row, const RsGxsId& id, bool inLocalDb, const QString& nickname,
        const QString& bannedSince, const QString& lastSeen )
{
	auto* idItem = new QTableWidgetItem(QString::fromStdString(id.toStdString()));
	idItem->setData(Qt::UserRole, QString::fromStdString(id.toStdString()));
	mTableWidget->setItem(row, COLUMN_ID, idItem);
	mTableWidget->setItem(row, COLUMN_NICKNAME, new QTableWidgetItem(nickname));
	mTableWidget->setItem(
	        row, COLUMN_IN_LOCAL_DB, new QTableWidgetItem(inLocalDb ? tr("Yes") : tr("No")) );
	mTableWidget->setItem(row, COLUMN_BANNED_SINCE, new QTableWidgetItem(bannedSince));
	mTableWidget->setItem(row, COLUMN_LAST_SEEN, new QTableWidgetItem(lastSeen));
}

QString BannedIdentitiesDialog::relativeTimeString(rstime_t ts) const
{
	if(ts == 0) return tr("Unknown");

	const rstime_t now = time(nullptr);
	if(ts > now) return tr("In the future");

	return DateTime::formatLongDateTime(ts)
	        + tr(" (%1 ago)").arg(misc::userFriendlyDuration(now - ts));
}

void BannedIdentitiesDialog::refreshList()
{
	std::vector<RsBannedIdentityInfo> bannedIds;
	rsReputations->getLocallyBannedIdentities(bannedIds);

	mTableWidget->setSortingEnabled(false);
	mTableWidget->setRowCount(static_cast<int>(bannedIds.size()));

	for(size_t i = 0; i < bannedIds.size(); ++i)
	{
		const RsBannedIdentityInfo& info(bannedIds[i]);
		RsIdentityDetails details;
		const bool inLocalDb = rsIdentity->getIdDetails(info.mId, details);
		const QString nickname = inLocalDb
		        ? QString::fromUtf8(details.mNickname.c_str()) : tr("<not in local DB>");

		setRowData(
		        static_cast<int>(i), info.mId, inLocalDb, nickname,
		        relativeTimeString(info.mOwnOpinionTs),
		        relativeTimeString(info.mLastUsedTS) );
	}

	mTableWidget->setSortingEnabled(true);
	mTableWidget->sortByColumn(COLUMN_BANNED_SINCE, Qt::DescendingOrder);
}

void BannedIdentitiesDialog::unbanSelected()
{
	const auto selected = mTableWidget->selectionModel()->selectedRows(COLUMN_ID);
	if(selected.empty())
	{
		QMessageBox::information(
		        this, tr("No selection"),
		        tr("Select one or more identities to unban.") );
		return;
	}

	for(const QModelIndex& idx : selected)
	{
		const QString idStr = mTableWidget->item(idx.row(), COLUMN_ID)->data(Qt::UserRole).toString();
		const RsGxsId id(idStr.toStdString());
		rsReputations->setOwnOpinion(id, RsOpinion::NEUTRAL);
	}

	refreshList();
}
