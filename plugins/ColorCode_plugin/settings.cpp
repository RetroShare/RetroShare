/* ColorCode, a free MasterMind clone with built in solver
 * Copyright (C) 2009  Dirk Laebisch
 * http://www.laebisch.com/
 *
 * ColorCode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ColorCode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ColorCode. If not, see <http://www.gnu.org/licenses/>.
*/

#include "settings.h"
#include "colorcode.h"
#include "ccsolver.h"

using namespace std;

const int Settings::INDICATOR_LETTER = 0;
const int Settings::INDICATOR_NUMBER = 1;

Settings::Settings()
{
    mSettings.setDefaultFormat(QSettings::IniFormat);
    mSettings.setFallbacksEnabled(false);

    InitSettings();
    SaveSettingsMap(mDefMap);
    ReadSettings();
    SaveSettingsMap(mLastMap);
}

Settings::~Settings()
{
}

void Settings::InitSettings()
{
    mShowToolBar = true;
    mShowMenuBar = true;
    mShowStatusBar = true;

    mShowIndicators = false;
    mIndicatorType = INDICATOR_LETTER;

    mHideColors = false;

    mPegCnt = 4;
    mColorCnt = 8;
    mSameColors = true;
    mGameMode = ColorCode::MODE_HVM;
    mAutoClose = false;
    mAutoHints = false;
    mHintsDelay = 500;

    mSolverStrength = CCSolver::STRENGTH_HIGH;
}

void Settings::SaveLastSettings()
{
    SaveSettingsMap(mLastMap);
}

void Settings::RestoreLastSettings()
{
    RestoreSettingsMap(mLastMap);
}

void Settings::RestoreDefSettings()
{
    RestoreSettingsMap(mDefMap);
}

void Settings::RestoreSettingsMap(QMap<QString, QVariant> &map)
{
    mShowToolBar = map["ShowToolBar"].toBool();
    mShowMenuBar = map["ShowMenuBar"].toBool();
    mShowStatusBar = map["ShowStatusBar"].toBool();

    mShowIndicators = map["ShowIndicators"].toBool();
    mIndicatorType = map["IndicatorType"].toInt();
    mHideColors = map["HideColors"].toBool();

    mPegCnt = map["ColumnCount"].toInt();
    mColorCnt = map["ColorCount"].toInt();
    mSameColors = map["SameColors"].toBool();

    mGameMode = map["GameMode"].toInt();
    mAutoClose = map["AutoClose"].toBool();
    mAutoHints = map["AutoHints"].toBool();
    mHintsDelay = map["HintsDelay"].toInt();

    mSolverStrength = map["SolverStrength"].toInt();
    Validate();
}

void Settings::SaveSettingsMap(QMap<QString, QVariant> &map)
{
    map["ShowToolBar"] = mShowToolBar;
    map["ShowMenuBar"] = mShowMenuBar;
    map["ShowStatusBar"] = mShowStatusBar;

    map["ShowIndicators"] = mShowIndicators;
    map["IndicatorType"] = mIndicatorType;
    map["HideColors"] = mHideColors;

    map["ColumnCount"] = mPegCnt;
    map["ColorCount"] = mColorCnt;
    map["SameColors"] = mSameColors;

    map["GameMode"] = mGameMode;
    map["AutoClose"] = mAutoClose;
    map["AutoHints"] = mAutoHints;
    map["HintsDelay"] = mHintsDelay;

    map["SolverStrength"] = mSolverStrength;
}

void Settings::ReadSettings()
{
    mSettings.beginGroup("Appearance");
    mShowToolBar = mSettings.value("ShowToolBar", mShowToolBar).toBool();
    mShowMenuBar = mSettings.value("ShowMenuBar", mShowMenuBar).toBool();
    mShowStatusBar = mSettings.value("ShowStatusBar", mShowStatusBar).toBool();

    mShowIndicators = mSettings.value("ShowIndicators", mShowIndicators).toBool();
    mIndicatorType = mSettings.value("IndicatorType", mIndicatorType).toInt();
    mHideColors = mSettings.value("HideColors", mHideColors).toBool();
    mSettings.endGroup();

    mSettings.beginGroup("Game");
    mPegCnt = mSettings.value("ColumnCount", mPegCnt).toInt();
    mColorCnt = mSettings.value("ColorCount", mColorCnt).toInt();
    mSameColors = mSettings.value("SameColors", mSameColors).toBool();

    mGameMode = mSettings.value("GameMode", mGameMode).toInt();
    mAutoClose = mSettings.value("AutoClose", mAutoClose).toBool();
    mAutoHints = mSettings.value("AutoHints", mAutoHints).toBool();
    mHintsDelay = mSettings.value("HintsDelay", mHintsDelay).toInt();

    mSolverStrength = mSettings.value("SolverStrength", mSolverStrength).toInt();

    mSettings.endGroup();
    Validate();
}

void Settings::Validate()
{
    if (mIndicatorType != INDICATOR_LETTER && mIndicatorType != INDICATOR_NUMBER)
    {
        mIndicatorType = INDICATOR_LETTER;
    }
    if (mPegCnt < 2 || mPegCnt > 5)
    {
        mPegCnt = 4;
    }
    if (mColorCnt < 2 || mColorCnt > 10)
    {
        mColorCnt = 8;
    }
    if (mGameMode != ColorCode::MODE_HVM && mGameMode != ColorCode::MODE_MVH)
    {
        mGameMode = ColorCode::MODE_HVM;
    }
    if (mSolverStrength < CCSolver::STRENGTH_LOW || mSolverStrength > CCSolver::STRENGTH_HIGH)
    {
        mSolverStrength = CCSolver::STRENGTH_HIGH;
    }
    if (mHintsDelay > 10000)
    {
        mHintsDelay = 10000;
    }
    else if (mHintsDelay < 0)
    {
        mHintsDelay = 0;
    }
}

void Settings::WriteSettings()
{
    mSettings.beginGroup("Appearance");
    mSettings.setValue("ShowToolBar", mShowToolBar);
    mSettings.setValue("ShowMenuBar", mShowMenuBar);
    mSettings.setValue("ShowStatusBar", mShowStatusBar);

    mSettings.setValue("ShowIndicators", mShowIndicators);
    mSettings.setValue("IndicatorType", mIndicatorType);
    mSettings.setValue("HideColors", mHideColors);
    mSettings.endGroup();

    mSettings.beginGroup("Game");
    mSettings.setValue("ColumnCount", mPegCnt);
    mSettings.setValue("ColorCount", mColorCnt);
    mSettings.setValue("SameColors", mSameColors);

    mSettings.setValue("GameMode", mGameMode);
    mSettings.setValue("AutoClose", mAutoClose);
    mSettings.setValue("AutoHints", mAutoHints);
    mSettings.setValue("HintsDelay", mHintsDelay);

    mSettings.setValue("SolverStrength", mSolverStrength);
    mSettings.endGroup();

    mSettings.sync();
}


