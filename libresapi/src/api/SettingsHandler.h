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
