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

#ifndef PREFDIALOG_H
#define PREFDIALOG_H

#include "ui_prefdialog.h"
#include "settings.h"
#include "colorcode.h"
#include "ccsolver.h"
#include <iostream>
#include <QDialog>

class PrefDialog : public QDialog, public Ui::PrefDialog
{
    Q_OBJECT

    public:
        PrefDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
        ~PrefDialog();

        void InitSettings(Settings* set);
        void SetSettings();
        virtual QSize sizeHint () const;

    signals:
        void ResetColorOrderSignal();

    private slots:
        void ApplySlot();
        void RestoreDefSlot();
        void CancelSlot();
        void OkSlot();

        void LevelPresetChangedSlot();
        void ColorCntChangedSlot();
        void PegCntChangedSlot();
        void SameColorsChangedSlot();
        void ShowIndicatorsChangedSlot();
        void ResetColorOrderSlot();

    private:
        bool SetSuppressSlots(bool b, bool force = false);
        void InitControls();
        void CheckLevelPresets();
        void CheckIndicators();
        void ApplySettings();

        QPushButton* mRestoreBtn;
        QPushButton* mOkBtn;
        QPushButton* mApplyBtn;
        QPushButton* mCancelBtn;

        Settings* mSettings;

        bool mSuppressSlots;

};

#endif // PREFDIALOG_H
