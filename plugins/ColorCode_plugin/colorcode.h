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
class PrefDialog;
class Settings;
class CCSolver;
class ColorPeg;
class PegRow;
class RowHint;
class Msg;
class BackGround;
class SolutionRow;
class GraphicsBtn;

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
        static const int STATE_ERROR;

        static const int MODE_HVM;
        static const int MODE_MVH;

        static const int MAX_COLOR_CNT;

        static const int LEVEL_SETTINGS[5][3];

        int ROW_CNT;
        int ROW_Y0;

        Settings* mSettings;

    public slots:
        void PegPressSlot(ColorPeg *cp);
        void PegSortSlot(ColorPeg* cp);
        void PegReleasedSlot(ColorPeg *cp);
        void RowSolutionSlot(int ix);
        void HintPressedSlot(int ix);
        void ShowMsgSlot(QString msg);
        void RemovePegSlot(ColorPeg* cp);

        void DoneBtnPressSlot(GraphicsBtn* btn = NULL);

        void ApplyPreferencesSlot();

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
        void OpenPreferencesSlot();
        void ShowToolbarSlot();
        void ShowMenubarSlot();
        void ShowStatusbarSlot();
        void ResetColorsOrderSlot();
        void SetIndicators();
        void SameColorSlot(bool checked);
        void SetSameColor(bool checked);
        void AutoCloseSlot();
        void AutoHintsSlot();
        void SetLevelSlot();
        void SetGameModeSlot();

        void RandRowSlot();
        void PrevRowSlot();
        void ClearRowSlot();

        void ColorCntChangedSlot();
        void PegCntChangedSlot();

        void UpdateRowMenuSlot();

        void SetGuessSlot();
        void SetHintsSlot();
        void SetAutoHintsSlot();

        void TestSlot();

    private:
        static const int LAYER_BG;
        static const int LAYER_ROWS;
        static const int LAYER_HINTS;
        static const int LAYER_SOL;
        static const int LAYER_MSG;
        static const int LAYER_PEGS;
        static const int LAYER_BTNS;
        static const int LAYER_DRAG;

        static int mGameMode;
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
        int mGameCnt;
        bool mHideColors;
        int mSolverStrength;

        std::vector<int> mSolution;

        QGraphicsScene* scene;
        QGraphicsView* view;
        QSize* mOrigSize;

        CCSolver* mSolver;
        Msg* mMsg;
        BackGround* mBg;
        GraphicsBtn* mDoneBtn;
        GraphicsBtn* mOkBtn;
        PrefDialog* mPrefDialog;

        PegRow* mCurRow;

        std::vector<ColorPeg *> mAllPegs;
        std::vector<ColorPeg *> mPegBuff;
        SolutionRow* mSolutionRow;
        PegRow* mPegRows[10];
        RowHint* mHintBtns[10];

        PegType* mTypesMap[10];
        QRadialGradient mGradMap[10];
        QRadialGradient mGradBuff[10];
        ColorPeg* mPegBtns[10];
        QPoint mBtnPos[10];

        QTimer* mHintsDelayTimer;

        void InitTypesMap();
        void InitSolution();
        void InitActions();
        void InitMenus();
        void InitToolBars();
        void CreatePrefDialog();
        void ApplySettings();
        bool NeedsRestart() const;
        void TryNewGame();

        void SetGameMode();
        void SetPegCnt();
        void SetColorCnt();

        void ApplyGameMode();
        void ApplyPegCnt();
        void ApplyColorCnt();
        void ApplySolverStrength();
        
        void CheckLevel();
        void CheckSameColorsSetting();
        void SetState(const int s);
        void ResetGame();
        void NewGame();
        void SetSolution();
        void GetSolution();
        void ShowSolution();
        void NextRow();
        void ResolveRow();
        std::vector<int> RateSol2Guess(const std::vector<int> sol, const std::vector<int> guess);
        void ResolveHints();
        void ResolveGame();
        bool GamesRunning();
        void Scale();

        ColorPeg* CreatePeg(int ix);
        void RemovePeg(ColorPeg* cp);

        QMenu* mMenuGame;
        QMenu* mMenuRow;
        QMenu* mMenuRowContext;
        QMenu* mMenuSettings;
        QMenu* mMenuModes;
        QMenu* mMenuLevels;
        QMenu* mMenuHelp;        

        QMenuBar* mMenuBar;
        QToolBar* mGameToolbar;
        QToolBar* mLevelToolbar;

        QComboBox* mColorCntCmb;
        QComboBox* mPegCntCmb;
        QLabel* mStatusLabel;

        QActionGroup* mActGroupLevels;
        QActionGroup* mActGroupModes;

        QAction* mActNewGame;
        QAction* mActRestartGame;
        QAction* mActGiveIn;
        QAction* mActShowToolbar;
        QAction* mActShowMenubar;
        QAction* mActShowStatusbar;
        QAction* mActSameColor;
        QAction* mActResetColorsOrder;
        QAction* mActShowLetter;
        QAction* mActSameColorIcon;
        QAction* mActAutoClose;
        QAction* mActAutoHints;
        QAction* mActPreferences;
        QAction* mActAbout;
        QAction* mActAboutQt;
        QAction* mActExit;

        QAction* mActSetPegCnt;

        QAction* mActModeHvM;
        QAction* mActModeMvH;

        QAction* mActLevelEasy;
        QAction* mActLevelClassic;
        QAction* mActLevelMedium;
        QAction* mActLevelChallenging;
        QAction* mActLevelHard;
        QAction* mLaunchHelpAction;

        QAction* mActRandRow;
        QAction* mActPrevRow;
        QAction* mActClearRow;

        QAction* mActSetGuess;
        QAction* mActSetHints;
};

#endif // COLORCODE_H
