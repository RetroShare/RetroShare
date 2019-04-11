/*******************************************************************************
 * libresapi/api/SettingsHandler.h                                             *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#ifndef SETTINGSHANDLER_H
#define SETTINGSHANDLER_H

#include <QSettings>

#include <util/rsthreads.h>

#include "ResourceRouter.h"
#include "StateTokenServer.h"

/* Reimplemented class RSettings*/
namespace resource_api
{
    class SettingsHandler : public ResourceRouter, public QSettings
    {
    public:
        SettingsHandler(StateTokenServer* sts, const QString group = QString());
        ~SettingsHandler();

        static void reset();

        QVariant value(const QString &key,
                               const QVariant &defaultVal = QVariant()) const;

        void setValue(const QString &key, const QVariant &val);

        QVariant valueFromGroup(const QString &group, const QString &key,
                               const QVariant &defaultVal = QVariant());
        void setValueToGroup(const QString &group, const QString &key,
                                     const QVariant &val);

    protected:
        void setDefault(const QString &key, const QVariant &val);
        QVariant defaultValue(const QString &key) const;

        bool m_bValid;

    private:
        void handleSettingsRequest(Request& req, Response& resp);

        void handleGetAdvancedMode(Request& req, Response& resp);
        void handleSetAdvancedMode(Request& req, Response& resp);

        void handleGetFlickableGridMode(Request& req, Response& resp);
        void handleSetFlickableGridMode(Request& req, Response& resp);

        void handleGetAutoLogin(Request& req, Response& resp);
        void handleSetAutoLogin(Request& req, Response& resp);

        QHash<QString, QVariant> _defaults;

        StateTokenServer* mStateTokenServer;

        RsMutex mMtx;
        StateToken mStateToken; // mutex protected
    };
} // namespace resource_api

#endif // SETTINGSHANDLER_H
