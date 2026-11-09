#pragma once

#include <QDialog>

#include "retroshare/rsids.h"
#include "util/rstime.h"

class QTableWidget;
class QPushButton;

class BannedIdentitiesDialog : public QDialog
{
public:
	explicit BannedIdentitiesDialog(QWidget* parent = nullptr);

private:
	void refreshList();
	void unbanSelected();
	void setupUi();
	void setRowData(int row, const RsGxsId& id, bool inLocalDb, const QString& nickname,
	                const QString& bannedSince, const QString& lastSeen);
	QString relativeTimeString(rstime_t ts) const;

	QTableWidget* mTableWidget;
	QPushButton* mRefreshButton;
	QPushButton* mUnbanButton;
	QPushButton* mCloseButton;
};
