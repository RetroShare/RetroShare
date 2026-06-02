/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarWidget.h                                *
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

#ifndef CALENDARWIDGET_H
#define CALENDARWIDGET_H

#include <QWidget>
#include <QDate>
#include <QMap>
#include "gui/msgs/CalendarData.h"
#include "ui_CalendarWidget.h"

class QListWidgetItem;

class CalendarWidget : public QWidget {
    Q_OBJECT
public:
    CalendarWidget(QWidget* parent = nullptr);
    ~CalendarWidget();

    void refreshData();

private slots:
    void onNewEvent();
    void onNewCalendar();
    void onPrevPeriod();
    void onNextPeriod();
    void onToday();
    void onViewChanged(int index);
    void onDateSelected(const QDate& date);
    void onEventSelected(int row, int col);
    void onCalendarSelectionChanged(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onCalendarContextMenu(const QPoint& pos);

private:
    void buildUi();
    void updateViews();
    void updateDayView();
    void updateWeekView();
    void updateMonthView();
    void updateEventList();

    QDate mSelectedDate;
    int mCurrentViewMode; // 0=Day, 1=Week, 2=Month
    QString mSearchText;

    // UI elements (now loaded from UI file but kept as pointers for compatibility)
    QCalendarWidget* mSidebarCalendar;
    QListWidget* mCalendarList;
    
    QLabel* mPeriodLabel;
    QLineEdit* mSearchEdit;
    
    QTableWidget* mEventTable; // Upcoming events list at top
    QStackedWidget* mViewStack;
    
    // Day View components
    QTableWidget* mDayTable;
    
    // Week View components
    QTableWidget* mWeekTable;
    
    // Month View components
    QTableWidget* mMonthTable;

    // Cached event IDs for grids
    QMap<QString, QString> mCellEventMap; // "viewMode_row_col" -> Event ID

    Ui::CalendarWidget ui;
};

#endif // CALENDARWIDGET_H
