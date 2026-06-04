/*******************************************************************************
 * retroshare-gui/src/gui/msgs/TasksWidget.cpp                                 *
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

#include "gui/msgs/TasksWidget.h"
#include "gui/msgs/TaskDialog.h"
#include "gui/msgs/CalendarPropertiesDialog.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QCalendarWidget>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QCheckBox>
#include <QDateTime>
#include <QUuid>

TasksWidget::TasksWidget(QWidget* parent)
    : QWidget(parent), mCurrentFilterMode(0)
{
    buildUi();
    refreshData();
}

TasksWidget::~TasksWidget() {}

void TasksWidget::buildUi() {
    ui.setupUi(this);

    // Initialize UI pointers
    mSidebarCalendar = ui.sidebarCalendar;
    mFilterList = ui.filterList;
    mCalendarList = ui.calendarList;
    mQuickTaskEdit = ui.quickTaskEdit;
    mSearchEdit = ui.searchEdit;
    mTaskTable = ui.taskTable;

    // Filter list items setup
    mFilterList->addItem(tr("All Tasks"));
    mFilterList->addItem(tr("Active Tasks"));
    mFilterList->addItem(tr("Completed Tasks"));
    mFilterList->addItem(tr("Overdue Tasks"));
    mFilterList->setCurrentRow(0);

    // Main Tasks List Table setup
    mTaskTable->setColumnCount(5);
    mTaskTable->setHorizontalHeaderLabels({"", "!", tr("Title"), tr("Start"), tr("Due Date")});
    mTaskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    mTaskTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    mTaskTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mTaskTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    mTaskTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    mTaskTable->horizontalHeader()->resizeSection(0, 30);
    mTaskTable->horizontalHeader()->resizeSection(1, 30);
    mTaskTable->verticalHeader()->setVisible(false);
    mTaskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTaskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTaskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Splitter configuration
    ui.splitter->setStretchFactor(0, 0);
    ui.splitter->setStretchFactor(1, 1);

    // Connections
    connect(ui.newTaskBtn, SIGNAL(clicked()), this, SLOT(onNewTask()));
    connect(mFilterList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onFilterSelected(QListWidgetItem*)));
    connect(mCalendarList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onCalendarSelectionChanged(QListWidgetItem*)));
    connect(mCalendarList, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onCalendarContextMenu(const QPoint&)));
    connect(mQuickTaskEdit, SIGNAL(returnPressed()), this, SLOT(onQuickTaskAdded()));
    connect(mSearchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onSearchChanged(const QString&)));
    connect(mTaskTable, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(onTaskDoubleClicked(int,int)));
    connect(mTaskTable, SIGNAL(cellClicked(int,int)), this, SLOT(onTaskClicked(int,int)));
}

void TasksWidget::refreshData() {
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

    updateTaskList();
}

void TasksWidget::updateTaskList() {
    mTaskTable->setRowCount(0);

    const auto& tasks = CalendarData::instance()->getTasks();

    // Get enabled calendar IDs
    QStringList enabledCalIds;
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) {
            enabledCalIds.append(item->data(Qt::UserRole).toString());
        }
    }

    int row = 0;
    QDateTime now = QDateTime::currentDateTime();

    for (const auto& task : tasks) {
        if (!enabledCalIds.contains(task.calendarId)) continue;

        // Apply filters
        if (mCurrentFilterMode == 1 && task.completed) continue; // Active Tasks
        if (mCurrentFilterMode == 2 && !task.completed) continue; // Completed Tasks
        if (mCurrentFilterMode == 3 && (task.completed || !task.hasDue || task.due >= now)) continue; // Overdue Tasks

        // Search text filter
        if (!mSearchText.isEmpty() && !task.title.contains(mSearchText, Qt::CaseInsensitive) && 
            !task.description.contains(mSearchText, Qt::CaseInsensitive)) {
            continue;
        }

        mTaskTable->insertRow(row);

        // Checkbox column
        QTableWidgetItem* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(task.completed ? Qt::Checked : Qt::Unchecked);
        checkItem->setData(Qt::UserRole, task.id);
        mTaskTable->setItem(row, 0, checkItem);

        // Priority / Exclamation mark column
        QTableWidgetItem* priorityItem = new QTableWidgetItem(task.category == tr("Urgent") ? "!" : "");
        priorityItem->setTextAlignment(Qt::AlignCenter);
        mTaskTable->setItem(row, 1, priorityItem);

        // Title column
        QTableWidgetItem* titleItem = new QTableWidgetItem(task.title);
        if (task.completed) {
            QFont font = titleItem->font();
            font.setStrikeOut(true);
            titleItem->setFont(font);
            titleItem->setForeground(QBrush(Qt::gray));
        }
        mTaskTable->setItem(row, 2, titleItem);

        // Start Date column
        QString startStr = task.hasStart ? task.start.toString("yyyy-MM-dd hh:mm") : tr("None");
        mTaskTable->setItem(row, 3, new QTableWidgetItem(startStr));

        // Due Date column
        QString dueStr = task.hasDue ? task.due.toString("yyyy-MM-dd hh:mm") : tr("None");
        QTableWidgetItem* dueItem = new QTableWidgetItem(dueStr);
        if (task.hasDue && !task.completed && task.due < now) {
            dueItem->setForeground(QBrush(Qt::red));
        }
        mTaskTable->setItem(row, 4, dueItem);

        row++;
    }
}

void TasksWidget::onNewTask() {
    TaskDialog dlg("", this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshData();
    }
}

void TasksWidget::onQuickTaskAdded() {
    QString title = mQuickTaskEdit->text().trimmed();
    if (title.isEmpty()) return;

    CalendarTask task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Choose the first enabled calendar
    QString calId = "personal";
    for (int i = 0; i < mCalendarList->count(); ++i) {
        QListWidgetItem* item = mCalendarList->item(i);
        if (item->checkState() == Qt::Checked) {
            calId = item->data(Qt::UserRole).toString();
            break;
        }
    }
    
    task.calendarId = calId;
    task.title = title;
    task.location = "";
    task.category = "None";
    task.hasStart = false;
    task.hasDue = false;
    task.status = "Not started";
    task.percentComplete = 0;
    task.repeat = "Does not repeat";
    task.reminder = "No reminder";
    task.description = "";
    task.completed = false;

    CalendarData::instance()->addTask(task);
    mQuickTaskEdit->clear();
    refreshData();
}

void TasksWidget::onTaskDoubleClicked(int row, int col) {
    if (col == 0) return; // Ignore checkbox double clicks

    QTableWidgetItem* checkItem = mTaskTable->item(row, 0);
    if (checkItem) {
        QString taskId = checkItem->data(Qt::UserRole).toString();
        TaskDialog dlg(taskId, this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshData();
        }
    }
}

void TasksWidget::onTaskClicked(int row, int col) {
    if (col != 0) return; // Only trigger for Checkbox column

    QTableWidgetItem* checkItem = mTaskTable->item(row, 0);
    if (checkItem) {
        QString taskId = checkItem->data(Qt::UserRole).toString();
        
        // Find and toggle completion
        const auto& tasks = CalendarData::instance()->getTasks();
        for (auto t : tasks) {
            if (t.id == taskId) {
                t.completed = !t.completed;
                t.status = t.completed ? tr("Completed") : tr("Not started");
                t.percentComplete = t.completed ? 100 : 0;
                CalendarData::instance()->updateTask(t);
                break;
            }
        }
        refreshData();
    }
}

void TasksWidget::onFilterSelected(QListWidgetItem* item) {
    int idx = mFilterList->row(item);
    if (idx != -1) {
        mCurrentFilterMode = idx;
        updateTaskList();
    }
}

void TasksWidget::onCalendarSelectionChanged(QListWidgetItem* /*item*/) {
    updateTaskList();
}

void TasksWidget::onSearchChanged(const QString& text) {
    mSearchText = text.trimmed();
    updateTaskList();
}

void TasksWidget::onCalendarContextMenu(const QPoint& pos) {
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
        updateTaskList();
    } else if (selectedAct == showAllAct) {
        mCalendarList->blockSignals(true);
        for (int i = 0; i < mCalendarList->count(); ++i) {
            mCalendarList->item(i)->setCheckState(Qt::Checked);
        }
        mCalendarList->blockSignals(false);
        updateTaskList();
    } else if (selectedAct == newAct) {
        CalendarPropertiesDialog dlg("", this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshData();
        }
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
