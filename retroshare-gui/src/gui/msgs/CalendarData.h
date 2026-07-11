/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarData.h                                  *
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

#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QColor>
#include <QStringList>
#include <QObject>
#include <memory>
#include <retroshare/rsevents.h>

struct CalendarInfo {
    QString id;
    QString name;
    QColor color;
    bool isPublic;
    QString owner; // contact PGP ID or "local"
    bool showReminders;
    QString email;
    bool onNetwork;

    uint32_t circleType;
    QString circleId;
    QString internalCircle;
    uint32_t groupFlags;
    QString description;
};

struct CalendarEvent {
    QString id;
    QString calendarId;
    QString title;
    QString location;
    QString category;
    bool allDay;
    QDateTime start;
    QDateTime end;
    QString repeat;
    QString reminder;
    QString description;
    QStringList attendees; // PGP IDs of contacts
    bool isPublic;
    QStringList attachments;
};

struct CalendarTask {
    QString id;
    QString calendarId;
    QString title;
    QString location;
    QString category;
    bool hasStart;
    QDateTime start;
    bool hasDue;
    QDateTime due;
    QString status;
    int percentComplete;
    QString repeat;
    QString reminder;
    QString description;
    bool completed;
    QStringList attachments;
};

class CalendarData : public QObject {
    Q_OBJECT
public:
    static CalendarData* instance();

    void loadData();
    void saveData();

    const QList<CalendarInfo>& getCalendars() const { return mCalendars; }
    const QList<CalendarEvent>& getEvents() const { return mEvents; }
    const QList<CalendarTask>& getTasks() const { return mTasks; }

    void addCalendar(const CalendarInfo& cal);
    void updateCalendar(const CalendarInfo& cal);
    void removeCalendar(const QString& id);

    void addEvent(const CalendarEvent& ev);
    void updateEvent(const CalendarEvent& ev);
    void removeEvent(const QString& id);

    void addTask(const CalendarTask& task);
    void updateTask(const CalendarTask& task);
    void removeTask(const QString& id);

    // Helpers
    static QMap<QString, QString> getContacts(); // map PGP ID -> Name

    QString exportCalendarToIcs(const QString& calId) const;
    void importCalendarFromIcs(const QString& calId, const QString& icsData);
    void migrateCalendarData(const QString& oldId, const QString& newId);
    void publishCalendarUpdates(const QString& calId);
    bool publishCalendar(const QString& oldId, const QString& email, QString& newIdOut);
    bool subscribeToCalendar(const QString& id, bool subscribe, const QString& name = "");

signals:
    void calendarDataChanged();

public slots:
    void updateCalendars();

private slots:
    void handleGxsEvent(std::shared_ptr<const RsEvent> event);

private:
    CalendarData();
    ~CalendarData() override;

    QList<CalendarInfo> mCalendars;
    QList<CalendarEvent> mEvents;
    QList<CalendarTask> mTasks;
    QMap<QString, QString> mLastMsgIds;

    static CalendarData* mInstance;
    uint32_t mEventHandlerId;
};

#endif // CALENDARDATA_H
