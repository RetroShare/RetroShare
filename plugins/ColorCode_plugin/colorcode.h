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

#ifndef COLORCODE_H
#define COLORCODE_H

#include <iostream>
#include <QMainWindow>

struct PegType
{
    int ix;
    char let;
    QRadialGradient* grad;
    QColor  pencolor;
};

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QGraphicsItem;
class QGraphicsTextItem;
class QGraphicsScene;
class QGraphicsView;
class CCSolver;
class ColorPeg;
class PegRow;
class RowHint;
class Msg;
class BackGround;
class SolRow;

class ColorCode : public QMainWindow
{
    Q_OBJECT

    public:
        ColorCode();
        ~ColorCode();

        static const int STATE_RUNNING;
        static const int STATE_WON;
        static const int STATE_LOST;
        static const int STATE_GAVE_UP;

        static const int MAX_COLOR_CNT;

        int ROW_CNT;
        int ROW_Y0;

    public slots:
        void PegPressSlot(ColorPeg *cp);
        void PegSortSlot(ColorPeg* cp);
        void PegReleasedSlot(ColorPeg *cp);
        void RowSolutionSlot(int ix);
        void HintPressedSlot(int ix);
        void ShowMsgSlot(QString msg);
        void RemovePegSlot(ColorPeg* cp);

    protected :
        void resizeEvent(QResizeEvent* e);
        void contextMenuEvent(QContextMenuEvent* e);
        void keyPressEvent(QKeyEvent *e);

    private slots:
        void InitRows();
        void InitPegBtns();
        void NewGameSlot();
        void RestartGameSlot();
        void GiveInSlot();
        void ResetRows();
        void OnlineHelpSlot();
        void AboutSlot();
        void AboutQtSlot();
        void ShowToolbarSlot();
        void ShowMenubarSlot();
        void ShowStatusbarSlot();
        void ResetColorsOrderSlot();
        void ShowLetterSlot();
        void SameColorSlot(bool checked);
        void SetSameColor(bool checked);
        void AutoCloseSlot();
        void ForceLevelSlot();

        void RandRowSlot();
        void PrevRowSlot();
        void ClearRowSlot();

        void mColorCntChangedSlot();
        void PegCntChangedSlot();

        void UpdateRowMenuSlot();

        void GetGuessSlot();

        void TestSlot();

    private:
        static const int mLevelSettings[5][3];

        static int mLevel;
        static int mColorCnt;
        static int mPegCnt;
        static int mDoubles;
        static int mMaxZ;
        static int mXOffs;

        volatile static bool mNoAct;

        int mGameState;
        int mGameId;
        int mGuessCnt;

        std::vector<int> mSolution;

        QGraphicsScene* scene;
        QGraphicsView* view;
        QSize* mOrigSize;

        CCSolver* mSolver;
        Msg* mMsg;
        BackGround* mBg;

        PegRow* mCurRow;

        std::vector<ColorPeg *> mAllPegs;
        std::vector<ColorPeg *> mPegBuff;
        SolRow* mSolRow;
        PegRow* mPegRows[10];
        RowHint* mHintBtns[10];

        PegType* mTypesMap[10];
        QRadialGradient mGradMap[10];
        QRadialGradient mGradBuff[10];
        ColorPeg* mPegBtns[10];
        ColorPeg** mSolPegs;
        QPoint mBtnPos[10];

        void InitTypesMap();
        void InitSolution();
        void InitActions();
        void InitMenus();
        void InitToolBars();

        void SetPegCnt();
        void SetColorCnt();
        void CheckLevel();
        void CheckSameColorsSetting();
        void SetState(const int s);
        void ResetGame();
        void NewGame();
        void SetSolution();
        void ShowSolution();
        void NextRow();
        void ResolveRow();
        void ResolveGame();
        void Scale();

        ColorPeg* CreatePeg(int ix);
        void RemovePeg(ColorPeg* cp);

        QMenu* GameMenu;
        QMenu* RowMenu;
        QMenu* SettingsMenu;
        QMenu* LevelMenu;
        QMenu* HelpMenu;
        QMenu* RowContextMenu;
        QMenuBar* mMenuBar;
        QToolBar* mGameToolbar;
        QToolBar* mLevelToolbar;

        QComboBox* mColorCntCmb;
        QComboBox* mPegCntCmb;
        QLabel* mStatusLabel;

        QActionGroup* mLevelActions;
        QAction* NewGameAction;
        QAction* RestartGameAction;
        QAction* GiveInAction;
        QAction* ShowToolbarAction;
        QAction* ShowMenubarAction;
        QAction* ShowStatusbarAction;
        QAction* SameColorAction;
        QAction* mActResetColorsOrder;
        QAction* mActShowLetter;
        QAction* mActSameColorIcon;
        QAction* AutoCloseAction;
        QAction* AboutAction;
        QAction* AboutQtAction;
        QAction* ExitAction;

        QAction* mActSetPegCnt;

        QAction* mActLevelEasy;
        QAction* mActLevelClassic;
        QAction* mActLevelMedium;
        QAction* mActLevelChallenging;
        QAction* mActLevelHard;
        QAction* mLaunchHelpAction;

        QAction* RandRowAction;
        QAction* PrevRowAction;
        QAction* ClearRowAction;

        QAction* mActGetGuess;
};

#endif // COLORCODE_H
