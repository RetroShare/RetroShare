/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarWidget.cpp                              *
 *                                                                             *
 * Copyright (C) 2011 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "gui/msgs/CalendarWidget.h"
#include "gui/msgs/EventDialog.h"
#include "gui/msgs/CalendarPropertiesDialog.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QCalendarWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QUuid>

CalendarWidget::CalendarWidget(QWidget* parent)
    : QWidget(parent), mSelectedDate(QDate::currentDate()), mCurrentViewMode(2) // Default to Month View
{
    buildUi();
    refreshData();
}

CalendarWidget::~CalendarWidget() {}

void CalendarWidget::buildUi() {
    ui.setupUi(this);

    // Initialize UI pointers
    mSidebarCalendar = ui.sidebarCalendar;
    mCalendarList = ui.calendarList;
    mPeriodLabel = ui.periodLabel;
    mSearchEdit = ui.searchEdit;
    mEventTable = ui.eventTable;
    mViewStack = ui.viewStack;
    mDayTable = ui.dayTable;
    mWeekTable = ui.weekTable;
    mMonthTable = ui.monthTable;

    // Sidebar Calendar configs
    mSidebarCalendar->setSelectedDate(mSelectedDate);

    // Event table configs
    mEventTable->setColumnCount(5);
    mEventTable->setHorizontalHeaderLabels({tr("Title"), tr("Start"), tr("End"), tr("Category"), tr("Calendar")});
    mEventTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mEventTable->verticalHeader()->setVisible(false);
    mEventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mEventTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mEventTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mEventTable->setMaximumHeight(120);
    connect(mEventTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onEventSelected(int,int)));

    // Stacked widget pages setup
    // 1. Day Table
    mDayTable->setColumnCount(2);
    mDayTable->setHorizontalHeaderLabels({tr("Time"), tr("Events")});
    mDayTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    mDayTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mDayTable->horizontalHeader()->resizeSection(0, 80);
    mDayTable->verticalHeader()->setVisible(false);
    mDayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(mDayTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onEventSelected(int,int)));

    // 2. Week Table
    mWeekTable->setColumnCount(7);
    mWeekTable->setHorizontalHeaderLabels({tr("Mon"), tr("Tue"), tr("Wed"), tr("Thu"), tr("Fri"), tr("Sat"), tr("Sun")});
    mWeekTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mWeekTable->verticalHeader()->setVisible(false);
    mWeekTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(mWeekTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onEventSelected(int,int)));

    // 3. Month Table
    mMonthTable->setColumnCount(7);
    mMonthTable->setHorizontalHeaderLabels({tr("Mon"), tr("Tue"), tr("Wed"), tr("Thu"), tr("Fri"), tr("Sat"), tr("Sun")});
    mMonthTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mMonthTable->verticalHeader()->setVisible(false);
    mMonthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(mMonthTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onEventSelected(int,int)));

    mViewStack->setCurrentIndex(mCurrentViewMode);

    // Set splitter sizes or stretch factors
    ui.splitter->setStretchFactor(0, 0);
    ui.splitter->setStretchFactor(1, 1);

    // Connect sidebar signals
    connect(ui.newEventBtn, SIGNAL(clicked()), this, SLOT(onNewEvent()));
    connect(mSidebarCalendar, SIGNAL(clicked(const QDate&)), this, SLOT(onDateSelected(const QDate&)));
    connect(mCalendarList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onCalendarSelectionChanged(QListWidgetItem*)));
    connect(mCalendarList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onCalendarContextMenu(const QPoint&)));
    connect(ui.newCalBtn, SIGNAL(clicked()), this, SLOT(onNewCalendar()));

    // Connect top control signals
    connect(ui.prevBtn, SIGNAL(clicked()), this, SLOT(onPrevPeriod()));
    connect(ui.todayBtn, SIGNAL(clicked()), this, SLOT(onToday()));
    connect(ui.nextBtn, SIGNAL(clicked()), this, SLOT(onNextPeriod()));
    connect(mSearchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onSearchChanged(const QString&)));

    // View selector buttons
    connect(ui.dayViewBtn, &QPushButton::clicked, [this]() {
        ui.dayViewBtn->setChecked(true); ui.weekViewBtn->setChecked(false); ui.monthViewBtn->setChecked(false);
        onViewChanged(0);
    });
    connect(ui.weekViewBtn, &QPushButton::clicked, [this]() {
        ui.dayViewBtn->setChecked(false); ui.weekViewBtn->setChecked(true); ui.monthViewBtn->setChecked(false);
        onViewChanged(1);
    });
    connect(ui.monthViewBtn, &QPushButton::clicked, [this]() {
        ui.dayViewBtn->setChecked(false); ui.weekViewBtn->setChecked(false); ui.monthViewBtn->setChecked(true);
        onViewChanged(2);
    });
}

void CalendarWidget::refreshData() {
    // Save current check states
    QMap<QString, Qt::CheckState> checkedStates;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        checkedStates[item->data(Qt::UserRole).toString()] = item->checkState();
    }

    // Populate Calendar selection list
    mCalendarList->blockSignals(true);
    mCalendarList->clear();
    const auto& cals = CalendarData::instance()->getCalendars();
    for (const auto& cal : cals) {
        QListWidgetItem* item = new QListWidgetItem(cal.name, mCalendarList);
        item->setData(Qt::UserRole, cal.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        
        // Render colored bullet point icon
        QPixmap pix(12, 12);
        pix.fill(cal.color);
        item->setIcon(QIcon(pix));

        // Restore checked state
        if (checkedStates.contains(cal.id)) {
            item->setCheckState(checkedStates[cal.id]);
        } else {
            item->setCheckState(Qt::Checked);
        }
    }
    mCalendarList->blockSignals(false);

    updateViews();
}

void CalendarWidget::updateViews() {
    mCellEventMap.clear();

    // 1. Update the Period Label
    if (mCurrentViewMode == 0) { // Day View
        mPeriodLabel->setText(mSelectedDate.toString("dd MMMM yyyy"));
    } else if (mCurrentViewMode == 1) { // Week View
        QDate monday = mSelectedDate.addDays(-(mSelectedDate.dayOfWeek() - 1));
        QDate sunday = monday.addDays(6);
        if (monday.month() == sunday.month()) {
            mPeriodLabel->setText(monday.toString("dd") + " - " + sunday.toString("dd") + " " + monday.toString("MMMM yyyy"));
        } else {
            mPeriodLabel->setText(monday.toString("dd MMM") + " - " + sunday.toString("dd MMM") + " " + sunday.toString("yyyy"));
        }
    } else { // Month View
        mPeriodLabel->setText(mSelectedDate.toString("MMMM yyyy"));
    }

    // 2. Load and Filter Active Events
    updateEventList();

    // 3. Render Stacked Calendar Views
    if (mCurrentViewMode == 0) {
        updateDayView();
    } else if (mCurrentViewMode == 1) {
        updateWeekView();
    } else {
        updateMonthView();
    }
}

void CalendarWidget::updateEventList() {
    mEventTable->setRowCount(0);

    const auto& events = CalendarData::instance()->getEvents();
    const auto& cals = CalendarData::instance()->getCalendars();

    // Get enabled calendar IDs
    QStringList enabledCalIds;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) {
            enabledCalIds.append(item->data(Qt::UserRole).toString());
        }
    }

    int row = 0;
    for (const auto& ev : events) {
        if (!enabledCalIds.contains(ev.calendarId)) continue;
        
        // Search text filter
        if (!mSearchText.isEmpty() && !ev.title.contains(mSearchText, Qt::CaseInsensitive) && 
            !ev.description.contains(mSearchText, Qt::CaseInsensitive)) {
            continue;
        }

        mEventTable->insertRow(row);
        
        QTableWidgetItem* titleItem = new QTableWidgetItem(ev.title);
        titleItem->setData(Qt::UserRole, ev.id);
        mEventTable->setItem(row, 0, titleItem);
        
        mEventTable->setItem(row, 1, new QTableWidgetItem(ev.start.toString("yyyy-MM-dd hh:mm")));
        mEventTable->setItem(row, 2, new QTableWidgetItem(ev.end.toString("yyyy-MM-dd hh:mm")));
        mEventTable->setItem(row, 3, new QTableWidgetItem(ev.category));

        // Get calendar name
        QString calName = "";
        for (const auto& c : cals) {
            if (c.id == ev.calendarId) {
                calName = c.name;
                break;
            }
        }
        mEventTable->setItem(row, 4, new QTableWidgetItem(calName));
        row++;
    }
}

void CalendarWidget::updateDayView() {
    mDayTable->setRowCount(0);
    mDayTable->setRowCount(24);

    // List of events for the selected day
    const auto& events = CalendarData::instance()->getEvents();

    QStringList enabledCalIds;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) enabledCalIds.append(item->data(Qt::UserRole).toString());
    }

    for (int hour = 0; hour < 24; ++hour) {
        QString timeText = QString("%1:00").arg(hour, 2, 10, QChar('0'));
        mDayTable->setItem(hour, 0, new QTableWidgetItem(timeText));

        // Match events starting or active during this hour
        QStringList matchedEvents;
        QString lastEventId = "";
        for (const auto& ev : events) {
            if (!enabledCalIds.contains(ev.calendarId)) continue;
            if (ev.start.date() == mSelectedDate && ev.start.time().hour() == hour) {
                matchedEvents.append(ev.title);
                lastEventId = ev.id;
            }
        }

        QTableWidgetItem* evCell = new QTableWidgetItem(matchedEvents.join(", "));
        if (!lastEventId.isEmpty()) {
            mCellEventMap[QString("0_%1_%2").arg(hour).arg(1)] = lastEventId;
            evCell->setBackground(QBrush(QColor("#eef5fc")));
        }
        mDayTable->setItem(hour, 1, evCell);
    }
}

void CalendarWidget::updateWeekView() {
    mWeekTable->setRowCount(0);
    mWeekTable->setRowCount(8); // Max events rows per week

    // Get current Monday
    QDate monday = mSelectedDate.addDays(-(mSelectedDate.dayOfWeek() - 1));

    // Update column headers with dates
    QStringList headers;
    for (int i = 0; i < 7; ++i) {
        headers << monday.addDays(i).toString("ddd dd/MM");
    }
    mWeekTable->setHorizontalHeaderLabels(headers);

    const auto& events = CalendarData::instance()->getEvents();

    QStringList enabledCalIds;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) enabledCalIds.append(item->data(Qt::UserRole).toString());
    }

    // Populate week cells
    for (int dayIdx = 0; dayIdx < 7; ++dayIdx) {
        QDate date = monday.addDays(dayIdx);
        int rowIdx = 0;
        for (const auto& ev : events) {
            if (!enabledCalIds.contains(ev.calendarId)) continue;
            if (ev.start.date() == date) {
                if (rowIdx >= mWeekTable->rowCount()) mWeekTable->insertRow(rowIdx);
                QTableWidgetItem* cellItem = new QTableWidgetItem(ev.title);
                cellItem->setBackground(QBrush(QColor("#eef5fc")));
                mWeekTable->setItem(rowIdx, dayIdx, cellItem);
                mCellEventMap[QString("1_%1_%2").arg(rowIdx).arg(dayIdx)] = ev.id;
                rowIdx++;
            }
        }
    }
}

void CalendarWidget::updateMonthView() {
    mMonthTable->setRowCount(6); // A month calendar grid needs up to 6 rows

    // Find first day of the month
    QDate firstOfMonth(mSelectedDate.year(), mSelectedDate.month(), 1);
    int startDayOfWeek = firstOfMonth.dayOfWeek(); // 1=Mon, 7=Sun
    QDate startDate = firstOfMonth.addDays(-(startDayOfWeek - 1));

    const auto& events = CalendarData::instance()->getEvents();
    
    QStringList enabledCalIds;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) enabledCalIds.append(item->data(Qt::UserRole).toString());
    }

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            QDate date = startDate.addDays(row * 7 + col);
            
            // Build cell contents: "Date \n Event1 \n Event2..."
            QStringList cellLines;
            cellLines << QString::number(date.day());

            QString matchedEventId = "";
            for (const auto& ev : events) {
                if (!enabledCalIds.contains(ev.calendarId)) continue;
                if (ev.start.date() == date) {
                    cellLines << ev.title;
                    matchedEventId = ev.id;
                }
            }

            QTableWidgetItem* cellItem = new QTableWidgetItem(cellLines.join("\n"));
            if (date.month() != mSelectedDate.month()) {
                cellItem->setForeground(QBrush(Qt::gray));
            }
            if (!matchedEventId.isEmpty()) {
                cellItem->setBackground(QBrush(QColor("#eef5fc")));
                mCellEventMap[QString("2_%1_%2").arg(row).arg(col)] = matchedEventId;
            }
            mMonthTable->setItem(row, col, cellItem);
        }
    }

    // Set row heights to expand nicely in the month grid
    for (int row = 0; row < 6; ++row) {
        mMonthTable->setRowHeight(row, 60);
    }
}

void CalendarWidget::onDateSelected(const QDate& date) {
    mSelectedDate = date;
    updateViews();
}

void CalendarWidget::onPrevPeriod() {
    if (mCurrentViewMode == 0) { // Day
        mSelectedDate = mSelectedDate.addDays(-1);
    } else if (mCurrentViewMode == 1) { // Week
        mSelectedDate = mSelectedDate.addDays(-7);
    } else { // Month
        mSelectedDate = mSelectedDate.addMonths(-1);
    }
    mSidebarCalendar->setSelectedDate(mSelectedDate);
    updateViews();
}

void CalendarWidget::onNextPeriod() {
    if (mCurrentViewMode == 0) { // Day
        mSelectedDate = mSelectedDate.addDays(1);
    } else if (mCurrentViewMode == 1) { // Week
        mSelectedDate = mSelectedDate.addDays(7);
    } else { // Month
        mSelectedDate = mSelectedDate.addMonths(1);
    }
    mSidebarCalendar->setSelectedDate(mSelectedDate);
    updateViews();
}

void CalendarWidget::onToday() {
    mSelectedDate = QDate::currentDate();
    mSidebarCalendar->setSelectedDate(mSelectedDate);
    updateViews();
}

void CalendarWidget::onViewChanged(int index) {
    mCurrentViewMode = index;
    mViewStack->setCurrentIndex(mCurrentViewMode);
    updateViews();
}

void CalendarWidget::onNewEvent() {
    // Open Dialog
    QDateTime defaultStart(mSelectedDate, QTime(QTime::currentTime().hour(), 0));
    EventDialog dlg("", defaultStart, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshData();
    }
}

void CalendarWidget::onNewCalendar() {
    CalendarPropertiesDialog dlg("", this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshData();
    }
}

void CalendarWidget::onEventSelected(int row, int col) {
    QObject* senderObj = sender();
    QString eventId = "";

    if (senderObj == mEventTable) {
        QTableWidgetItem* titleItem = mEventTable->item(row, 0);
        if (titleItem) eventId = titleItem->data(Qt::UserRole).toString();
    } else {
        QString key = QString("%1_%2_%3").arg(mCurrentViewMode).arg(row).arg(col);
        if (mCellEventMap.contains(key)) {
            eventId = mCellEventMap[key];
        }
    }

    // If double clicked a cell/row containing an event, edit it. Otherwise create a new one.
    if (!eventId.isEmpty()) {
        EventDialog dlg(eventId, QDateTime::currentDateTime(), this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshData();
        }
    } else {
        // Create new event on the clicked cell's date
        QDateTime startDateTime = QDateTime::currentDateTime();
        if (mCurrentViewMode == 1) { // Week View
            QDate monday = mSelectedDate.addDays(-(mSelectedDate.dayOfWeek() - 1));
            startDateTime.setDate(monday.addDays(col));
        } else if (mCurrentViewMode == 2) { // Month View
            QDate firstOfMonth(mSelectedDate.year(), mSelectedDate.month(), 1);
            QDate startDate = firstOfMonth.addDays(-(firstOfMonth.dayOfWeek() - 1));
            startDateTime.setDate(startDate.addDays(row * 7 + col));
        } else if (mCurrentViewMode == 0) { // Day View
            startDateTime.setDate(mSelectedDate);
            startDateTime.setTime(QTime(row, 0));
        }
        EventDialog dlg("", startDateTime, this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshData();
        }
    }
}

void CalendarWidget::onCalendarSelectionChanged(QListWidgetItem* /*item*/) {
    updateViews();
}

void CalendarWidget::onSearchChanged(const QString& text) {
    mSearchText = text.trimmed();
    updateEventList();
}

void CalendarWidget::onCalendarContextMenu(const QPoint& pos) {
    QListWidgetItem* item = mCalendarList->itemAt(pos);
    if (!item) return;

    QString calId = item->data(Qt::UserRole).toString();
    QString calName = item->text();
    bool isChecked = item->checkState() == Qt::Checked;

    QMenu menu(this);

    QAction* toggleAct = menu.addAction(isChecked ? tr("Hide %1").arg(calName) : tr("Show %1").arg(calName));
    QAction* showOnlyAct = menu.addAction(tr("Show Only %1").arg(calName));
    QAction* showAllAct = menu.addAction(tr("Show All Calendars"));
    menu.addSeparator();
    QAction* newAct = menu.addAction(tr("New Calendar..."));
    QAction* deleteAct = menu.addAction(tr("Delete Calendar..."));
    menu.addSeparator();
    QAction* exportAct = menu.addAction(tr("Export Calendar..."));
    QAction* publishAct = menu.addAction(tr("Publish Calendar..."));
    menu.addSeparator();
    QAction* propertiesAct = menu.addAction(tr("Properties"));

    QAction* selectedAct = menu.exec(mCalendarList->mapToGlobal(pos));
    if (!selectedAct) return;

    if (selectedAct == toggleAct) {
        item->setCheckState(isChecked ? Qt::Unchecked : Qt::Checked);
    } else if (selectedAct == showOnlyAct) {
        mCalendarList->blockSignals(true);
        for (int i = 0; i < mCalendarList->count(); ++i) {
            QListWidgetItem* it = mCalendarList->item(i);
            it->setCheckState(it == item ? Qt::Checked : Qt::Unchecked);
        }
        mCalendarList->blockSignals(false);
        updateViews();
    } else if (selectedAct == showAllAct) {
        mCalendarList->blockSignals(true);
        for (int i = 0; i < mCalendarList->count(); ++i) {
            mCalendarList->item(i)->setCheckState(Qt::Checked);
        }
        mCalendarList->blockSignals(false);
        updateViews();
    } else if (selectedAct == newAct) {
        onNewCalendar();
    } else if (selectedAct == deleteAct) {
        if (QMessageBox::question(this, tr("Delete Calendar"),
            tr("Are you sure you want to delete calendar '%1'?\nThis will also delete all associated events and tasks.").arg(calName),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            CalendarData::instance()->removeCalendar(calId);
            refreshData();
        }
    } else if (selectedAct == exportAct) {
        QMessageBox::information(this, tr("Export Calendar"), tr("Calendar '%1' exported successfully!").arg(calName));
    } else if (selectedAct == publishAct) {
        QMessageBox::information(this, tr("Publish Calendar"), tr("Calendar '%1' published successfully!").arg(calName));
    } else if (selectedAct == propertiesAct) {
        CalendarPropertiesDialog dlg(calId, this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshData();
        }
    }
}
