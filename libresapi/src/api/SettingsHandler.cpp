#include "SettingsHandler.h"

#include <iostream>

#include <retroshare/rsinit.h>

namespace resource_api
{
    #define SETTINGS_FILE   (QString::fromUtf8(RsAccounts::AccountDirectory().c_str()) + "/Sonet.conf")

    SettingsHandler::SettingsHandler(StateTokenServer *sts, const QString settingsGroup) :
        QSettings(SETTINGS_FILE, QSettings::IniFormat),
        mStateTokenServer(sts),
        mMtx("SettingsHandler Mutex"),
        mStateToken(sts->getNewToken())
    {
        RsPeerId sPreferedId;
        m_bValid = RsAccounts::GetPreferredAccountId(sPreferedId);

        if (!settingsGroup.isEmpty())
            beginGroup(settingsGroup);

        addResourceHandler("*", this, &SettingsHandler::handleSettingsRequest);
        addResourceHandler("get_advanced_mode", this, &SettingsHandler::handleGetAdvancedMode);
        addResourceHandler("set_advanced_mode", this, &SettingsHandler::handleSetAdvancedMode);
        addResourceHandler("get_flickable_grid_mode", this, &SettingsHandler::handleGetFlickableGridMode);
        addResourceHandler("set_flickable_grid_mode", this, &SettingsHandler::handleSetFlickableGridMode);
        addResourceHandler("get_auto_login", this, &SettingsHandler::handleGetAutoLogin);
        addResourceHandler("set_auto_login", this, &SettingsHandler::handleSetAutoLogin);
    }

    SettingsHandler::~SettingsHandler()
    {
        sync();
    }

    void SettingsHandler::handleSettingsRequest(Request &/*req*/, Response &resp)
    {

    }

    void SettingsHandler::handleGetAdvancedMode(Request &/*req*/, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool advanced_mode = valueFromGroup("General", "Advanced", false).toBool();
        resp.mDataStream << makeKeyValueReference("advanced_mode", advanced_mode);
        resp.setOk();
        sync();
    }

    void SettingsHandler::handleSetAdvancedMode(Request &req, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool advanced_mode;
        req.mStream << makeKeyValueReference("advanced_mode", advanced_mode);
        setValueToGroup("General", "Advanced", advanced_mode);
        resp.setOk();
        sync();
    }

    void SettingsHandler::handleGetFlickableGridMode(Request &/*req*/, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool flickable_grid_mode = valueFromGroup("General", "FlickableGrid", false).toBool();
        resp.mDataStream << makeKeyValueReference("flickable_grid_mode", flickable_grid_mode);
        resp.setOk();
        sync();
    }

    void SettingsHandler::handleSetFlickableGridMode(Request &req, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool flickable_grid_mode;
        req.mStream << makeKeyValueReference("flickable_grid_mode", flickable_grid_mode);
        setValueToGroup("General", "FlickableGrid", flickable_grid_mode);

        resp.setOk();
        sync();
    }

    void SettingsHandler::handleGetAutoLogin(Request &/*req*/, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool autoLogin = RsInit::getAutoLogin();;
        resp.mDataStream << makeKeyValueReference("auto_login", autoLogin);
        resp.setOk();
        sync();
    }

    void SettingsHandler::handleSetAutoLogin(Request &req, Response &resp)
    {
        {
            RS_STACK_MUTEX(mMtx);
            resp.mStateToken = mStateToken;
        }

        bool autoLogin;
        req.mStream << makeKeyValueReference("auto_login", autoLogin);
        RsInit::setAutoLogin(autoLogin);

        resp.setOk();
        sync();
    }

    QVariant SettingsHandler::value(const QString &key, const QVariant &defaultVal) const
    {
        if (m_bValid == false)
        {
            return defaultVal.isNull() ? defaultValue(key) : defaultVal;
        }
        return QSettings::value(key, defaultVal.isNull() ? defaultValue(key) : defaultVal);
    }

    void SettingsHandler::setValue(const QString &key, const QVariant &val)
    {
        if (m_bValid == false)
        {
            std::cerr << "RSettings::setValue() Calling on invalid object, key = " << key.toStdString() << std::endl;
            return;
        }
        if (val == defaultValue(key))
            QSettings::remove(key);
        else if (val != value(key))
            QSettings::setValue(key, val);
    }

    QVariant SettingsHandler::valueFromGroup(const QString &group, const QString &key, const QVariant &defaultVal)
    {
        beginGroup(group);
        QVariant val = value(key, defaultVal);
        endGroup();

        return val;
    }

    void SettingsHandler::setValueToGroup(const QString &group, const QString &key, const QVariant &val)
    {
        beginGroup(group);
        setValue(key, val);
        endGroup();
    }

    void SettingsHandler::setDefault(const QString &key, const QVariant &val)
    {
      _defaults.insert(key, val);
    }

    QVariant SettingsHandler::defaultValue(const QString &key) const
    {
      if (_defaults.contains(key))
        return _defaults.value(key);
      return QVariant();
    }

    void SettingsHandler::reset()
    {
      /* Static method, so we have to create a QSettings object. */
      QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
      settings.clear();
    }
} // namespace resource_api

