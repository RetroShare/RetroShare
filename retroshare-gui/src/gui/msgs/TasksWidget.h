/*******************************************************************************
 * retroshare-gui/src/gui/msgs/TasksWidget.h                                   *
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

#ifndef TASKSWIDGET_H
#define TASKSWIDGET_H

#include <QWidget>
#include <QDate>
#include "gui/msgs/CalendarData.h"
#include "ui_TasksWidget.h"

class QListWidgetItem;
class QComboBox;

class TasksWidget : public QWidget {
    Q_OBJECT
public:
    TasksWidget(QWidget* parent = nullptr);
    ~TasksWidget();

    void refreshData();

private slots:
    void onNewTask();
    void onQuickTaskAdded();
    void onTaskDoubleClicked(int row, int col);
    void onTaskClicked(int row, int col);
    void onFilterSelected(QListWidgetItem* item);
    void onCalendarSelectionChanged(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onCalendarContextMenu(const QPoint& pos);
    void onCalendarViewModeChanged(int index);

private:
    void buildUi();
    void updateTaskList();
    void exportCalendar(const QString& calId, const QString& calName);

    int mCurrentFilterMode; // 0=All, 1=Active, 2=Completed, 3=Overdue
    QString mSearchText;
    int mCalendarListMode; // 0=My Calendars, 1=Shared Calendars
    bool mInitialLoadDone;

protected:
    void showEvent(QShowEvent* event) override;

    // UI elements (loaded from UI file, kept as pointers for compatibility)
    QCalendarWidget* mSidebarCalendar;
    QListWidget* mFilterList;
    QListWidget* mCalendarList;
    QComboBox* mCalendarViewCombo;

    QLineEdit* mQuickTaskEdit;
    QLineEdit* mSearchEdit;
    QTableWidget* mTaskTable;

    Ui::TasksWidget ui;
};

#endif // TASKSWIDGET_H
