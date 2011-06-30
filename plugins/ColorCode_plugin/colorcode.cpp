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

#include <QtGui>

#include "settings.h"
#include "ccsolver.h"
#include "colorcode.h"
#include "colorpeg.h"
#include "pegrow.h"
#include "rowhint.h"
#include "msg.h"
#include "background.h"
#include "about.h"
#include "prefdialog.h"
#include "solutionrow.h"
#include "graphicsbtn.h"

using namespace std;

const int IdRole = Qt::UserRole;

volatile bool ColorCode::mNoAct    = false;

const int ColorCode::STATE_RUNNING = 0;
const int ColorCode::STATE_WON     = 1;
const int ColorCode::STATE_LOST    = 2;
const int ColorCode::STATE_GAVE_UP = 3;
const int ColorCode::STATE_ERROR   = 4;

const int ColorCode::MODE_HVM = 0;
const int ColorCode::MODE_MVH = 1;

const int ColorCode::MAX_COLOR_CNT = 10;

const int ColorCode::LAYER_BG    = 1;
const int ColorCode::LAYER_ROWS  = 2;
const int ColorCode::LAYER_HINTS = 3;
const int ColorCode::LAYER_SOL   = 4;
const int ColorCode::LAYER_MSG   = 5;
const int ColorCode::LAYER_PEGS  = 6;
const int ColorCode::LAYER_BTNS  = 7;
const int ColorCode::LAYER_DRAG  = 8;

const int ColorCode::LEVEL_SETTINGS[5][3] = {
                                                { 2, 2, 1},
                                                { 4, 3, 0},
                                                { 6, 4, 1},
                                                { 8, 4, 1},
                                                {10, 5, 1}
                                            };

int ColorCode::mColorCnt = 0;
int ColorCode::mPegCnt   = 0;
int ColorCode::mDoubles  = 1;
int ColorCode::mXOffs    = 0;
int ColorCode::mMaxZ     = 0;
int ColorCode::mLevel    = -1;
int ColorCode::mGameMode = ColorCode::MODE_HVM;

ColorCode::ColorCode()
{
    QTime midnight(0, 0, 0);
    qsrand(midnight.secsTo(QTime::currentTime()));

    QCoreApplication::setOrganizationName("dirks");
    QCoreApplication::setOrganizationDomain("laebisch.com");
    QCoreApplication::setApplicationName("colorcode");

    mPrefDialog = NULL;
    mSettings = new Settings();

    ROW_CNT = 10;
    ROW_Y0  = 130;

    mOrigSize = NULL;

    setWindowTitle(tr("ColorCode"));
    setWindowIcon(QIcon(QPixmap(":/img/cc16.png")));
    setIconSize(QSize(16, 16));

    mMenuBar = menuBar();

    // as long as menuBar's size isn invalid populated initially ...
#ifdef Q_WS_X11
    scene = new QGraphicsScene(0, 0, 320, 580);
#else
    scene = new QGraphicsScene(0, 0, 320, 560);
#endif

    scene->setBackgroundBrush(QBrush(QColor("#b0b1b3")));

    view = new QGraphicsView;
    view->setScene(scene);
    view->setGeometry(0, 0, 320, 560);
    view->setDragMode(QGraphicsView::NoDrag);
    view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    view->setContextMenuPolicy(Qt::NoContextMenu);
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setCentralWidget(view);

    mBg = new BackGround();
    scene->addItem(mBg);
    mBg->setPos(0, 0);
    mBg->setZValue(LAYER_BG);

    mMsg = new Msg;
    scene->addItem(mMsg);
    mMsg->setPos(20, 0);
    mMsg->setZValue(LAYER_MSG);

    mDoneBtn = new GraphicsBtn;
    mDoneBtn->SetLabel(tr("Done"));
    scene->addItem(mDoneBtn);
    mDoneBtn->setPos(84, 118);
    mDoneBtn->setZValue(LAYER_BTNS);
    mDoneBtn->ShowBtn(false);
    connect(mDoneBtn, SIGNAL(BtnPressSignal(GraphicsBtn *)), this, SLOT(DoneBtnPressSlot(GraphicsBtn *)));

    mOkBtn = new GraphicsBtn;
    mOkBtn->SetWidth(38);
    mOkBtn->SetLabel("ok");
    scene->addItem(mOkBtn);
    mOkBtn->setPos(5, 329);
    mOkBtn->setZValue(LAYER_BTNS);
    mOkBtn->ShowBtn(false);
    connect(mOkBtn, SIGNAL(BtnPressSignal(GraphicsBtn *)), this, SLOT(DoneBtnPressSlot(GraphicsBtn *)));

    mSolver = new CCSolver(this);
    mHintsDelayTimer = new QTimer();
    mHintsDelayTimer->setSingleShot(true);
    mHintsDelayTimer->setInterval(500);
    connect(mHintsDelayTimer, SIGNAL(timeout()), this, SLOT(SetAutoHintsSlot()));

    mCurRow         = NULL;
    mColorCnt       = 0;
    mGameCnt        = 0;
    mGameId         = 0;
    mGuessCnt       = 0;
    mHideColors     = false;
    mSolverStrength = CCSolver::STRENGTH_HIGH;

    InitTypesMap();
    InitActions();
    InitMenus();
    InitToolBars();

    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
    mStatusLabel = new QLabel();
    statusBar()->addPermanentWidget(mStatusLabel, 0);

    InitSolution();
    InitRows();
    InitPegBtns();

    ApplySettings();

    NewGame();
}

ColorCode::~ColorCode()
{
    mSettings->WriteSettings();
}

void ColorCode::InitSolution()
{
    mSolutionRow = new SolutionRow();
    mSolutionRow->setPos(50, 70);
    mSolutionRow->setZValue(LAYER_SOL);
    scene->addItem(mSolutionRow);
    connect(mSolutionRow, SIGNAL(RemovePegSignal(ColorPeg*)), this, SLOT(RemovePegSlot(ColorPeg*)));
    connect(mSolutionRow, SIGNAL(RowSolutionSignal(int)), this, SLOT(RowSolutionSlot(int)));
}

void ColorCode::InitRows()
{
    PegRow* row;
    RowHint* hint;

    int xpos = 60;
    int ypos = ROW_Y0;
    int i;

    for (i = 0; i < ROW_CNT; ++i)
    {
        row = new PegRow();
        row->SetIx(i);
        row->setPos(QPoint(xpos, ypos + (ROW_CNT - (i + 1)) * 40));
        row->setZValue(LAYER_ROWS);
        row->SetActive(false);
        scene->addItem(row);
        mPegRows[i] = row;
        connect(row, SIGNAL(RemovePegSignal(ColorPeg*)), this, SLOT(RemovePegSlot(ColorPeg*)));
        connect(row, SIGNAL(RowSolutionSignal(int)), this, SLOT(RowSolutionSlot(int)));

        hint = new RowHint;
        hint->SetIx(i);
        hint->setPos(QPoint(4, ypos + (ROW_CNT - (i + 1)) * 40));
        hint->setZValue(LAYER_HINTS);
        hint->SetActive(false);
        scene->addItem(hint);
        mHintBtns[i] = hint;
        connect(hint, SIGNAL(HintPressedSignal(int)), this, SLOT(HintPressedSlot(int)));
    }
}

void ColorCode::InitPegBtns()
{
    for (int i = 0; i < MAX_COLOR_CNT; ++i)
    {
        mPegBtns[i] = NULL;
    }
}

void ColorCode::InitTypesMap()
{
    int ix;

    QRadialGradient grad = QRadialGradient(20, 20, 40, 5, 5);
    QRadialGradient grad2 = QRadialGradient(18, 18, 18, 12, 12);

    ix = 0;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FFFF80"));
    grad.setColorAt(1.0, QColor("#C05800"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 1;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FF3300"));
    grad.setColorAt(1.0, QColor("#400040"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 2;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#33CCFF"));
    grad.setColorAt(1.0, QColor("#000080"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 3;
    mTypesMap[ix] = new PegType;
    grad2.setColorAt(0.0, Qt::white);
    grad2.setColorAt(0.5, QColor("#f8f8f0"));
    grad2.setColorAt(0.7, QColor("#f0f0f0"));
    grad2.setColorAt(1.0, QColor("#d0d0d0"));
    mGradMap[ix] = grad2;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 4;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#808080"));
    grad.setColorAt(1.0, Qt::black);
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 5;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#66FF33"));
    grad.setColorAt(1.0, QColor("#385009"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 6;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FF9900"));
    grad.setColorAt(1.0, QColor("#A82A00"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 7;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#BA88FF"));
    grad.setColorAt(1.0, QColor("#38005D"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 8;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#00FFFF"));
    grad.setColorAt(1.0, QColor("#004040"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 9;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FFC0FF"));
    grad.setColorAt(1.0, QColor("#800080"));
    mGradMap[ix] = grad;
    mTypesMap[ix]->grad = &mGradMap[ix];
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    for (ix = 0; ix < MAX_COLOR_CNT; ++ix)
    {
        mTypesMap[ix]->let = 'A' + ix;
        mGradBuff[ix] = mGradMap[ix];
    }
}

void ColorCode::InitActions()
{
    mActNewGame = new QAction(tr("&New Game"), this);
    mActNewGame->setIcon(QIcon(":/img/document-new.png"));
    mActNewGame->setShortcut(tr("Ctrl+N"));
    connect(mActNewGame, SIGNAL(triggered()), this, SLOT(NewGameSlot()));

    mActRestartGame = new QAction(tr("&Restart Game"), this);
    mActRestartGame->setIcon(QIcon(":/img/view-refresh.png"));
    mActRestartGame->setShortcut(tr("Ctrl+Shift+N"));
    connect(mActRestartGame, SIGNAL(triggered()), this, SLOT(RestartGameSlot()));

    mActGiveIn = new QAction(tr("&Throw In The Towel"), this);
    mActGiveIn->setIcon(QIcon(":/img/face-sad.png"));
    mActGiveIn->setShortcut(tr("Ctrl+G"));
    connect(mActGiveIn, SIGNAL(triggered()), this, SLOT(GiveInSlot()));

    mActExit = new QAction(tr("E&xit"), this);
    mActExit->setIcon(QIcon(":/img/application-exit.png"));
    mActExit->setShortcut(tr("Ctrl+Q"));
    connect(mActExit, SIGNAL(triggered()), this, SLOT(close()));

    mActShowToolbar = new QAction(tr("Show Toolbar"), this);
    mActShowToolbar->setCheckable(true);
    mActShowToolbar->setChecked(true);
    mActShowToolbar->setShortcut(tr("Ctrl+T"));
    connect(mActShowToolbar, SIGNAL(triggered()), this, SLOT(ShowToolbarSlot()));

    mActShowMenubar = new QAction(tr("Show Menubar"), this);
    mActShowMenubar->setCheckable(true);
    mActShowMenubar->setChecked(true);
    mActShowMenubar->setShortcut(tr("Ctrl+M"));
    connect(mActShowMenubar, SIGNAL(triggered()), this, SLOT(ShowMenubarSlot()));

    mActShowStatusbar = new QAction(tr("Show Statusbar"), this);
    mActShowStatusbar->setCheckable(true);
    mActShowStatusbar->setChecked(true);
    mActShowStatusbar->setShortcut(tr("Ctrl+S"));
    connect(mActShowStatusbar, SIGNAL(triggered()), this, SLOT(ShowStatusbarSlot()));

    mActResetColorsOrder = new QAction(tr("Reset Color Order"), this);
    mActResetColorsOrder->setShortcut(tr("Ctrl+Shift+R"));
    connect(mActResetColorsOrder, SIGNAL(triggered()), this, SLOT(ResetColorsOrderSlot()));

    mActShowLetter = new QAction(tr("Show Indicators"), this);
    mActShowLetter->setCheckable(true);
    mActShowLetter->setChecked(false);
    mActShowLetter->setShortcut(tr("Ctrl+Shift+L"));
    connect(mActShowLetter, SIGNAL(triggered()), this, SLOT(SetIndicators()));

    mActSameColor = new QAction(tr("Allow Pegs of the Same Color"), this);
    mActSameColor->setCheckable(true);
    mActSameColor->setChecked(true);
    mActSameColor->setShortcut(tr("Ctrl+Shift+C"));
    connect(mActSameColor, SIGNAL(triggered(bool)), this, SLOT(SameColorSlot(bool)));

    mActSameColorIcon = new QAction(tr("Allow Pegs of the Same Color"), this);
    mActSameColorIcon->setCheckable(true);
    mActSameColorIcon->setChecked(true);
    mActSameColorIcon->setToolTip(tr("Disallow Pegs of the Same Color"));
    mActSameColorIcon->setIcon(QIcon(":/img/same_color_1.png"));
    connect(mActSameColorIcon, SIGNAL(triggered(bool)), this, SLOT(SameColorSlot(bool)));

    mActAutoClose = new QAction(tr("Close Rows when the last Peg is placed"), this);
    mActAutoClose->setCheckable(true);
    mActAutoClose->setChecked(false);
    mActAutoClose->setShortcut(tr("Ctrl+L"));
    connect(mActAutoClose, SIGNAL(triggered()), this, SLOT(AutoCloseSlot()));

    mActAutoHints = new QAction(tr("Set Hints automatically"), this);
    mActAutoHints->setCheckable(true);
    mActAutoHints->setChecked(false);
    mActAutoHints->setShortcut(tr("Ctrl+Shift+H"));
    connect(mActAutoHints, SIGNAL(triggered()), this, SLOT(AutoHintsSlot()));

    mLaunchHelpAction = new QAction(tr("Online &Help"), this);
    mLaunchHelpAction->setIcon(QIcon(":/img/help.png"));
    mLaunchHelpAction->setShortcut(tr("F1"));
    connect(mLaunchHelpAction, SIGNAL(triggered()), this, SLOT(OnlineHelpSlot()));

    mActAbout = new QAction(tr("About &ColorCode"), this);
    mActAbout->setIcon(QIcon(":/img/help-about.png"));
    mActAbout->setShortcut(tr("Ctrl+A"));
    connect(mActAbout, SIGNAL(triggered()), this, SLOT(AboutSlot()));

    mActAboutQt = new QAction(tr("About &Qt"), this);
    mActAboutQt->setIcon(QIcon(":/img/qt.png"));
    mActAboutQt->setShortcut(tr("Ctrl+I"));
    connect(mActAboutQt, SIGNAL(triggered()), this, SLOT(AboutQtSlot()));

    mActRandRow = new QAction(tr("Fill Row by Random"), this);
    mActRandRow->setIcon(QIcon(":/img/system-switch-user.png"));
    mActRandRow->setShortcut(tr("Ctrl+R"));
    connect(mActRandRow, SIGNAL(triggered()), this, SLOT(RandRowSlot()));

    mActPrevRow = new QAction(tr("Duplicate Previous Row"), this);
    mActPrevRow->setIcon(QIcon(":/img/edit-copy.png"));
    mActPrevRow->setShortcut(tr("Ctrl+D"));
    connect(mActPrevRow, SIGNAL(triggered()), this, SLOT(PrevRowSlot()));

    mActClearRow = new QAction(tr("Clear Row"), this);
    mActClearRow->setIcon(QIcon(":/img/edit-clear.png"));
    mActClearRow->setShortcut(tr("Ctrl+C"));
    connect(mActClearRow, SIGNAL(triggered()), this, SLOT(ClearRowSlot()));

    mActLevelEasy = new QAction(tr("Beginner (2 Colors, 2 Slots, Doubles)"), this);
    mActLevelEasy->setData(0);
    mActLevelEasy->setCheckable(true);
    mActLevelEasy->setChecked(false);
    connect(mActLevelEasy, SIGNAL(triggered()), this, SLOT(SetLevelSlot()));

    mActLevelClassic = new QAction(tr("Easy (4 Colors, 3 Slots, No Doubles)"), this);
    mActLevelClassic->setData(1);
    mActLevelClassic->setCheckable(true);
    mActLevelClassic->setChecked(false);
    connect(mActLevelClassic, SIGNAL(triggered()), this, SLOT(SetLevelSlot()));

    mActLevelMedium = new QAction(tr("Classic (6 Colors, 4 Slots, Doubles)"), this);
    mActLevelMedium->setData(2);
    mActLevelMedium->setCheckable(true);
    mActLevelMedium->setChecked(true);
    connect(mActLevelMedium, SIGNAL(triggered()), this, SLOT(SetLevelSlot()));

    mActLevelChallenging = new QAction(tr("Challenging (8 Colors, 4 Slots, Doubles)"), this);
    mActLevelChallenging->setData(3);
    mActLevelChallenging->setCheckable(true);
    mActLevelChallenging->setChecked(false);
    connect(mActLevelChallenging, SIGNAL(triggered()), this, SLOT(SetLevelSlot()));

    mActLevelHard = new QAction(tr("Hard (10 Colors, 5 Slots, Doubles)"), this);
    mActLevelHard->setData(4);
    mActLevelHard->setCheckable(true);
    mActLevelHard->setChecked(false);
    connect(mActLevelHard, SIGNAL(triggered()), this, SLOT(SetLevelSlot()));

    mActSetGuess = new QAction(tr("Computer's Guess"), this);
    mActSetGuess->setIcon(QIcon(":/img/business_user.png"));
    mActSetGuess->setShortcut(tr("Ctrl+H"));
    connect(mActSetGuess, SIGNAL(triggered()), this, SLOT(SetGuessSlot()));

    mActSetHints = new QAction(tr("Rate it for me"), this);
    mActSetHints->setIcon(QIcon(":/img/icon_female16.png"));
    mActSetHints->setShortcut(tr("Ctrl+H"));
    connect(mActSetHints, SIGNAL(triggered()), this, SLOT(SetHintsSlot()));

    mActModeHvM = new QAction(tr("Human vs Computer"), this);
    mActModeHvM->setData(MODE_HVM);
    mActModeHvM->setCheckable(true);
    mActModeHvM->setChecked(true);
    connect(mActModeHvM, SIGNAL(triggered()), this, SLOT(SetGameModeSlot()));

    mActModeMvH = new QAction(tr("Computer vs Human"), this);
    mActModeMvH->setData(MODE_MVH);
    mActModeMvH->setCheckable(true);
    mActModeMvH->setChecked(false);
    connect(mActModeMvH, SIGNAL(triggered()), this, SLOT(SetGameModeSlot()));

    mActPreferences = new QAction(tr("Preferences"), this);
    mActPreferences->setIcon(QIcon(":/img/configure.png"));
    mActPreferences->setShortcut(tr("Ctrl+P"));
    connect(mActPreferences, SIGNAL(triggered()), this, SLOT(OpenPreferencesSlot()));
}

void ColorCode::InitMenus()
{
    mMenuGame = mMenuBar->addMenu(tr("&Game"));
    mMenuGame->addAction(mActNewGame);
    mMenuGame->addAction(mActRestartGame);
    mMenuGame->addAction(mActGiveIn);
    mMenuGame->addSeparator();
    mMenuGame->addAction(mActExit);

    mMenuRow = mMenuBar->addMenu(tr("&Row"));
    mMenuRow->addAction(mActRandRow);
    mMenuRow->addAction(mActPrevRow);
    mMenuRow->addAction(mActClearRow);
    mMenuRow->addSeparator();
    mMenuRow->addAction(mActSetGuess);
    mMenuRow->addAction(mActSetHints);
    connect(mMenuRow, SIGNAL(aboutToShow()), this, SLOT(UpdateRowMenuSlot()));

    mMenuSettings = mMenuBar->addMenu(tr("&Settings"));

    mMenuModes = mMenuSettings->addMenu(tr("Game Mode"));
    mActGroupModes = new QActionGroup(mMenuModes);
    mActGroupModes->setExclusive(true);
    mActGroupModes->addAction(mActModeHvM);
    mActGroupModes->addAction(mActModeMvH);
    QList<QAction *> modeacts = mActGroupModes->actions();
    mMenuModes->addActions(modeacts);
    mMenuSettings->addSeparator();

    mMenuSettings->addAction(mActShowMenubar);
    mMenuSettings->addAction(mActShowToolbar);
    mMenuSettings->addAction(mActShowStatusbar);
    mMenuSettings->addSeparator();
    mMenuSettings->addAction(mActResetColorsOrder);
    mMenuSettings->addAction(mActShowLetter);
    mMenuSettings->addSeparator();

    mMenuLevels = mMenuSettings->addMenu(tr("Level Presets"));
    mActGroupLevels = new QActionGroup(mMenuLevels);
    mActGroupLevels->addAction(mActLevelEasy);
    mActGroupLevels->addAction(mActLevelClassic);
    mActGroupLevels->addAction(mActLevelMedium);
    mActGroupLevels->addAction(mActLevelChallenging);
    mActGroupLevels->addAction(mActLevelHard);
    QList<QAction *> levelacts = mActGroupLevels->actions();
    mMenuLevels->addActions(levelacts);
    mMenuSettings->addSeparator();

    mMenuSettings->addAction(mActSameColor);
    mMenuSettings->addSeparator();

    mMenuSettings->addAction(mActAutoClose);
    mMenuSettings->addAction(mActAutoHints);

    mMenuSettings->addSeparator();
    mMenuSettings->addAction(mActPreferences);

    mMenuHelp = mMenuBar->addMenu(tr("&Help"));
    mMenuHelp->addAction(mLaunchHelpAction);
    mMenuHelp->addSeparator();
    mMenuHelp->addAction(mActAbout);
    mMenuHelp->addAction(mActAboutQt);

    mMenuRowContext = new QMenu();
    mMenuRowContext->addAction(mActRandRow);
    mMenuRowContext->addAction(mActPrevRow);
    mMenuRowContext->addAction(mActClearRow);
    mMenuRowContext->addSeparator();
    mMenuRowContext->addAction(mActSetGuess);
    mMenuRowContext->addSeparator();
    mMenuRowContext->addAction(mActNewGame);
    mMenuRowContext->addAction(mActRestartGame);
    mMenuRowContext->addAction(mActGiveIn);
    mMenuRowContext->addSeparator();
    mMenuRowContext->addAction(mActExit);

    addActions(mMenuBar->actions());
}

void ColorCode::InitToolBars()
{
    mGameToolbar = addToolBar(tr("Game"));
    mGameToolbar->setAllowedAreas(Qt::NoToolBarArea);
    mGameToolbar->setFloatable(false);
    mGameToolbar->setIconSize(QSize(16, 16));
    mGameToolbar->setMovable(false);
    mGameToolbar->addAction(mActNewGame);
    mGameToolbar->addAction(mActRestartGame);
    mGameToolbar->addAction(mActGiveIn);
    mGameToolbar->addSeparator();
    mGameToolbar->addAction(mActSetGuess);
    mGameToolbar->addAction(mActSetHints);

    mColorCntCmb = new QComboBox();
    mColorCntCmb->setLayoutDirection(Qt::LeftToRight);
    mColorCntCmb->setFixedWidth(84);
    mColorCntCmb->addItem("2 " + tr("Colors"), 2);
    mColorCntCmb->addItem("3 " + tr("Colors"), 3);
    mColorCntCmb->addItem("4 " + tr("Colors"), 4);
    mColorCntCmb->addItem("5 " + tr("Colors"), 5);
    mColorCntCmb->addItem("6 " + tr("Colors"), 6);
    mColorCntCmb->addItem("7 " + tr("Colors"), 7);
    mColorCntCmb->addItem("8 " + tr("Colors"), 8);
    mColorCntCmb->addItem("9 " + tr("Colors"), 9);
    mColorCntCmb->addItem("10 " + tr("Colors"), 10);
    mColorCntCmb->setCurrentIndex(6);
    connect(mColorCntCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(ColorCntChangedSlot()));

    mPegCntCmb = new QComboBox();
    mPegCntCmb->setLayoutDirection(Qt::LeftToRight);
    mPegCntCmb->setFixedWidth(76);
    mPegCntCmb->addItem("2 " + tr("Slots"), 2);
    mPegCntCmb->addItem("3 " + tr("Slots"), 3);
    mPegCntCmb->addItem("4 " + tr("Slots"), 4);
    mPegCntCmb->addItem("5 " + tr("Slots"), 5);
    mPegCntCmb->setCurrentIndex(2);
    connect(mPegCntCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(PegCntChangedSlot()));

    mLevelToolbar = addToolBar(tr("Level"));
    mLevelToolbar->setAllowedAreas(Qt::NoToolBarArea);
    mLevelToolbar->setFloatable(false);
    mLevelToolbar->setIconSize(QSize(16, 16));
    mLevelToolbar->setMovable(false);
    mLevelToolbar->setLayoutDirection(Qt::RightToLeft);
    mLevelToolbar->addWidget(mColorCntCmb);
    QWidget* spacer = new QWidget();
    spacer->setMinimumWidth(4);
    mLevelToolbar->addWidget(spacer);
    mLevelToolbar->addWidget(mPegCntCmb);
    QWidget* spacer2 = new QWidget();
    spacer2->setMinimumWidth(4);
    mLevelToolbar->addWidget(spacer2);
    mLevelToolbar->addAction(mActSameColorIcon);
}

void ColorCode::ApplySettings()
{
    bool restart = NeedsRestart();
    mNoAct = true;

    mActShowToolbar->setChecked(mSettings->mShowToolBar);
    ShowToolbarSlot();
    mActShowMenubar->setChecked(mSettings->mShowMenuBar);
    ShowMenubarSlot();
    mActShowStatusbar->setChecked(mSettings->mShowStatusBar);
    ShowStatusbarSlot();
    mActShowLetter->setChecked(mSettings->mShowIndicators);
    SetIndicators();

    mActAutoClose->setChecked(mSettings->mAutoClose);
    mActAutoHints->setChecked(mSettings->mAutoHints);

    SetSameColor(mSettings->mSameColors);
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
    CheckLevel();

    if (mSettings->mGameMode == MODE_HVM)
    {
        mActModeHvM->setChecked(true);
    }
    else
    {
        mActModeMvH->setChecked(true);
    }

    mHintsDelayTimer->setInterval(mSettings->mHintsDelay);

    mNoAct = false;

    if (restart)
    {
        TryNewGame();
    }
}

void ColorCode::TryNewGame()
{
    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }
}

bool ColorCode::NeedsRestart() const
{
    bool need = false;

    if ( mSettings->mSameColors != mActSameColor->isChecked()
        || mSettings->mColorCnt != mColorCnt
        || mSettings->mPegCnt != mPegCnt
        || mSettings->mGameMode != mGameMode
        || (mGameMode == MODE_MVH && mSolverStrength != mSettings->mSolverStrength) )
    {
        need = true;
    }

    return need;
}

void ColorCode::contextMenuEvent(QContextMenuEvent* e)
{

    QList<QGraphicsItem *> list = view->items(view->mapFromGlobal(e->globalPos()));
    int i = 0;
    bool isrow = false;
    if (mGameState == STATE_RUNNING && mCurRow != NULL)
    {
        for (i = 0; i < list.size(); ++i)
        {
            if (list.at(i) == mCurRow || list.at(i) == mHintBtns[mCurRow->GetIx()])
            {
                 isrow = true;
                 break;
            }
        }
    }

    if (isrow)
    {
        UpdateRowMenuSlot();
        mMenuRowContext->exec(e->globalPos());
    }
    else
    {
        mMenuGame->exec(e->globalPos());
    }
}

void ColorCode::resizeEvent (QResizeEvent* e)
{
    Q_UNUSED(e);
    Scale();
}

void ColorCode::keyPressEvent(QKeyEvent *e)
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

    switch (e->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (mGameMode == MODE_HVM)
            {
                int ix = mCurRow->GetIx();
                if (mHintBtns[ix]->mActive)
                {
                    std::vector<int> s = mCurRow->GetSolution();
                    if (s.size() == (unsigned) mPegCnt)
                    {
                        mHintBtns[ix]->SetActive(false);
                        HintPressedSlot(ix);
                    }
                }
            }
            else if (mGameMode == MODE_MVH)
            {
                if (mDoneBtn->isVisible() || mOkBtn->isVisible())
                {
                    DoneBtnPressSlot();
                }
            }
        break;
    }
}

void ColorCode::UpdateRowMenuSlot()
{
    if (mGameMode == MODE_HVM)
    {
        if (mGameState != STATE_RUNNING || mCurRow == NULL)
        {
            mActRandRow->setEnabled(false);
            mActPrevRow->setEnabled(false);
            mActClearRow->setEnabled(false);
            mActSetGuess->setEnabled(false);
            return;
        }
        else
        {
            mActRandRow->setEnabled(true);
        }

        if (mCurRow->GetIx() < 1)
        {
            mActPrevRow->setEnabled(false);
        }
        else
        {
            mActPrevRow->setEnabled(true);
        }

        if (mCurRow->GetPegCnt() == 0)
        {
            mActClearRow->setEnabled(false);
        }
        else
        {
            mActClearRow->setEnabled(true);
        }
    }
    else if (mGameMode == MODE_MVH)
    {
        if (mGameState == STATE_RUNNING && mCurRow == mSolutionRow)
        {
            mActRandRow->setEnabled(true);
            mActClearRow->setEnabled(true);
        }
        else
        {
            mActRandRow->setEnabled(false);
            mActClearRow->setEnabled(false);
        }
    }
}

void ColorCode::RestartGameSlot()
{
    if (mGameMode == MODE_HVM)
    {
        ResetRows();
        SetState(STATE_RUNNING);

        mCurRow = NULL;
        mSolver->RestartGame();
        NextRow();
    }
    else if (mGameMode == MODE_MVH)
    {
        mHintsDelayTimer->stop();
        mOkBtn->ShowBtn(false);
        ResetRows();
        SetState(STATE_RUNNING);

        mCurRow = NULL;
        mSolver->NewGame(mColorCnt, mPegCnt, mDoubles, mSolverStrength, ROW_CNT);
        mSolutionRow->OpenRow();
        GetSolution();
        RowSolutionSlot(mSolutionRow->GetIx());
    }
}

void ColorCode::NewGameSlot()
{
    if (mGameState == STATE_RUNNING && mCurRow != NULL)
    {
        int r = QMessageBox::warning( this,
                                      tr("New Game"),
                                      tr("Do you want to give in\nand start a new Game?"),
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (r == QMessageBox::Yes)
        {
            NewGame();
        }
    }
    else
    {
        NewGame();
    }
}

void ColorCode::GiveInSlot()
{
    int r = QMessageBox::Yes;
    if (mGameState == STATE_RUNNING && mCurRow != NULL)
    {
        r = QMessageBox::warning( this,
                                  tr("Give In"),
                                  tr("Do you really want to give in?"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    }

    if (r == QMessageBox::Yes)
    {
        SetState(STATE_GAVE_UP);

        if (mCurRow != NULL)
        {
            mCurRow->CloseRow();
            mHintBtns[mCurRow->GetIx()]->SetActive(false);
        }

        ResolveGame();
    }
}

void ColorCode::OpenPreferencesSlot()
{
    if (mPrefDialog == NULL)
    {
        CreatePrefDialog();
    }
    if (mPrefDialog == NULL)
    {
        return;
    }

    mSettings->SaveLastSettings();
    mPrefDialog->SetSettings();
    int r = mPrefDialog->exec();
    if (r == QDialog::Accepted)
    {
        ApplySettings();
    }
    else
    {
        mSettings->RestoreLastSettings();
    }
}

void ColorCode::OnlineHelpSlot()
{
    QDesktopServices::openUrl(QUrl("http://colorcode.laebisch.com/documentation", QUrl::TolerantMode));
}

void ColorCode::AboutSlot()
{
    About ab(this);
    ab.exec();
}

void ColorCode::AboutQtSlot()
{
    QMessageBox::aboutQt( this,
                        tr("About Qt"));
}

void ColorCode::ShowToolbarSlot()
{
    mSettings->mShowToolBar = mActShowToolbar->isChecked();
    if (!mActShowToolbar->isChecked())
    {
        mGameToolbar->hide();
        mLevelToolbar->hide();
    }
    else
    {
        mGameToolbar->show();
        mLevelToolbar->show();
    }
    Scale();
}

void ColorCode::ShowMenubarSlot()
{
    mSettings->mShowMenuBar = mActShowMenubar->isChecked();
    if (!mActShowMenubar->isChecked())
    {
        mMenuBar->hide();
    }
    else
    {
        mMenuBar->show();
    }
    Scale();
}

void ColorCode::ShowStatusbarSlot()
{
    mSettings->mShowStatusBar = mActShowStatusbar->isChecked();
    if (!mActShowStatusbar->isChecked())
    {
        statusBar()->hide();
    }
    else
    {
        statusBar()->show();
    }
    Scale();
}

void ColorCode::ResetColorsOrderSlot()
{
    int ix;
    for (ix = 0; ix < MAX_COLOR_CNT; ++ix)
    {
        mGradMap[ix] = mGradBuff[ix];
    }
    scene->update(scene->sceneRect());
}

void ColorCode::SetIndicators()
{
    bool checked = mActShowLetter->isChecked();
    mSettings->mShowIndicators = checked;
    if (!mSettings->mShowIndicators)
    {
        mHideColors = false;
    }
    else
    {
        mHideColors = mSettings->mHideColors;
    }
    vector<ColorPeg *>::iterator it;
    for (it = mAllPegs.begin(); it < mAllPegs.end(); it++)
    {
        (*it)->SetIndicator(checked, mSettings->mIndicatorType, mHideColors);
    }
}

void ColorCode::SameColorSlot(bool checked)
{
    if (mNoAct)
    {
        return;
    }

    SetSameColor(checked);

    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }
}

void ColorCode::AutoCloseSlot()
{
    mSettings->mAutoClose = mActAutoClose->isChecked();
}

void ColorCode::AutoHintsSlot()
{
    mSettings->mAutoHints = mActAutoHints->isChecked();
}

void ColorCode::ColorCntChangedSlot()
{
    SetColorCnt();

    if (mNoAct)
    {
        return;
    }

    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }
}

void ColorCode::PegCntChangedSlot()
{
    SetPegCnt();

    if (mNoAct)
    {
        return;
    }

    int pcnt = mPegCntCmb->itemData(mPegCntCmb->currentIndex(), IdRole).toInt();
    if (pcnt == mPegCnt)
    {
        return;
    }

    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }
}

void ColorCode::SetLevelSlot()
{
    mNoAct = true;
    int ix = mActGroupLevels->checkedAction()->data().toInt();

    if (ix < 0 || ix > 4)
    {
        return;
    }

    int i;
    i = mColorCntCmb->findData(LEVEL_SETTINGS[ix][0]);
    if (i != -1 && mColorCntCmb->currentIndex() != i)
    {
        mColorCntCmb->setCurrentIndex(i);
        SetColorCnt();
    }
    i = mPegCntCmb->findData(LEVEL_SETTINGS[ix][1]);
    if (i != -1 && mPegCntCmb->currentIndex() != i)
    {
        mPegCntCmb->setCurrentIndex(i);
        SetPegCnt();
    }

    SetSameColor((LEVEL_SETTINGS[ix][2] == 1));

    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    mNoAct = false;

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }    
}

void ColorCode::SetGameModeSlot()
{
    SetGameMode();

    int r = QMessageBox::Yes;
    if (GamesRunning())
    {
        r = QMessageBox::question( this,
                                   tr("Message"),
                                   tr("The changed settings will only apply to new games!\nDo you want to give in the current and start a new Game?"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::Yes);
    }

    if (r == QMessageBox::Yes)
    {
        NewGame();
    }
}


void ColorCode::SetPegCnt()
{
    mSettings->mPegCnt = mPegCntCmb->itemData(mPegCntCmb->currentIndex(), IdRole).toInt();
}

void ColorCode::SetColorCnt()
{
    mSettings->mColorCnt = mColorCntCmb->itemData(mColorCntCmb->currentIndex(), IdRole).toInt();
}

void ColorCode::SetGameMode()
{
    int ix = mActGroupModes->checkedAction()->data().toInt();
    if (ix != MODE_HVM && ix != MODE_MVH)
    {
        return;
    }
    mSettings->mGameMode = ix;
}


void ColorCode::RemovePegSlot(ColorPeg* cp)
{
    RemovePeg(cp);
}

void ColorCode::ShowMsgSlot(QString msg)
{
    mMsg->ShowMsg(msg);
}

void ColorCode::PegPressSlot(ColorPeg* cp)
{
    if (cp == NULL) { return; }

    cp->setZValue(LAYER_DRAG);

    if (cp->IsBtn())
    {
        cp->SetBtn(false);
        mPegBtns[cp->GetPegType()->ix] = CreatePeg(cp->GetPegType()->ix);
        mPegBtns[cp->GetPegType()->ix]->SetEnabled(false);
    }
}

void ColorCode::PegSortSlot(ColorPeg* cp)
{
    if (cp == NULL) { return; }
    
    cp->setZValue(LAYER_DRAG);

    if (cp->IsBtn())
    {
        cp->SetBtn(false);
        mPegBtns[cp->GetPegType()->ix] = CreatePeg(cp->GetPegType()->ix);
        mPegBtns[cp->GetPegType()->ix]->SetEnabled(false);
    }
}

void ColorCode::PegReleasedSlot(ColorPeg* cp)
{
    if (cp == NULL || !cp) { return; }

    cp->setZValue(LAYER_PEGS);
    scene->clearSelection();
    scene->clearFocus();

    int i;
    if (cp->GetSort() == 0)
    {
        bool snapped = false;

        if (mGameMode == MODE_HVM)
        {
            QList<QGraphicsItem *> list = scene->items(QPointF(cp->pos().x() + 18, cp->pos().y() + 18));

            for (i = 0; i < list.size(); ++i)
            {
                if (mCurRow != NULL && list.at(i) == mCurRow)
                {
                     snapped = mCurRow->SnapCP(cp);
                     break;
                }
            }
        }
        else if (mGameMode == MODE_MVH)
        {
            snapped = mSolutionRow->SnapCP(cp);
        }

        if (!snapped)
        {
            RemovePeg(cp);
        }
    }
    else
    {
        int ystart = ROW_Y0 + (MAX_COLOR_CNT - mColorCnt) * 40;
        int ix1 = qRound((qRound(cp->pos().y()) - ystart + 18) / 40);
        ix1 = min(mColorCnt - 1, max(0, ix1));

        int ix0 = cp->GetPegType()->ix;
        if (ix0 != ix1)
        {
            QRadialGradient tmp = mGradMap[ix0];

            if (cp->GetSort() == 1)
            {
                mGradMap[ix0] = mGradMap[ix1];
            }
            else
            {
                int step = (ix1 > ix0) ? 1 : -1;



                for (i = ix0;; i += step)
                {
                    mGradMap[i] = mGradMap[i + step];
                    if (i == ix1 - step)
                    {
                        break;
                    }
                }
            }

            mGradMap[ix1] = tmp;
        }

        RemovePeg(cp);
    }

    if (!mPegBtns[cp->GetPegType()->ix]->isEnabled())
    {
        mPegBtns[cp->GetPegType()->ix]->SetEnabled(true);
    }
}

void ColorCode::RowSolutionSlot(int ix)
{
    if (mGameMode == MODE_HVM)
    {
        if (ix == -1) { return; }
        
        std::vector<int> s = mPegRows[ix]->GetSolution();
        if (s.size() == (unsigned) mPegCnt)
        {
            if (mActAutoClose->isChecked())
            {
                HintPressedSlot(ix);
            }
            else
            {
                mHintBtns[ix]->SetActive(true);
                ShowMsgSlot(tr("Press the Hint Field or Key Enter if You're done."));
            }
        }
        else
        {
            mHintBtns[ix]->SetActive(false);
            ShowMsgSlot(tr("Place Your pegs ..."));
        }
    }
    else if (mGameMode == MODE_MVH)
    {
        if (ix == -1)
        {
            std::vector<int> s = mSolutionRow->GetSolution();
            if (s.size() == (unsigned) mPegCnt)
            {
                bool valid = true;
                if (!mActSameColor->isChecked())
                {                    
                    int i;
                    int check[mColorCnt];
                    for (i = 0; i < mColorCnt; ++i)
                    {
                        check[i] = 0;
                    }

                    for (i = 0; (unsigned)i < s.size(); ++i)
                    {
                        if (s[i] >= mColorCnt)
                        {
                            valid = false;
                            break;
                        }

                        if (check[s[i]] != 0)
                        {
                            valid = false;
                            break;
                        }

                        check[s[i]] = 1;
                    }
                }

                if (valid)
                {
                    mDoneBtn->ShowBtn(true);
                    ShowMsgSlot(tr("Press the button below or Key Enter if You're done."));
                }
                else
                {
                    ShowMsgSlot(tr("The chosen settings do not allow pegs of the same color!"));
                }
            }
            else
            {
                mDoneBtn->ShowBtn(false);
                ShowMsgSlot(tr("Place Your secret ColorCode ..."));
            }
        }
    }
}

void ColorCode::HintPressedSlot(int)
{
    mCurRow->CloseRow();
    ResolveRow();
    NextRow();
    ResolveGame();
}

void ColorCode::RandRowSlot()
{
    if (mCurRow == NULL || !mCurRow->IsActive() || mGameState != STATE_RUNNING)
    {
        return;
    }

    if (mGameMode == MODE_HVM || (mGameMode == MODE_MVH && mCurRow == mSolutionRow))
    {
        mCurRow->ClearRow();

        int i, rndm;
        int check[mColorCnt];
        ColorPeg* peg;
        for (i = 0; i < mColorCnt; ++i)
        {
            check[i] = 0;
        }

        for (i = 0; i < mPegCnt; ++i)
        {
            rndm = qrand() % mColorCnt;
            if (mDoubles == 0 && check[rndm] != 0)
            {
                --i;
                continue;
            }

            check[rndm] = 1;

            peg = CreatePeg(rndm);
            mCurRow->ForceSnap(peg, i);
        }
    }
}

void ColorCode::SetGuessSlot()
{
    if (mCurRow == NULL || mCurRow == mSolutionRow || mGameState != STATE_RUNNING || mSolver->mBusy)
    {
        return;
    }

    mCurRow->ClearRow();
    mActSetGuess->setEnabled(false);
    int* row = mSolver->GuessOut();
    if (row == NULL)
    {
        SetState(STATE_ERROR);
        return;
    }
    mActSetGuess->setEnabled(true);

    ColorPeg* peg;
    int i;
    for (i = 0; i < mPegCnt; ++i)
    {
        peg = CreatePeg(row[i]);
        mCurRow->ForceSnap(peg, i);
    }
}

void ColorCode::SetHintsSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

    if (mGameMode == MODE_MVH)
    {
        if (mCurRow == mSolutionRow || !mHintBtns[mCurRow->GetIx()]->IsActive())
        {
            return;
        }

        std::vector<int> hints = RateSol2Guess(mSolution, mCurRow->GetSolution());
        mHintBtns[mCurRow->GetIx()]->DrawHints(hints);
    }
}

void ColorCode::PrevRowSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

    if (mCurRow->GetIx() < 1)
    {
        return;
    }

    mCurRow->ClearRow();

    ColorPeg* peg;
    std::vector<int> prev = mPegRows[mCurRow->GetIx() - 1]->GetSolution();
    int i;
    for (i = 0; i < mPegCnt; ++i)
    {
        peg = CreatePeg(prev.at(i));
        mCurRow->ForceSnap(peg, i);
    }
}

void ColorCode::ClearRowSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

    mCurRow->ClearRow();
}

void ColorCode::DoneBtnPressSlot(GraphicsBtn*)
{
    mDoneBtn->ShowBtn(false);
    mOkBtn->ShowBtn(false);

    if (mCurRow == mSolutionRow)
    {
        SetSolution();
        mActClearRow->setEnabled(false);
        mActRandRow->setEnabled(false);
        mActSetHints->setEnabled(true);
        NextRow();
    }
    else
    {
        ResolveHints();
    }
}

void ColorCode::SetAutoHintsSlot()
{
    DoneBtnPressSlot(mOkBtn);
}

void ColorCode::TestSlot()
{
    
}

void ColorCode::ApplyPreferencesSlot()
{

}

void ColorCode::CreatePrefDialog()
{
    mPrefDialog = new PrefDialog(this);
    mPrefDialog->setModal(true);
    mPrefDialog->InitSettings(mSettings);
    connect(mPrefDialog, SIGNAL(accepted()), this, SLOT(ApplyPreferencesSlot()));
    connect(mPrefDialog, SIGNAL(ResetColorOrderSignal()), this, SLOT(ResetColorsOrderSlot()));
}

void ColorCode::CheckSameColorsSetting()
{
    if (mColorCnt < mPegCnt)
    {
        if (mActSameColor->isEnabled())
        {
            mActSameColor->setEnabled(false);
        }
        if (mActSameColorIcon->isEnabled())
        {
            mActSameColorIcon->setEnabled(false);
        }
        if (!mActSameColor->isChecked())
        {
            mActSameColor->setChecked(true);
        }
        if (!mActSameColorIcon->isChecked())
        {
            mActSameColorIcon->setChecked(true);
        }
    }
    else
    {
        if (!mActSameColor->isEnabled())
        {
            mActSameColor->setEnabled(true);
        }
        if (!mActSameColorIcon->isEnabled())
        {
            mActSameColorIcon->setEnabled(true);
        }
    }

    if (mActSameColorIcon->isChecked())
    {
        mActSameColorIcon->setIcon(QIcon(":/img/same_color_1.png"));
        mActSameColorIcon->setToolTip(tr("Disallow Pegs of the Same Color"));
    }
    else
    {
        mActSameColorIcon->setIcon(QIcon(":/img/same_color_0.png"));
        mActSameColorIcon->setToolTip(tr("Allow Pegs of the Same Color"));
    }
}

void ColorCode::SetSameColor(bool checked)
{
    mSettings->mSameColors = checked;

    mActSameColor->setChecked(checked);
    mActSameColorIcon->setChecked(checked);

    if (checked)
    {
        mActSameColorIcon->setIcon(QIcon(":/img/same_color_1.png"));
        mActSameColorIcon->setToolTip(tr("Disallow Pegs of the Same Color"));
    }
    else
    {
        mActSameColorIcon->setIcon(QIcon(":/img/same_color_0.png"));
        mActSameColorIcon->setToolTip(tr("Allow Pegs of the Same Color"));
    }
}

void ColorCode::CheckLevel()
{
    int ix = -1;
    for (int i = 0; i < 5; ++i)
    {
        if ( LEVEL_SETTINGS[i][0] == mColorCnt
             && LEVEL_SETTINGS[i][1] == mPegCnt
             && LEVEL_SETTINGS[i][2] == mDoubles )
        {
            ix = i;
            break;
        }
    }

    if (ix > -1)
    {
        QList<QAction *> levelacts = mActGroupLevels->actions();
        levelacts.at(ix)->setChecked(true);
    }
    else
    {
        QAction* act = mActGroupLevels->checkedAction();
        if (act != NULL)
        {
            act->setChecked(false);
        }
    }
}

void ColorCode::ResetGame()
{
    mDoneBtn->ShowBtn(false);
    mOkBtn->ShowBtn(false);

    ApplyPegCnt();

    mSolutionRow->Reset(mPegCnt, mGameMode);
    ResetRows();

    mGameId = qrand();
    mGuessCnt = 0;
    ++mGameCnt;

    ApplyColorCnt();
    
    CheckSameColorsSetting();

    mDoubles = (int) mActSameColor->isChecked();
    CheckLevel();

    ApplySolverStrength();
}

void ColorCode::ResetRows()
{
    for (int i = 0; i < ROW_CNT; ++i)
    {
        mPegRows[i]->Reset(mPegCnt, mGameMode);
        mHintBtns[i]->Reset(mPegCnt, mGameMode);
    }
}

void ColorCode::NewGame()
{
    mHintsDelayTimer->stop();
    ApplyGameMode();
    ResetGame();
    QString doubles = (mDoubles == 1) ? tr("Yes") : tr("No");
    QString colors = QString::number(mColorCnt, 10);
    QString pegs = QString::number(mPegCnt, 10);
    mStatusLabel->setText(tr("Pegs of Same Color") + ": <b>" + doubles + "</b> :: " + tr("Slots") + ": <b>" + pegs + "</b> :: " + tr("Colors") + ": <b>" + colors + "</b> ");
    SetState(STATE_RUNNING);

    mCurRow = NULL;

    if (mGameMode == MODE_HVM)
    {
        mSolver->NewGame(mColorCnt, mPegCnt, mDoubles, CCSolver::STRENGTH_HIGH, ROW_CNT);
        SetSolution();
        NextRow();
    }
    else if (mGameMode == MODE_MVH)
    {
        mSolver->NewGame(mColorCnt, mPegCnt, mDoubles, mSolverStrength, ROW_CNT);
        GetSolution();
    }
}

void ColorCode::NextRow()
{
    if (mGameState != STATE_RUNNING)
    {
        return;
    }

    if (mCurRow == NULL)
    {
        mCurRow = mPegRows[0];
    }
    else if (mCurRow->GetIx() < ROW_CNT - 1)
    {
        mCurRow = mPegRows[mCurRow->GetIx() + 1];
    }
    else
    {
        mCurRow = NULL;
        SetState(STATE_LOST);
    }

    if (mCurRow != NULL)
    {
        ++mGuessCnt;
        mCurRow->SetActive(true);
        if (mGameMode == MODE_HVM)
        {
            ShowMsgSlot(tr("Place Your pegs ..."));
        }
        else if (mGameMode == MODE_MVH)
        {
            SetGuessSlot();
            if (mGameState == STATE_RUNNING)
            {
                mCurRow->CloseRow();
                mHintBtns[mCurRow->GetIx()]->SetActive(true);

                if (mActAutoHints->isChecked())
                {
                    if (mOkBtn->isVisible())
                    {
                        mOkBtn->ShowBtn(false);
                    }
                    ShowMsgSlot(tr("Please rate the guess. Press OK or Key Enter if You're done."));
                    SetHintsSlot();
                    mHintBtns[mCurRow->GetIx()]->SetActive(false);

                    std::vector<int> rowhints = mHintBtns[mCurRow->GetIx()]->GetHints();
                    int b = 0;
                    for (unsigned i = 0; i < rowhints.size(); ++i)
                    {
                        if (rowhints.at(i) == 2)
                        {
                            ++b;
                        }
                    }

                    if (b == mPegCnt)
                    {
                        SetState(STATE_WON);
                        ResolveGame();
                    }
                    else
                    {
                        mHintsDelayTimer->start();
                    }
                }
                else
                {
                    ShowMsgSlot(tr("Please rate the guess. Press OK or Key Enter if You're done."));
                    mOkBtn->setPos(5, mCurRow->pos().y() - 39);
                    mOkBtn->ShowBtn(true);
                }
            }
            else
            {
                mCurRow->SetActive(false);
                ResolveGame();
            }
        }
    }
}

void ColorCode::ResolveRow()
{
    std::vector<int> rowsol = mCurRow->GetSolution();
    mSolver->GuessIn(&rowsol);

    std::vector<int> hints = RateSol2Guess(mSolution, rowsol);

    if (hints.size() == (unsigned) mPegCnt)
    {
        int bl = 0;
        for (int i = 0; i < mPegCnt; ++i)
        {
            if (hints.at(i) == 2)
            {
                ++bl;
            }
        }

        if (bl == mPegCnt)
        {
            SetState(STATE_WON);
        }
    }

    mSolver->ResIn(&hints);
    mHintBtns[mCurRow->GetIx()]->DrawHints(hints);
}

std::vector<int> ColorCode::RateSol2Guess(const std::vector<int> sol, const std::vector<int> guess)
{
    std::vector<int> hints;
    std::vector<int> left1;
    std::vector<int> left0;

    int i, p0, p1;
    for (i = 0; i < mPegCnt; ++i)
    {
        p0 = guess.at(i);
        p1 = sol.at(i);
        if (p0 == p1)
        {
            hints.push_back(2);
        }
        else
        {
            left0.push_back(p0);
            left1.push_back(p1);
        }
    }

    if (hints.size() < (unsigned) mPegCnt)
    {
        int len0 = left0.size();
        for (i = 0; i < len0; ++i)
        {
            p0 = left0.at(i);
            for (unsigned j = 0; j < left1.size(); ++j)
            {
                p1 = left1.at(j);
                if (p0 == p1)
                {
                    hints.push_back(1);
                    left1.erase(left1.begin() + j);
                    break;
                }
            }
        }
    }

    return hints;
}

void ColorCode::ResolveHints()
{
    mHintBtns[mCurRow->GetIx()]->SetActive(false);

    std::vector<int> rowsol = mCurRow->GetSolution();
    mSolver->GuessIn(&rowsol);
    std::vector<int> rowhints = mHintBtns[mCurRow->GetIx()]->GetHints();
    int b = 0;
    int w = 0;
    for (unsigned i = 0; i < rowhints.size(); ++i)
    {
        if (rowhints.at(i) == 2)
        {
            ++b;
        }
        else if (rowhints.at(i) == 1)
        {
            ++w;
        }
    }

    if (b == mPegCnt)
    {
        SetState(STATE_WON);
        ResolveGame();
    }
    else if (b == mPegCnt - 1 && w == 1)
    {
        
    }
    else
    {
        mSolver->ResIn(&rowhints);
        NextRow();
        ResolveGame();
    }
}

void ColorCode::ResolveGame()
{
    if (mGameMode == MODE_HVM)
    {
        switch (mGameState)
        {
            case STATE_WON:
                ShowMsgSlot(tr("Congratulation! You have won!"));
            break;
            case STATE_LOST:
                ShowMsgSlot(tr("Sorry! You lost!"));
            break;
            case STATE_GAVE_UP:
                ShowMsgSlot(tr("Sure, You're too weak for me!"));
            break;
            case STATE_ERROR:
                ShowMsgSlot(tr("The impossible happened, sorry."));
            break;
            case STATE_RUNNING:
            default:
                return;
            break;
        }

        ShowSolution();
    }
    else if (mGameMode == MODE_MVH)
    {
        switch (mGameState)
        {
            case STATE_WON:
                ShowMsgSlot(tr("Yeah! I guessed it, man!"));
            break;
            case STATE_LOST:
                ShowMsgSlot(tr("Embarrassing! I lost a game!"));
            break;
            case STATE_GAVE_UP:
                ShowMsgSlot(tr("Don't you like to see me winning? ;-)"));
            break;
            case STATE_ERROR:
                ShowMsgSlot(tr("Nope! Thats impossible! Did you gave me false hints?"));
            break;
            case STATE_RUNNING:
            default:
                return;
            break;
        }

        mDoneBtn->ShowBtn(false);
        mOkBtn->ShowBtn(false);
    }
}

void ColorCode::ApplyGameMode()
{
    int ix = mActGroupModes->checkedAction()->data().toInt();

    if (ix != MODE_HVM && ix != MODE_MVH)
    {
        return;
    }

    mGameMode = ix;

    if (mGameMode == MODE_HVM)
    {
        mActSetHints->setVisible(false);

        mActSetGuess->setVisible(true);
        mActPrevRow->setVisible(true);
    }
    else if (mGameMode == MODE_MVH)
    {
        mActSetHints->setVisible(true);

        mActSetGuess->setVisible(false);
        mActPrevRow->setVisible(false);
    }
}

void ColorCode::ApplyPegCnt()
{
    int pegcnt = mPegCntCmb->itemData(mPegCntCmb->currentIndex(), IdRole).toInt();
    pegcnt = max(2, min(5, pegcnt));
    mPegCnt = pegcnt;
    mXOffs = 160 - mPegCnt * 20;
}

void ColorCode::ApplyColorCnt()
{
    int ccnt = mColorCntCmb->itemData(mColorCntCmb->currentIndex(), IdRole).toInt();
    ccnt = max(2, min(10, ccnt));

    if (mColorCnt == ccnt)
    {
        return;
    }

    mColorCnt = ccnt;
    int xpos = 279;
    int ystart = ROW_Y0 + (MAX_COLOR_CNT - mColorCnt) * 40;
    int ypos;
    int i;

    for (i = 0; i < MAX_COLOR_CNT; ++i)
    {
        ypos = ystart + i * 40 + 2;
        mBtnPos[i] = QPoint(xpos, ypos);

        if (i < mColorCnt)
        {
            if (mPegBtns[i] == NULL)
            {
                mPegBtns[i] = CreatePeg(i);
            }
            mPegBtns[i]->setPos(mBtnPos[i]);
            mPegBtns[i]->setVisible(true);
        }
        else if (mPegBtns[i] != NULL)
        {
            mPegBtns[i]->setVisible(false);
        }
    }
}

void ColorCode::ApplySolverStrength()
{
    mSolverStrength = mSettings->mSolverStrength;
}

void ColorCode::SetSolution()
{
    mSolution.clear();

    if (mGameMode == MODE_HVM)
    {
        mSolutionRow->ClearRow();
        int i, rndm;
        int check[mColorCnt];
        for (i = 0; i < mColorCnt; ++i)
        {
            check[i] = 0;
        }

        for (i = 0; i < mPegCnt; ++i)
        {
            rndm = qrand() % mColorCnt;
            if (mDoubles == 0 && check[rndm] != 0)
            {
                --i;
                continue;
            }
            mSolution.push_back(rndm);
            check[rndm] = 1;
        }
    }
    else if (mGameMode == MODE_MVH)
    {
        std::vector<int> s = mSolutionRow->GetSolution();
        if (s.size() == (unsigned) mPegCnt)
        {
            mSolutionRow->CloseRow();
            mDoneBtn->ShowBtn(false);

            for (int i = 0; i < mPegCnt; ++i)
            {
                mSolution.push_back(s.at(i));
            }
        }
    }
}

void ColorCode::GetSolution()
{
    mActSetHints->setEnabled(false);
    mActRandRow->setEnabled(true);
    mActClearRow->setEnabled(true);

    ShowMsgSlot(tr("Place Your secret ColorCode ..."));
    mSolutionRow->SetActive(true);
    mCurRow = mSolutionRow;
}

void ColorCode::ShowSolution()
{
    mSolutionRow->SetActive(true);
    ColorPeg* peg;
    for (int i = 0; i < mPegCnt; ++i)
    {
        peg = CreatePeg(mSolution.at(i));
        peg->SetBtn(false);
        mSolutionRow->ForceSnap(peg, i);
    }
    mSolutionRow->CloseRow();
}

void ColorCode::SetState(const int s)
{
    if (mGameState == s)
    {
        return;
    }

    mGameState = s;

    bool running = mGameState == STATE_RUNNING;

    mActRestartGame->setEnabled(running);
    mActGiveIn->setEnabled(running);
    mActSetGuess->setEnabled(running);
    mActSetHints->setEnabled(running);
}

bool ColorCode::GamesRunning()
{
    if (mGameMode == MODE_HVM)
    {
        return (mGameState == STATE_RUNNING && mGuessCnt > 1);
    }
    else if (mGameMode == MODE_MVH)
    {
        return (mGameState == STATE_RUNNING && mCurRow != mSolutionRow);
    }
    return false;
}

void ColorCode::Scale()
{
    if (mGameCnt == 0)
    {
        return;
    }

    qreal w = geometry().width() - 4;
    qreal h = geometry().height() - 4;

    if (mActShowStatusbar->isChecked())
    {
        h -= statusBar()->height();
    }
    if (mActShowMenubar->isChecked())
    {
        h -= mMenuBar->height();
    }
    if (mActShowToolbar->isChecked())
    {
        h -= mGameToolbar->height();
    }

    if (w < 50 || h < 50)
    {
        return;
    }

    if (mOrigSize == NULL)
    {
        mOrigSize = new QSize(320, 560);
        scene->setSceneRect(QRectF(0, 0, 320, 560));
    }
    else
    {
        qreal sc = min(w / mOrigSize->width(), h / mOrigSize->height());
        view->resetTransform();
        view->scale(sc, sc);
    }
}

ColorPeg* ColorCode::CreatePeg(int ix)
{
    if (ix < 0 || ix >= mColorCnt)
    {
        ix = mAllPegs.size() % mColorCnt;
    }

    PegType *pt = mTypesMap[ix];
    ColorPeg *peg;

    if (mPegBuff.empty())
    {
        peg = new ColorPeg;
        peg->SetPegType(pt);
        peg->SetBtn(true);
        peg->SetIndicator(mActShowLetter->isChecked(), mSettings->mIndicatorType, mHideColors);
        peg->SetId(mAllPegs.size());
        scene->addItem(peg);
        peg->setPos(mBtnPos[ix]);
        peg->setZValue(LAYER_PEGS);
        mAllPegs.push_back(peg);
        scene->clearSelection();
        scene->update(mBtnPos[ix].x(), mBtnPos[ix].y(), 38, 38);
        peg->setSelected(true);

        connect(peg, SIGNAL(PegPressSignal(ColorPeg *)), this, SLOT(PegPressSlot(ColorPeg *)));
        connect(peg, SIGNAL(PegSortSignal(ColorPeg *)), this, SLOT(PegSortSlot(ColorPeg *)));
        connect(peg, SIGNAL(PegReleasedSignal(ColorPeg *)), this, SLOT(PegReleasedSlot(ColorPeg *)));
    }
    else
    {
        unsigned int sz = mPegBuff.size();
        peg = mPegBuff.at(sz - 1);
        mPegBuff.resize(sz - 1);

        peg->setVisible(true);

        peg->SetPegType(pt);
        peg->SetBtn(true);
        peg->SetEnabled(true);
        peg->setPos(mBtnPos[ix]);
        scene->clearSelection();
        scene->update(mBtnPos[ix].x(), mBtnPos[ix].y(), 38, 38);
        peg->setSelected(true);
    }

    return peg;
}

void ColorCode::RemovePeg(ColorPeg* cp)
{
    if (cp != NULL)
    {
        cp->Reset();
        cp->setVisible(false);
        mPegBuff.push_back(cp);
    }
}
