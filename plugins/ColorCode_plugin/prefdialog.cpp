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

#include "prefdialog.h"

using namespace std;

PrefDialog::PrefDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
{
    setupUi(this);
    setSizeGripEnabled(false);
    mSuppressSlots = false;
    InitControls();
}

PrefDialog::~PrefDialog()
{
}

void PrefDialog::InitControls()
{
    mLevelPresetsCmb->addItem(tr("... Select from predefined level settings"), -1);
    mLevelPresetsCmb->addItem(tr("Beginner (2 Colors, 2 Slots, Doubles)"), 0);
    mLevelPresetsCmb->addItem(tr("Easy (4 Colors, 3 Slots, No Doubles)"), 1);
    mLevelPresetsCmb->addItem(tr("Classic (6 Colors, 4 Slots, Doubles)"), 2);
    mLevelPresetsCmb->addItem(tr("Challenging (8 Colors, 4 Slots, Doubles)"), 3);
    mLevelPresetsCmb->addItem(tr("Hard (10 Colors, 5 Slots, Doubles)"), 4);

    mPegCntCmb->addItem("2 " + tr("Slots"), 2);
    mPegCntCmb->addItem("3 " + tr("Slots"), 3);
    mPegCntCmb->addItem("4 " + tr("Slots"), 4);
    mPegCntCmb->addItem("5 " + tr("Slots"), 5);

    mColorCntCmb->addItem("2 " + tr("Colors"), 2);
    mColorCntCmb->addItem("3 " + tr("Colors"), 3);
    mColorCntCmb->addItem("4 " + tr("Colors"), 4);
    mColorCntCmb->addItem("5 " + tr("Colors"), 5);
    mColorCntCmb->addItem("6 " + tr("Colors"), 6);
    mColorCntCmb->addItem("7 " + tr("Colors"), 7);
    mColorCntCmb->addItem("8 " + tr("Colors"), 8);
    mColorCntCmb->addItem("9 " + tr("Colors"), 9);
    mColorCntCmb->addItem("10 " + tr("Colors"), 10);

    mGameModeCmb->addItem(tr("Human vs. Computer"), ColorCode::MODE_HVM);
    mGameModeCmb->addItem(tr("Computer vs. Human"), ColorCode::MODE_MVH);

    mStrengthCmb->addItem(tr("Low"), CCSolver::STRENGTH_LOW);
    mStrengthCmb->addItem(tr("Medium"), CCSolver::STRENGTH_MEDIUM);
    mStrengthCmb->addItem(tr("High"), CCSolver::STRENGTH_HIGH);
    mDelaySb->setMaximum(10000);
    mDelaySb->setMinimum(0);
    mDelaySb->setSingleStep(100);

    mRestoreBtn = mButtonBox->button(QDialogButtonBox::RestoreDefaults);
    mOkBtn = mButtonBox->button(QDialogButtonBox::Ok);
    mApplyBtn = mButtonBox->button(QDialogButtonBox::Apply);
    mCancelBtn = mButtonBox->button(QDialogButtonBox::Cancel);

    connect(mLevelPresetsCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(LevelPresetChangedSlot()));
    connect(mColorCntCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(ColorCntChangedSlot()));
    connect(mPegCntCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(PegCntChangedSlot()));
    connect(mSameColorsCb, SIGNAL(stateChanged(int)), this, SLOT(SameColorsChangedSlot()));
    connect(mShowIndicatorsCb, SIGNAL(stateChanged(int)), this, SLOT(ShowIndicatorsChangedSlot()));

    connect(mResetColorOrderBtn, SIGNAL(clicked()), this, SLOT(ResetColorOrderSlot()));

    connect(mRestoreBtn, SIGNAL(clicked()), this, SLOT(RestoreDefSlot()));
    connect(mApplyBtn, SIGNAL(clicked()), this, SLOT(ApplySlot()));
    connect(mCancelBtn, SIGNAL(clicked()), this, SLOT(CancelSlot()));
    connect(mOkBtn, SIGNAL(clicked()), this, SLOT(OkSlot()));
}

void PrefDialog::InitSettings(Settings* set)
{
    mSettings = set;
    SetSettings();
}

void PrefDialog::SetSettings()
{
    bool sup = SetSuppressSlots(true);

    mMenuBarCb->setChecked(mSettings->mShowMenuBar);
    mStatusBarCb->setChecked(mSettings->mShowStatusBar);
    mToolBarCb->setChecked(mSettings->mShowToolBar);

    mShowIndicatorsCb->setChecked(mSettings->mShowIndicators);
    if (mSettings->mIndicatorType == Settings::INDICATOR_NUMBER)
    {
        mNumbersRb->setChecked(true);
    }
    else
    {
        mLettersRb->setChecked(true);
    }
    mHideColorsCb->setChecked(mSettings->mHideColors);

    CheckIndicators();

    int i;
    i = mColorCntCmb->findData(mSettings->mColorCnt);
    if (i != -1 && mColorCntCmb->currentIndex() != i)
    {
        mColorCntCmb->setCurrentIndex(i);
    }
    i = mPegCntCmb->findData(mSettings->mPegCnt);
    if (i != -1 && mPegCntCmb->currentIndex() != i)
    {
        mPegCntCmb->setCurrentIndex(i);
    }

    mSameColorsCb->setChecked(mSettings->mSameColors);
    CheckLevelPresets();

    i = mGameModeCmb->findData(mSettings->mGameMode);
    if (i != -1 && mGameModeCmb->currentIndex() != i)
    {
        mGameModeCmb->setCurrentIndex(i);
    }

    i = mStrengthCmb->findData(mSettings->mSolverStrength);
    if (i != -1 && mStrengthCmb->currentIndex() != i)
    {
        mStrengthCmb->setCurrentIndex(i);
    }

    mAutoCloseCb->setChecked(mSettings->mAutoClose);
    mAutoHintsCb->setChecked(mSettings->mAutoHints);

    mDelaySb->setValue(mSettings->mHintsDelay);

    if (sup)
    {
        SetSuppressSlots(false);
    }
}

void PrefDialog::LevelPresetChangedSlot()
{
    int ix, i;
    ix = mLevelPresetsCmb->itemData(mLevelPresetsCmb->currentIndex()).toInt();

    if (ix < 0 || ix > 4)
    {
        return;
    }

    bool sup = SetSuppressSlots(true);

    i = mColorCntCmb->findData(ColorCode::LEVEL_SETTINGS[ix][0]);
    if (i != -1 && mColorCntCmb->currentIndex() != i)
    {
        mColorCntCmb->setCurrentIndex(i);
    }
    i = mPegCntCmb->findData(ColorCode::LEVEL_SETTINGS[ix][1]);
    if (i != -1 && mPegCntCmb->currentIndex() != i)
    {
        mPegCntCmb->setCurrentIndex(i);
    }

    mSameColorsCb->setChecked((ColorCode::LEVEL_SETTINGS[ix][2] == 1));

    if (sup)
    {
        SetSuppressSlots(false);
    }
}

void PrefDialog::CheckLevelPresets()
{
    int ix = -1;
    for (int i = 0; i < 5; ++i)
    {
        if ( ColorCode::LEVEL_SETTINGS[i][0] == mColorCntCmb->itemData(mColorCntCmb->currentIndex()).toInt()
             && ColorCode::LEVEL_SETTINGS[i][1] == mPegCntCmb->itemData(mPegCntCmb->currentIndex()).toInt()
             && ColorCode::LEVEL_SETTINGS[i][2] == (int) mSameColorsCb->isChecked() )
        {
            ix = i;
            break;
        }
    }

    mLevelPresetsCmb->setCurrentIndex(mLevelPresetsCmb->findData(ix));
}

void PrefDialog::ColorCntChangedSlot()
{
    if (!mSuppressSlots)
    {
        CheckLevelPresets();
    }
}

void PrefDialog::PegCntChangedSlot()
{
    if (!mSuppressSlots)
    {
        CheckLevelPresets();
    }
}

void PrefDialog::SameColorsChangedSlot()
{
    if (!mSuppressSlots)
    {
        CheckLevelPresets();
    }
}

void PrefDialog::ShowIndicatorsChangedSlot()
{
    if (!mSuppressSlots)
    {
        CheckIndicators();
    }
}

void PrefDialog::CheckIndicators()
{

}

void PrefDialog::ResetColorOrderSlot()
{
    emit ResetColorOrderSignal();
}

void PrefDialog::RestoreDefSlot()
{
    mSettings->RestoreDefSettings();
    SetSettings();
}

void PrefDialog::ApplySlot()
{
    ApplySettings();
    setResult(QDialog::Accepted);
    hide();
}

void PrefDialog::OkSlot()
{
    ApplySettings();
    setResult(QDialog::Accepted);
    hide();
}

void PrefDialog::CancelSlot()
{
    setResult(QDialog::Rejected);
    hide();
}

bool PrefDialog::SetSuppressSlots(bool b, bool force)
{
    if (mSuppressSlots == b && !force)
    {
        return false;
    }
    mSuppressSlots = b;
    return true;
}

void PrefDialog::ApplySettings()
{
    mSettings->mShowMenuBar = mMenuBarCb->isChecked();
    mSettings->mShowStatusBar = mStatusBarCb->isChecked();
    mSettings->mShowToolBar = mToolBarCb->isChecked();

    mSettings->mShowIndicators = mShowIndicatorsCb->isChecked();

    if (mNumbersRb->isChecked())
    {
        mSettings->mIndicatorType = Settings::INDICATOR_NUMBER;
    }
    else
    {
        mSettings->mIndicatorType = Settings::INDICATOR_LETTER;
    }
    mSettings->mHideColors = mHideColorsCb->isChecked();

    mSettings->mColorCnt = mColorCntCmb->itemData(mColorCntCmb->currentIndex()).toInt();
    mSettings->mPegCnt = mPegCntCmb->itemData(mPegCntCmb->currentIndex()).toInt();
    mSettings->mSameColors = mSameColorsCb->isChecked();

    mSettings->mGameMode = mGameModeCmb->itemData(mGameModeCmb->currentIndex()).toInt();
    mSettings->mSolverStrength = mStrengthCmb->itemData(mStrengthCmb->currentIndex()).toInt();

    mSettings->mAutoClose = mAutoCloseCb->isChecked();
    mSettings->mAutoHints = mAutoHintsCb->isChecked();
    mSettings->mHintsDelay = mDelaySb->value();

    mSettings->Validate();
}

QSize PrefDialog::sizeHint () const
{
    return QSize(500, 400);
}
