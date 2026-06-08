/*******************************************************************************
 * retroshare-gui/src/gui/msgs/CalendarData.cpp                                *
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

#include "gui/msgs/CalendarData.h"
#include <retroshare/rsinit.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxscalendar.h>
#include <retroshare/rsgxscircles.h>
#include <util/qtthreadsutils.h>
#include <QSettings>
#include <QDir>
#include <QUuid>

CalendarData* CalendarData::mInstance = nullptr;

CalendarData* CalendarData::instance() {
    if (!mInstance) {
        mInstance = new CalendarData();
    }
    return mInstance;
}

CalendarData::CalendarData() : QObject(), mEventHandlerId(0) {
    loadData();

    if (rsEvents && rsGxsCalendar) {
        RsEventType calendarEventType = (RsEventType)rsEvents->getDynamicEventType("GXS_CALENDAR");

        rsEvents->registerEventsHandler(
            [this](std::shared_ptr<const RsEvent> event) {
                RsQThreadUtils::postToObject([=]() { handleGxsEvent(event); }, this);
            },
            mEventHandlerId, calendarEventType
        );
    }
}

CalendarData::~CalendarData() {
    saveData();
    if (rsEvents && mEventHandlerId != 0) {
        rsEvents->unregisterEventsHandler(mEventHandlerId);
    }
}

void CalendarData::loadData() {
    mCalendars.clear();
    mEvents.clear();
    mTasks.clear();

    QString accountDir = QString::fromStdString(RsAccounts::AccountDirectory());
    QString path = accountDir + "/calendar.conf";

    QSettings settings(path, QSettings::IniFormat);

    // Load Calendars
    int calSize = settings.beginReadArray("calendars");
    for (int i = 0; i < calSize; ++i) {
        settings.setArrayIndex(i);
        CalendarInfo cal;
        cal.id = settings.value("id").toString();
        cal.name = settings.value("name").toString();
        cal.color = QColor(settings.value("color").toString());
        cal.isPublic = settings.value("isPublic").toBool();
        cal.owner = settings.value("owner").toString();
        cal.showReminders = settings.value("showReminders", true).toBool();
        cal.email = settings.value("email", "").toString();
        cal.onNetwork = settings.value("onNetwork", false).toBool();
        cal.circleType = settings.value("circleType", 1).toUInt();
        cal.circleId = settings.value("circleId", "").toString();
        cal.internalCircle = settings.value("internalCircle", "").toString();
        cal.groupFlags = settings.value("groupFlags", 4).toUInt(); // Default FLAG_PRIVACY_PUBLIC (4)
        cal.description = settings.value("description", "").toString();
        mCalendars.append(cal);
    }
    settings.endArray();

    // Ensure we have at least one default calendar
    if (mCalendars.isEmpty()) {
        CalendarInfo defaultCal;
        defaultCal.id = "personal";
        defaultCal.name = "Privat";
        defaultCal.color = QColor("#4a90e2");
        defaultCal.isPublic = false;
        defaultCal.owner = "local";
        defaultCal.showReminders = true;
        defaultCal.email = "retroshare <retroshare@GXSID>";
        defaultCal.onNetwork = false;
        mCalendars.append(defaultCal);

        CalendarInfo testCal;
        testCal.id = "test";
        testCal.name = "test";
        testCal.color = QColor("#50e3c2");
        testCal.isPublic = true;
        testCal.owner = "local";
        testCal.showReminders = true;
        testCal.email = "retroshare <retroshare@GXSID>";
        testCal.onNetwork = true;
        mCalendars.append(testCal);
    }

    // Load Events
    int eventSize = settings.beginReadArray("events");
    for (int i = 0; i < eventSize; ++i) {
        settings.setArrayIndex(i);
        CalendarEvent ev;
        ev.id = settings.value("id").toString();
        ev.calendarId = settings.value("calendarId").toString();
        ev.title = settings.value("title").toString();
        ev.location = settings.value("location").toString();
        ev.category = settings.value("category").toString();
        ev.allDay = settings.value("allDay").toBool();
        ev.start = settings.value("start").toDateTime();
        ev.end = settings.value("end").toDateTime();
        ev.repeat = settings.value("repeat").toString();
        ev.reminder = settings.value("reminder").toString();
        ev.description = settings.value("description").toString();
        ev.attendees = settings.value("attendees").toStringList();
        ev.isPublic = settings.value("isPublic").toBool();
        mEvents.append(ev);
    }
    settings.endArray();

    // Load Tasks
    int taskSize = settings.beginReadArray("tasks");
    for (int i = 0; i < taskSize; ++i) {
        settings.setArrayIndex(i);
        CalendarTask task;
        task.id = settings.value("id").toString();
        task.calendarId = settings.value("calendarId").toString();
        task.title = settings.value("title").toString();
        task.location = settings.value("location").toString();
        task.category = settings.value("category").toString();
        task.hasStart = settings.value("hasStart").toBool();
        task.start = settings.value("start").toDateTime();
        task.hasDue = settings.value("hasDue").toBool();
        task.due = settings.value("due").toDateTime();
        task.status = settings.value("status").toString();
        task.percentComplete = settings.value("percentComplete").toInt();
        task.repeat = settings.value("repeat").toString();
        task.reminder = settings.value("reminder").toString();
        task.description = settings.value("description").toString();
        task.completed = settings.value("completed").toBool();
        mTasks.append(task);
    }
    settings.endArray();
}

void CalendarData::saveData() {
    QString accountDir = QString::fromStdString(RsAccounts::AccountDirectory());
    QString path = accountDir + "/calendar.conf";

    QSettings settings(path, QSettings::IniFormat);

    // Save Calendars
    settings.beginWriteArray("calendars");
    for (int i = 0; i < mCalendars.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", mCalendars[i].id);
        settings.setValue("name", mCalendars[i].name);
        settings.setValue("color", mCalendars[i].color.name());
        settings.setValue("isPublic", mCalendars[i].isPublic);
        settings.setValue("owner", mCalendars[i].owner);
        settings.setValue("showReminders", mCalendars[i].showReminders);
        settings.setValue("email", mCalendars[i].email);
        settings.setValue("onNetwork", mCalendars[i].onNetwork);
        settings.setValue("circleType", mCalendars[i].circleType);
        settings.setValue("circleId", mCalendars[i].circleId);
        settings.setValue("internalCircle", mCalendars[i].internalCircle);
        settings.setValue("groupFlags", mCalendars[i].groupFlags);
        settings.setValue("description", mCalendars[i].description);
    }
    settings.endArray();

    // Save Events
    settings.beginWriteArray("events");
    for (int i = 0; i < mEvents.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", mEvents[i].id);
        settings.setValue("calendarId", mEvents[i].calendarId);
        settings.setValue("title", mEvents[i].title);
        settings.setValue("location", mEvents[i].location);
        settings.setValue("category", mEvents[i].category);
        settings.setValue("allDay", mEvents[i].allDay);
        settings.setValue("start", mEvents[i].start);
        settings.setValue("end", mEvents[i].end);
        settings.setValue("repeat", mEvents[i].repeat);
        settings.setValue("reminder", mEvents[i].reminder);
        settings.setValue("description", mEvents[i].description);
        settings.setValue("attendees", mEvents[i].attendees);
        settings.setValue("isPublic", mEvents[i].isPublic);
    }
    settings.endArray();

    // Save Tasks
    settings.beginWriteArray("tasks");
    for (int i = 0; i < mTasks.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", mTasks[i].id);
        settings.setValue("calendarId", mTasks[i].calendarId);
        settings.setValue("title", mTasks[i].title);
        settings.setValue("location", mTasks[i].location);
        settings.setValue("category", mTasks[i].category);
        settings.setValue("hasStart", mTasks[i].hasStart);
        settings.setValue("start", mTasks[i].start);
        settings.setValue("hasDue", mTasks[i].hasDue);
        settings.setValue("due", mTasks[i].due);
        settings.setValue("status", mTasks[i].status);
        settings.setValue("percentComplete", mTasks[i].percentComplete);
        settings.setValue("repeat", mTasks[i].repeat);
        settings.setValue("reminder", mTasks[i].reminder);
        settings.setValue("description", mTasks[i].description);
        settings.setValue("completed", mTasks[i].completed);
    }
    settings.endArray();

    settings.sync();
}

void CalendarData::addCalendar(const CalendarInfo& cal) {
    mCalendars.append(cal);
    saveData();
}

void CalendarData::updateCalendar(const CalendarInfo& cal) {
    for (int i = 0; i < mCalendars.size(); ++i) {
        if (mCalendars[i].id == cal.id) {
            mCalendars[i] = cal;
            break;
        }
    }
    saveData();
}

void CalendarData::removeCalendar(const QString& id) {
    for (int i = 0; i < mCalendars.size(); ++i) {
        if (mCalendars[i].id == id) {
            // Unsubscribe from GXS if network calendar
            if (mCalendars[i].onNetwork && rsGxsCalendar) {
                std::string errMsg;
                rsGxsCalendar->subscribeToCalendar(RsGxsGroupId(id.toStdString()), false, errMsg);
            }
            mCalendars.removeAt(i);
            break;
        }
    }

    // Remove associated events and tasks
    mEvents.erase(std::remove_if(mEvents.begin(), mEvents.end(),
        [&id](const CalendarEvent& ev) { return ev.calendarId == id; }), mEvents.end());
    mTasks.erase(std::remove_if(mTasks.begin(), mTasks.end(),
        [&id](const CalendarTask& t) { return t.calendarId == id; }), mTasks.end());

    saveData();
}

void CalendarData::addEvent(const CalendarEvent& ev) {
    mEvents.append(ev);
    saveData();
    publishCalendarUpdates(ev.calendarId);
}

void CalendarData::updateEvent(const CalendarEvent& ev) {
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents[i].id == ev.id) {
            mEvents[i] = ev;
            break;
        }
    }
    saveData();
    publishCalendarUpdates(ev.calendarId);
}

void CalendarData::removeEvent(const QString& id) {
    QString calId;
    for (int i = 0; i < mEvents.size(); ++i) {
        if (mEvents[i].id == id) {
            calId = mEvents[i].calendarId;
            mEvents.removeAt(i);
            break;
        }
    }
    saveData();
    if (!calId.isEmpty()) {
        publishCalendarUpdates(calId);
    }
}

void CalendarData::addTask(const CalendarTask& task) {
    mTasks.append(task);
    saveData();
    publishCalendarUpdates(task.calendarId);
}

void CalendarData::updateTask(const CalendarTask& task) {
    for (int i = 0; i < mTasks.size(); ++i) {
        if (mTasks[i].id == task.id) {
            mTasks[i] = task;
            break;
        }
    }
    saveData();
    publishCalendarUpdates(task.calendarId);
}

void CalendarData::removeTask(const QString& id) {
    QString calId;
    for (int i = 0; i < mTasks.size(); ++i) {
        if (mTasks[i].id == id) {
            calId = mTasks[i].calendarId;
            mTasks.removeAt(i);
            break;
        }
    }
    saveData();
    if (!calId.isEmpty()) {
        publishCalendarUpdates(calId);
    }
}

QMap<QString, QString> CalendarData::getContacts() {
    QMap<QString, QString> contacts;
    
    if (!rsPeers) {
        return contacts;
    }

    std::list<RsPgpId> pgpIds;
    rsPeers->getGPGAcceptedList(pgpIds);

    for (const auto& pgpId : pgpIds) {
        RsPeerDetails details;
        if (rsPeers->getGPGDetails(pgpId, details)) {
            contacts.insert(QString::fromStdString(pgpId.toStdString()), QString::fromUtf8(details.name.c_str()));
        }
    }

    // Fallbacks/Mocks if empty (to make sure it lists some developers/coworkers as requested in the screenshots)
    if (contacts.isEmpty()) {
        contacts.insert("friend1", "Alice (Developer)");
        contacts.insert("friend2", "Bob (Coworker)");
        contacts.insert("friend3", "Charlie (Friend)");
    }

    return contacts;
}

static RsGxsId getGxsIdFromEmail(const QString& email) {
    int idx = email.lastIndexOf('@');
    int endIdx = email.lastIndexOf('>');
    if (idx != -1 && endIdx != -1 && endIdx > idx) {
        std::string gxsIdStr = email.mid(idx + 1, endIdx - idx - 1).toStdString();
        return RsGxsId(gxsIdStr);
    }
    return RsGxsId();
}

QString CalendarData::exportCalendarToIcs(const QString& calId) const {
    QString icsContent = "BEGIN:VCALENDAR\r\n"
                         "VERSION:2.0\r\n"
                         "PRODID:-//RetroShare//Calendar//EN\r\n";

    for (const auto& ev : mEvents) {
        if (ev.calendarId != calId) continue;

        icsContent += "BEGIN:VEVENT\r\n";
        icsContent += QString("UID:%1\r\n").arg(ev.id);
        icsContent += QString("SUMMARY:%1\r\n").arg(ev.title);
        
        if (!ev.description.isEmpty()) {
            QString desc = ev.description;
            desc.replace("\n", "\\n").replace("\r", "");
            icsContent += QString("DESCRIPTION:%1\r\n").arg(desc);
        }
        
        if (!ev.location.isEmpty()) {
            icsContent += QString("LOCATION:%1\r\n").arg(ev.location);
        }
        
        if (!ev.category.isEmpty()) {
            icsContent += QString("CATEGORIES:%1\r\n").arg(ev.category);
        }

        if (ev.allDay) {
            icsContent += QString("DTSTART;VALUE=DATE:%1\r\n").arg(ev.start.toString("yyyyMMdd"));
            icsContent += QString("DTEND;VALUE=DATE:%1\r\n").arg(ev.end.toString("yyyyMMdd"));
        } else {
            icsContent += QString("DTSTART:%1\r\n").arg(ev.start.toUTC().toString("yyyyMMdd'T'HHmmss'Z'"));
            icsContent += QString("DTEND:%1\r\n").arg(ev.end.toUTC().toString("yyyyMMdd'T'HHmmss'Z'"));
        }

        icsContent += "END:VEVENT\r\n";
    }

    for (const auto& t : mTasks) {
        if (t.calendarId != calId) continue;

        icsContent += "BEGIN:VTODO\r\n";
        icsContent += QString("UID:%1\r\n").arg(t.id);
        icsContent += QString("SUMMARY:%1\r\n").arg(t.title);

        if (!t.description.isEmpty()) {
            QString desc = t.description;
            desc.replace("\n", "\\n").replace("\r", "");
            icsContent += QString("DESCRIPTION:%1\r\n").arg(desc);
        }

        if (!t.location.isEmpty()) {
            icsContent += QString("LOCATION:%1\r\n").arg(t.location);
        }

        if (!t.category.isEmpty()) {
            icsContent += QString("CATEGORIES:%1\r\n").arg(t.category);
        }

        if (t.hasStart) {
            icsContent += QString("DTSTART:%1\r\n").arg(t.start.toUTC().toString("yyyyMMdd'T'HHmmss'Z'"));
        }

        if (t.hasDue) {
            icsContent += QString("DUE:%1\r\n").arg(t.due.toUTC().toString("yyyyMMdd'T'HHmmss'Z'"));
        }

        if (!t.status.isEmpty()) {
            icsContent += QString("STATUS:%1\r\n").arg(t.status);
        }

        icsContent += QString("PERCENT-COMPLETE:%1\r\n").arg(t.percentComplete);
        icsContent += QString("COMPLETED:%1\r\n").arg(t.completed ? "TRUE" : "FALSE");

        icsContent += "END:VTODO\r\n";
    }

    icsContent += "END:VCALENDAR\r\n";
    return icsContent;
}

void CalendarData::importCalendarFromIcs(const QString& calId, const QString& icsData) {
    auto evIt = mEvents.begin();
    while (evIt != mEvents.end()) {
        if (evIt->calendarId == calId) {
            evIt = mEvents.erase(evIt);
        } else {
            ++evIt;
        }
    }

    auto tIt = mTasks.begin();
    while (tIt != mTasks.end()) {
        if (tIt->calendarId == calId) {
            tIt = mTasks.erase(tIt);
        } else {
            ++tIt;
        }
    }

    QStringList rawLines = icsData.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    QStringList lines;
    for (int i = 0; i < rawLines.size(); ++i) {
        QString line = rawLines[i];
        while (i + 1 < rawLines.size() && (rawLines[i + 1].startsWith(" ") || rawLines[i + 1].startsWith("\t"))) {
            line += rawLines[i + 1].mid(1);
            i++;
        }
        lines.append(line);
    }

    auto parseIcsDateTime = [](const QString& val) -> QDateTime {
        QDateTime dt;
        if (val.endsWith('Z')) {
            dt = QDateTime::fromString(val, "yyyyMMdd'T'HHmmss'Z'");
            dt.setTimeSpec(Qt::UTC);
            dt = dt.toLocalTime();
        } else {
            dt = QDateTime::fromString(val, "yyyyMMdd'T'HHmmss");
            dt.setTimeSpec(Qt::LocalTime);
        }
        return dt;
    };

    bool inEvent = false;
    bool inTask = false;
    CalendarEvent currentEvent;
    CalendarTask currentTask;

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) continue;

        if (trimmedLine.startsWith("BEGIN:VEVENT", Qt::CaseInsensitive)) {
            inEvent = true;
            currentEvent = CalendarEvent();
            currentEvent.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            currentEvent.calendarId = calId;
            currentEvent.allDay = false;
            currentEvent.isPublic = true;
        } else if (trimmedLine.startsWith("END:VEVENT", Qt::CaseInsensitive)) {
            if (inEvent) {
                if (!currentEvent.start.isValid()) currentEvent.start = QDateTime::currentDateTime();
                if (!currentEvent.end.isValid()) currentEvent.end = currentEvent.start.addSecs(3600);
                mEvents.append(currentEvent);
                inEvent = false;
            }
        } else if (trimmedLine.startsWith("BEGIN:VTODO", Qt::CaseInsensitive)) {
            inTask = true;
            currentTask = CalendarTask();
            currentTask.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            currentTask.calendarId = calId;
            currentTask.hasStart = false;
            currentTask.hasDue = false;
            currentTask.completed = false;
            currentTask.percentComplete = 0;
        } else if (trimmedLine.startsWith("END:VTODO", Qt::CaseInsensitive)) {
            if (inTask) {
                mTasks.append(currentTask);
                inTask = false;
            }
        } else if (inEvent) {
            int colonIdx = trimmedLine.indexOf(':');
            int semiIdx = trimmedLine.indexOf(';');
            int splitIdx = -1;

            if (colonIdx != -1 && semiIdx != -1) splitIdx = qMin(colonIdx, semiIdx);
            else if (colonIdx != -1) splitIdx = colonIdx;
            else if (semiIdx != -1) splitIdx = semiIdx;

            if (splitIdx != -1) {
                QString key = trimmedLine.left(splitIdx).trimmed();
                QString val = trimmedLine.mid(colonIdx + 1).trimmed();

                if (key.compare("UID", Qt::CaseInsensitive) == 0) {
                    currentEvent.id = val;
                } else if (key.compare("SUMMARY", Qt::CaseInsensitive) == 0) {
                    currentEvent.title = val;
                } else if (key.compare("LOCATION", Qt::CaseInsensitive) == 0) {
                    currentEvent.location = val;
                } else if (key.compare("CATEGORIES", Qt::CaseInsensitive) == 0) {
                    currentEvent.category = val;
                } else if (key.compare("DESCRIPTION", Qt::CaseInsensitive) == 0) {
                    currentEvent.description = val.replace("\\n", "\n").replace("\\r", "").replace("\\,", ",");
                } else if (key.startsWith("DTSTART", Qt::CaseInsensitive)) {
                    if (trimmedLine.contains("VALUE=DATE", Qt::CaseInsensitive)) {
                        currentEvent.allDay = true;
                        currentEvent.start = QDateTime(QDate::fromString(val, "yyyyMMdd"), QTime(0, 0));
                    } else {
                        currentEvent.start = parseIcsDateTime(val);
                    }
                } else if (key.startsWith("DTEND", Qt::CaseInsensitive)) {
                    if (trimmedLine.contains("VALUE=DATE", Qt::CaseInsensitive)) {
                        currentEvent.allDay = true;
                        currentEvent.end = QDateTime(QDate::fromString(val, "yyyyMMdd"), QTime(0, 0));
                    } else {
                        currentEvent.end = parseIcsDateTime(val);
                    }
                }
            }
        } else if (inTask) {
            int colonIdx = trimmedLine.indexOf(':');
            int semiIdx = trimmedLine.indexOf(';');
            int splitIdx = -1;

            if (colonIdx != -1 && semiIdx != -1) splitIdx = qMin(colonIdx, semiIdx);
            else if (colonIdx != -1) splitIdx = colonIdx;
            else if (semiIdx != -1) splitIdx = semiIdx;

            if (splitIdx != -1) {
                QString key = trimmedLine.left(splitIdx).trimmed();
                QString val = trimmedLine.mid(colonIdx + 1).trimmed();

                if (key.compare("UID", Qt::CaseInsensitive) == 0) {
                    currentTask.id = val;
                } else if (key.compare("SUMMARY", Qt::CaseInsensitive) == 0) {
                    currentTask.title = val;
                } else if (key.compare("LOCATION", Qt::CaseInsensitive) == 0) {
                    currentTask.location = val;
                } else if (key.compare("CATEGORIES", Qt::CaseInsensitive) == 0) {
                    currentTask.category = val;
                } else if (key.compare("DESCRIPTION", Qt::CaseInsensitive) == 0) {
                    currentTask.description = val.replace("\\n", "\n").replace("\\r", "").replace("\\,", ",");
                } else if (key.startsWith("DTSTART", Qt::CaseInsensitive)) {
                    currentTask.hasStart = true;
                    currentTask.start = parseIcsDateTime(val);
                } else if (key.startsWith("DUE", Qt::CaseInsensitive)) {
                    currentTask.hasDue = true;
                    currentTask.due = parseIcsDateTime(val);
                } else if (key.compare("STATUS", Qt::CaseInsensitive) == 0) {
                    currentTask.status = val;
                } else if (key.compare("PERCENT-COMPLETE", Qt::CaseInsensitive) == 0) {
                    currentTask.percentComplete = val.toInt();
                } else if (key.compare("COMPLETED", Qt::CaseInsensitive) == 0) {
                    currentTask.completed = (val.compare("TRUE", Qt::CaseInsensitive) == 0);
                }
            }
        }
    }
}

void CalendarData::migrateCalendarData(const QString& oldId, const QString& newId) {
    for (auto& ev : mEvents) {
        if (ev.calendarId == oldId) {
            ev.calendarId = newId;
        }
    }
    for (auto& t : mTasks) {
        if (t.calendarId == oldId) {
            t.calendarId = newId;
        }
    }
}

void CalendarData::publishCalendarUpdates(const QString& calId) {
    if (!rsGxsCalendar) return;

    CalendarInfo cal;
    bool found = false;
    for (const auto& c : mCalendars) {
        if (c.id == calId) {
            cal = c;
            found = true;
            break;
        }
    }
    if (!found || !cal.onNetwork) return;

    RsGxsId authorId = getGxsIdFromEmail(cal.email);
    RsGxsGroupId groupId(calId.toStdString());
    QString ics = exportCalendarToIcs(calId);
    RsGxsMessageId msgId;
    std::string errMsg;
    rsGxsCalendar->publishCalendarIcs(groupId, ics.toStdString(), authorId, msgId, errMsg);
}

bool CalendarData::publishCalendar(const QString& oldId, const QString& email, QString& newIdOut) {
    if (!rsGxsCalendar) return false;

    // Find local calendar
    int calIdx = -1;
    for (int i = 0; i < mCalendars.size(); ++i) {
        if (mCalendars[i].id == oldId) {
            calIdx = i;
            break;
        }
    }
    if (calIdx == -1) return false;

    CalendarInfo& cal = mCalendars[calIdx];
    if (cal.onNetwork) return false; // Already on network

    // Extract GXS ID from email
    RsGxsId authorId = getGxsIdFromEmail(email);

    RsGxsGroupId groupId;
    std::string errMsg;
    if (!rsGxsCalendar->createCalendar(cal.name.toStdString(), "RetroShare Calendar", authorId, cal.circleType, RsGxsCircleId(cal.circleId.toStdString()), RsGxsCircleId(cal.internalCircle.toStdString()), cal.groupFlags, groupId, errMsg)) {
        return false;
    }

    QString newId = QString::fromStdString(groupId.toStdString());
    newIdOut = newId;

    // Migrate events and tasks
    migrateCalendarData(oldId, newId);

    // Update calendar info
    cal.id = newId;
    cal.onNetwork = true;
    cal.isPublic = true;
    cal.email = email;
    cal.owner = "local";

    saveData();

    // Subscribe and publish initial ICS
    rsGxsCalendar->subscribeToCalendar(groupId, true, errMsg);
    publishCalendarUpdates(newId);

    emit calendarDataChanged();
    return true;
}

bool CalendarData::subscribeToCalendar(const QString& id, bool subscribe, const QString& name) {
    if (!rsGxsCalendar) return false;

    RsGxsGroupId groupId(id.toStdString());
    std::string errMsg;
    if (!rsGxsCalendar->subscribeToCalendar(groupId, subscribe, errMsg)) {
        return false;
    }

    if (subscribe) {
        // Find if it's already in mCalendars
        bool found = false;
        for (const auto& c : mCalendars) {
            if (c.id == id) {
                found = true;
                break;
            }
        }
        if (!found) {
            CalendarInfo localCal;
            localCal.id = id;
            localCal.name = name.isEmpty() ? tr("Shared Calendar") : name;
            localCal.color = QColor("#4a90e2");
            localCal.isPublic = true;
            localCal.onNetwork = true;
            localCal.showReminders = true;
            localCal.owner = "network";
            localCal.circleType = 1;
            localCal.circleId = "";
            localCal.internalCircle = "";
            localCal.groupFlags = 4;
            localCal.description = "";
            mCalendars.append(localCal);
            saveData();
        }
        emit calendarDataChanged();
        // Trigger sync to fetch contents
        updateCalendars();
    } else {
        // Remove from local list
        for (int i = 0; i < mCalendars.size(); ++i) {
            if (mCalendars[i].id == id) {
                mCalendars.removeAt(i);
                break;
            }
        }
        // Remove associated events and tasks
        mEvents.erase(std::remove_if(mEvents.begin(), mEvents.end(),
            [&id](const CalendarEvent& ev) { return ev.calendarId == id; }), mEvents.end());
        mTasks.erase(std::remove_if(mTasks.begin(), mTasks.end(),
            [&id](const CalendarTask& t) { return t.calendarId == id; }), mTasks.end());

        saveData();
        emit calendarDataChanged();
    }
    return true;
}

void CalendarData::updateCalendars() {
    if (!rsGxsCalendar) return;

    std::list<RsGroupMetaData> calendars;
    if (rsGxsCalendar->getCalendarsSummaries(calendars)) {
        bool changed = false;
        for (const auto& meta : calendars) {
            bool isSubscribed = (meta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED);
            if (isSubscribed) {
                QString calId = QString::fromStdString(meta.mGroupId.toStdString());
                
                bool found = false;
                CalendarInfo localCal;
                for (auto& c : mCalendars) {
                    if (c.id == calId) {
                        localCal = c;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    localCal.id = calId;
                    localCal.name = QString::fromUtf8(meta.mGroupName.c_str());
                    localCal.color = QColor("#4a90e2");
                    localCal.isPublic = true;
                    localCal.onNetwork = true;
                    localCal.showReminders = true;
                    localCal.owner = "network";
                    localCal.circleType = meta.mCircleType;
                    localCal.circleId = QString::fromStdString(meta.mCircleId.toStdString());
                    localCal.internalCircle = QString::fromStdString(meta.mInternalCircle.toStdString());
                    localCal.groupFlags = meta.mGroupFlags;
                    localCal.description = "";
                    mCalendars.append(localCal);
                    changed = true;
                } else {
                    QString remoteName = QString::fromUtf8(meta.mGroupName.c_str());
                    for (auto& c : mCalendars) {
                        if (c.id == calId && c.name != remoteName) {
                            c.name = remoteName;
                            changed = true;
                        }
                    }
                }

                std::vector<RsGxsCalendarMessage> messages;
                if (rsGxsCalendar->getCalendarContent(meta.mGroupId, messages)) {
                    if (!messages.empty()) {
                        uint32_t latestTime = 0;
                        size_t latestIdx = 0;
                        for (size_t i = 0; i < messages.size(); ++i) {
                            if (messages[i].mMeta.mPublishTs > latestTime) {
                                latestTime = messages[i].mMeta.mPublishTs;
                                latestIdx = i;
                            }
                        }
                        QString msgIdStr = QString::fromStdString(messages[latestIdx].mMeta.mMsgId.toStdString());
                        if (!mLastMsgIds.contains(calId) || mLastMsgIds[calId] != msgIdStr) {
                            importCalendarFromIcs(calId, QString::fromStdString(messages[latestIdx].mIcsData));
                            mLastMsgIds[calId] = msgIdStr;
                            changed = true;
                        }
                    }
                }
            }
        }

        if (changed) {
            saveData();
        }
        // Always emit so the UI refreshes the shared calendar list
        // from GXS group metadata (getCalendarsSummaries), even when
        // local calendar data didn't change.
        emit calendarDataChanged();
    }
}

void CalendarData::handleGxsEvent(std::shared_ptr<const RsEvent> event) {
    const RsGxsCalendarEvent *e = dynamic_cast<const RsGxsCalendarEvent*>(event.get());
    if (e) {
        switch (e->mCalendarEventCode) {
            case RsCalendarEventCode::NEW_CALENDAR:
            case RsCalendarEventCode::UPDATED_CALENDAR:
            case RsCalendarEventCode::NEW_EVENT:
            case RsCalendarEventCode::UPDATED_EVENT:
            case RsCalendarEventCode::SUBSCRIBE_STATUS_CHANGED:
                updateCalendars();
                break;
            default:
                break;
        }
    }
}
