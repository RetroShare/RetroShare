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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <iostream>

class Settings
{
    public:
        Settings();
        ~Settings();

        static const int INDICATOR_LETTER;
        static const int INDICATOR_NUMBER;

        void InitSettings();
        void ReadSettings();
        void Validate();
        void SaveLastSettings();
        void RestoreLastSettings();
        void RestoreDefSettings();
        void WriteSettings();

        QSettings mSettings;

        bool mShowToolBar;
        bool mShowMenuBar;
        bool mShowStatusBar;
        bool mShowIndicators;
        int mIndicatorType;
        bool mHideColors;

        int mPegCnt;
        int mColorCnt;
        bool mSameColors;
        int mGameMode;
        bool mAutoClose;
        bool mAutoHints;
        int mSolverStrength;
        int mHintsDelay;

    private:
        void RestoreSettingsMap(QMap<QString, QVariant> &map);
        void SaveSettingsMap(QMap<QString, QVariant> &map);
        QMap<QString, QVariant> mDefMap;
        QMap<QString, QVariant> mLastMap;
};

#endif // SETTINGS_H
