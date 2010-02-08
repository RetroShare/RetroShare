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

#include "ccsolver.h"
#include "colorcode.h"
#include "colorpeg.h"
#include "pegrow.h"
#include "rowhint.h"
#include "msg.h"
#include "background.h"
#include "about.h"
#include "solrow.h"

using namespace std;

const int IdRole = Qt::UserRole;

int ColorCode::mColorCnt = 0;
int ColorCode::mPegCnt   = 0;
int ColorCode::mDoubles  = 1;
int ColorCode::mXOffs    = 0;
int ColorCode::mMaxZ     = 0;
int ColorCode::mLevel    = -1;

volatile bool ColorCode::mNoAct    = false;

const int ColorCode::STATE_RUNNING = 0;
const int ColorCode::STATE_WON     = 1;
const int ColorCode::STATE_LOST    = 2;
const int ColorCode::STATE_GAVE_UP = 3;

const int ColorCode::MAX_COLOR_CNT = 10;

const int ColorCode::mLevelSettings[5][3] = {
                                                { 2, 2, 1},
                                                { 4, 3, 0},
                                                { 6, 4, 1},
                                                { 8, 4, 1},
                                                {10, 5, 1}
                                            };

ColorCode::ColorCode()
{
    QTime midnight(0, 0, 0);
    qsrand(midnight.secsTo(QTime::currentTime()));

    ROW_CNT = 10;
    ROW_Y0  = 130;

    mOrigSize = NULL;

    setWindowTitle(tr("ColorCode"));
    setWindowIcon(QIcon(QPixmap(":/img/cc16.png")));
    setIconSize(QSize(16, 16));

    mMenuBar = menuBar();
    mBg = new BackGround();
    mMsg = new Msg;

    // as long as menuBar's size is invalid populated initially ...
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
    scene->addItem(mBg);
    mBg->setPos(0, 0);
    scene->addItem(mMsg);
    mMsg->setPos(20, 10);

    mSolver = new CCSolver(this);

    mCurRow   = NULL;
    mColorCnt = MAX_COLOR_CNT;
    mGameId   = 0;
    mGuessCnt = 0;

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

    NewGame();

    mMsg->setZValue(++ColorCode::mMaxZ);
}

ColorCode::~ColorCode()
{
    int i;
    if (mPegCnt > 0)
    {
        for (i = 0; i < mPegCnt; ++i)
        {
            if (mSolPegs[i] != NULL)
            {
                RemovePeg(mSolPegs[i]);
                mSolPegs[i] = NULL;
            }
        }

        delete [] mSolPegs;
        mSolPegs = NULL;
    }
}

void ColorCode::InitSolution()
{
    mSolRow = new SolRow();
    mSolRow->setPos(50, 70);
    scene->addItem(mSolRow);
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
        row->setZValue(++ColorCode::mMaxZ);
        row->SetActive(false);
        scene->addItem(row);
        mPegRows[i] = row;
        connect(row, SIGNAL(RemovePegSignal(ColorPeg*)), this, SLOT(RemovePegSlot(ColorPeg*)));
        connect(row, SIGNAL(RowSolutionSignal(int)), this, SLOT(RowSolutionSlot(int)));

        hint = new RowHint;
        hint->SetIx(i);
        hint->setPos(QPoint(4, ypos + (ROW_CNT - (i + 1)) * 40));
        hint->setZValue(++ColorCode::mMaxZ);
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
    NewGameAction = new QAction(tr("&New Game"), this);
    NewGameAction->setIcon(QIcon(":/img/document-new.png"));
    NewGameAction->setShortcut(tr("Ctrl+N"));
    connect(NewGameAction, SIGNAL(triggered()), this, SLOT(NewGameSlot()));

    RestartGameAction = new QAction(tr("&Restart Game"), this);
    RestartGameAction->setIcon(QIcon(":/img/view-refresh.png"));
    RestartGameAction->setShortcut(tr("Ctrl+Shift+R"));
    connect(RestartGameAction, SIGNAL(triggered()), this, SLOT(RestartGameSlot()));

    GiveInAction = new QAction(tr("&Throw In The Towel"), this);
    GiveInAction->setIcon(QIcon(":/img/face-sad.png"));
    GiveInAction->setShortcut(tr("Ctrl+G"));
    connect(GiveInAction, SIGNAL(triggered()), this, SLOT(GiveInSlot()));

    ExitAction = new QAction(tr("E&xit"), this);
    ExitAction->setIcon(QIcon(":/img/application-exit.png"));
    ExitAction->setShortcut(tr("Ctrl+Q"));
    connect(ExitAction, SIGNAL(triggered()), this, SLOT(close()));

    ShowToolbarAction = new QAction(tr("Show Toolbar"), this);
    ShowToolbarAction->setCheckable(true);
    ShowToolbarAction->setChecked(true);
    ShowToolbarAction->setShortcut(tr("Ctrl+T"));
    connect(ShowToolbarAction, SIGNAL(triggered()), this, SLOT(ShowToolbarSlot()));

    ShowMenubarAction = new QAction(tr("Show Menubar"), this);
    ShowMenubarAction->setCheckable(true);
    ShowMenubarAction->setChecked(true);
    ShowMenubarAction->setShortcut(tr("Ctrl+M"));
    connect(ShowMenubarAction, SIGNAL(triggered()), this, SLOT(ShowMenubarSlot()));

    ShowStatusbarAction = new QAction(tr("Show Statusbar"), this);
    ShowStatusbarAction->setCheckable(true);
    ShowStatusbarAction->setChecked(true);
    ShowStatusbarAction->setShortcut(tr("Ctrl+S"));
    connect(ShowStatusbarAction, SIGNAL(triggered()), this, SLOT(ShowStatusbarSlot()));

    mActResetColorsOrder = new QAction(tr("Reset Color Order"), this);
    mActResetColorsOrder->setShortcut(tr("Ctrl+Shift+R"));
    connect(mActResetColorsOrder, SIGNAL(triggered()), this, SLOT(ResetColorsOrderSlot()));

    mActShowLetter = new QAction(tr("Show Letter Indicators"), this);
    mActShowLetter->setCheckable(true);
    mActShowLetter->setChecked(false);
    mActShowLetter->setShortcut(tr("Ctrl+Shift+L"));
    connect(mActShowLetter, SIGNAL(triggered()), this, SLOT(ShowLetterSlot()));

    SameColorAction = new QAction(tr("Allow Pegs of the Same Color"), this);
    SameColorAction->setCheckable(true);
    SameColorAction->setChecked(true);
    SameColorAction->setShortcut(tr("Ctrl+Shift+C"));
    connect(SameColorAction, SIGNAL(triggered(bool)), this, SLOT(SameColorSlot(bool)));

    mActSameColorIcon = new QAction(tr("Allow Pegs of the Same Color"), this);
    mActSameColorIcon->setCheckable(true);
    mActSameColorIcon->setChecked(true);
    mActSameColorIcon->setToolTip(tr("Disallow Pegs of the Same Color"));
    mActSameColorIcon->setIcon(QIcon(":/img/same_color_1.png"));
    connect(mActSameColorIcon, SIGNAL(triggered(bool)), this, SLOT(SameColorSlot(bool)));

    AutoCloseAction = new QAction(tr("Close Rows when the last Peg is placed"), this);
    AutoCloseAction->setCheckable(true);
    AutoCloseAction->setChecked(false);
    AutoCloseAction->setShortcut(tr("Ctrl+L"));
    connect(AutoCloseAction, SIGNAL(triggered()), this, SLOT(AutoCloseSlot()));

    mLaunchHelpAction = new QAction(tr("Online &Help"), this);
    mLaunchHelpAction->setIcon(QIcon(":/img/help.png"));
    mLaunchHelpAction->setShortcut(tr("F1"));
    connect(mLaunchHelpAction, SIGNAL(triggered()), this, SLOT(OnlineHelpSlot()));

    AboutAction = new QAction(tr("About &ColorCode"), this);
    AboutAction->setIcon(QIcon(":/img/help-about.png"));
    AboutAction->setShortcut(tr("Ctrl+A"));
    connect(AboutAction, SIGNAL(triggered()), this, SLOT(AboutSlot()));

    AboutQtAction = new QAction(tr("About &Qt"), this);
    AboutQtAction->setIcon(QIcon(":/img/qt.png"));
    AboutQtAction->setShortcut(tr("Ctrl+I"));
    connect(AboutQtAction, SIGNAL(triggered()), this, SLOT(AboutQtSlot()));

    RandRowAction = new QAction(tr("Fill Row by Random"), this);
    RandRowAction->setIcon(QIcon(":/img/system-switch-user.png"));
    RandRowAction->setShortcut(tr("Ctrl+R"));
    connect(RandRowAction, SIGNAL(triggered()), this, SLOT(RandRowSlot()));

    PrevRowAction = new QAction(tr("Duplicate Previous Row"), this);
    PrevRowAction->setIcon(QIcon(":/img/edit-copy.png"));
    PrevRowAction->setShortcut(tr("Ctrl+D"));
    connect(PrevRowAction, SIGNAL(triggered()), this, SLOT(PrevRowSlot()));

    ClearRowAction = new QAction(tr("Clear Row"), this);
    ClearRowAction->setIcon(QIcon(":/img/edit-clear.png"));
    ClearRowAction->setShortcut(tr("Ctrl+C"));
    connect(ClearRowAction, SIGNAL(triggered()), this, SLOT(ClearRowSlot()));

    mActLevelEasy = new QAction(tr("Beginner (2 Colors, 2 Slots, Doubles)"), this);
    mActLevelEasy->setData(0);
    mActLevelEasy->setCheckable(true);
    mActLevelEasy->setChecked(false);
    connect(mActLevelEasy, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelClassic = new QAction(tr("Easy (4 Colors, 3 Slots, No Doubles)"), this);
    mActLevelClassic->setData(1);
    mActLevelClassic->setCheckable(true);
    mActLevelClassic->setChecked(false);
    connect(mActLevelClassic, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelMedium = new QAction(tr("Classic (6 Colors, 4 Slots, Doubles)"), this);
    mActLevelMedium->setData(2);
    mActLevelMedium->setCheckable(true);
    mActLevelMedium->setChecked(true);
    connect(mActLevelMedium, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelChallenging = new QAction(tr("Challenging (8 Colors, 4 Slots, Doubles)"), this);
    mActLevelChallenging->setData(3);
    mActLevelChallenging->setCheckable(true);
    mActLevelChallenging->setChecked(false);
    connect(mActLevelChallenging, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelHard = new QAction(tr("Hard (10 Colors, 5 Slots, Doubles)"), this);
    mActLevelHard->setData(4);
    mActLevelHard->setCheckable(true);
    mActLevelHard->setChecked(false);
    connect(mActLevelHard, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));


    mActGetGuess = new QAction(tr("Computer's Guess"), this);
    mActGetGuess->setIcon(QIcon(":/img/business_user.png"));
    mActGetGuess->setShortcut(tr("Ctrl+H"));
    connect(mActGetGuess, SIGNAL(triggered()), this, SLOT(GetGuessSlot()));
}

void ColorCode::InitMenus()
{
    GameMenu = mMenuBar->addMenu(tr("&Game"));
    GameMenu->addAction(NewGameAction);
    GameMenu->addAction(RestartGameAction);
    GameMenu->addAction(GiveInAction);
    GameMenu->addSeparator();
    GameMenu->addAction(ExitAction);

    RowMenu = mMenuBar->addMenu(tr("&Row"));
    RowMenu->addAction(RandRowAction);
    RowMenu->addAction(PrevRowAction);
    RowMenu->addAction(ClearRowAction);
    RowMenu->addSeparator();
    RowMenu->addAction(mActGetGuess);
    connect(RowMenu, SIGNAL(aboutToShow()), this, SLOT(UpdateRowMenuSlot()));

    SettingsMenu = mMenuBar->addMenu(tr("&Settings"));
    SettingsMenu->addAction(ShowMenubarAction);
    SettingsMenu->addAction(ShowToolbarAction);
    SettingsMenu->addAction(ShowStatusbarAction);
    SettingsMenu->addSeparator();
    SettingsMenu->addAction(mActResetColorsOrder);
    SettingsMenu->addAction(mActShowLetter);
    SettingsMenu->addSeparator();

    LevelMenu = SettingsMenu->addMenu(tr("Level Presets"));
    mLevelActions = new QActionGroup(LevelMenu);
    mLevelActions->addAction(mActLevelEasy);
    mLevelActions->addAction(mActLevelClassic);
    mLevelActions->addAction(mActLevelMedium);
    mLevelActions->addAction(mActLevelChallenging);
    mLevelActions->addAction(mActLevelHard);
    QList<QAction *> levelacts = mLevelActions->actions();
    LevelMenu->addActions(levelacts);
    SettingsMenu->addSeparator();

    SettingsMenu->addAction(SameColorAction);
    SettingsMenu->addAction(AutoCloseAction);

    HelpMenu = mMenuBar->addMenu(tr("&Help"));
    HelpMenu->addAction(mLaunchHelpAction);
    HelpMenu->addSeparator();
    HelpMenu->addAction(AboutAction);
    HelpMenu->addAction(AboutQtAction);

    RowContextMenu = new QMenu();
    RowContextMenu->addAction(RandRowAction);
    RowContextMenu->addAction(PrevRowAction);
    RowContextMenu->addAction(ClearRowAction);
    RowContextMenu->addSeparator();
    RowContextMenu->addAction(mActGetGuess);
    RowContextMenu->addSeparator();
    RowContextMenu->addAction(NewGameAction);
    RowContextMenu->addAction(RestartGameAction);
    RowContextMenu->addAction(GiveInAction);
    RowContextMenu->addSeparator();
    RowContextMenu->addAction(ExitAction);

    addActions(mMenuBar->actions());
}

void ColorCode::InitToolBars()
{
    mGameToolbar = addToolBar(tr("Game"));
    mGameToolbar->setAllowedAreas(Qt::NoToolBarArea);
    mGameToolbar->setFloatable(false);
    mGameToolbar->setIconSize(QSize(16, 16));
    mGameToolbar->setMovable(false);
    mGameToolbar->addAction(NewGameAction);
    mGameToolbar->addAction(RestartGameAction);
    mGameToolbar->addAction(GiveInAction);
    mGameToolbar->addSeparator();
    mGameToolbar->addAction(mActGetGuess);

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
    connect(mColorCntCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(mColorCntChangedSlot()));

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
        RowContextMenu->exec(e->globalPos());
    }
    else
    {
        GameMenu->exec(e->globalPos());
    }
}

void ColorCode::resizeEvent (QResizeEvent* e)
{
    Q_UNUSED(e);
    Scale();
}

void ColorCode::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (mCurRow != NULL && mGameState == STATE_RUNNING)
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
        break;
    }
}

void ColorCode::UpdateRowMenuSlot()
{
    if (mGameState != STATE_RUNNING || mCurRow == NULL)
    {
        RandRowAction->setEnabled(false);
        PrevRowAction->setEnabled(false);
        ClearRowAction->setEnabled(false);
        mActGetGuess->setEnabled(false);
        return;
    }
    else
    {
        RandRowAction->setEnabled(true);
        mActGetGuess->setEnabled(true);
    }

    if (mCurRow->GetIx() < 1)
    {
        PrevRowAction->setEnabled(false);
    }
    else
    {
        PrevRowAction->setEnabled(true);
    }

    if (mCurRow->GetPegCnt() == 0)
    {
        ClearRowAction->setEnabled(false);
    }
    else
    {
        ClearRowAction->setEnabled(true);
    }
}

void ColorCode::RestartGameSlot()
{
    ResetRows();
    SetState(STATE_RUNNING);

    mCurRow = NULL;
    mSolver->RestartGame();
    NextRow();
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
    if (!ShowToolbarAction->isChecked())
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
    if (!ShowMenubarAction->isChecked())
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
    if (!ShowStatusbarAction->isChecked())
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

void ColorCode::ShowLetterSlot()
{
    bool checked = mActShowLetter->isChecked();
    vector<ColorPeg *>::iterator it;
    for (it = mAllPegs.begin(); it < mAllPegs.end(); it++)
    {
        (*it)->ShowLetter(checked);
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
    if (mGameState == STATE_RUNNING && mGuessCnt > 1)
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

}

void ColorCode::mColorCntChangedSlot()
{
    if (mNoAct)
    {
        return;
    }

    int r = QMessageBox::Yes;
    if (mGameState == STATE_RUNNING && mGuessCnt > 1)
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
    if (mGameState == STATE_RUNNING && mGuessCnt > 1)
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

void ColorCode::ForceLevelSlot()
{
    mNoAct = true;
    int ix = mLevelActions->checkedAction()->data().toInt();

    if (ix < 0 || ix > 4)
    {
        return;
    }

    int i;
    i = mColorCntCmb->findData(mLevelSettings[ix][0]);
    if (i != -1 && mColorCntCmb->currentIndex() != i)
    {
        mColorCntCmb->setCurrentIndex(i);
    }
    i = mPegCntCmb->findData(mLevelSettings[ix][1]);
    if (i != -1 && mPegCntCmb->currentIndex() != i)
    {
        mPegCntCmb->setCurrentIndex(i);
    }

    SetSameColor((mLevelSettings[ix][2] == 1));

    int r = QMessageBox::Yes;
    if (mGameState == STATE_RUNNING && mGuessCnt > 1)
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
    cp->setZValue(++ColorCode::mMaxZ);

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
    cp->setZValue(++ColorCode::mMaxZ);

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

    scene->clearSelection();
    scene->clearFocus();

    int i;
    if (cp->GetSort() == 0)
    {
        bool snapped = false;
        QList<QGraphicsItem *> list = scene->items(QPointF(cp->pos().x() + 18, cp->pos().y() + 18));

        for (i = 0; i < list.size(); ++i)
        {
            if (mCurRow != NULL && list.at(i) == mCurRow)
            {
                 snapped = mCurRow->SnapCP(cp);
                 break;
            }
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
    std::vector<int> s = mPegRows[ix]->GetSolution();
    if (s.size() == (unsigned) mPegCnt)
    {
        if (AutoCloseAction->isChecked())
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

void ColorCode::HintPressedSlot(int)
{
    mCurRow->CloseRow();
    ResolveRow();
    NextRow();
    ResolveGame();
}

void ColorCode::RandRowSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

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

void ColorCode::GetGuessSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING || mSolver->mBusy)
    {
        return;
    }

    mCurRow->ClearRow();
    mActGetGuess->setEnabled(false);
    int* row = mSolver->GuessOut();
    mActGetGuess->setEnabled(true);
    if (row == NULL)
    {
        return;
    }

    ColorPeg* peg;
    int i;
    for (i = 0; i < mPegCnt; ++i)
    {
        if (row[i] < 0 || row[i] >= MAX_COLOR_CNT)
        {
            ;
        }
        peg = CreatePeg(row[i]);
        mCurRow->ForceSnap(peg, i);
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

void ColorCode::TestSlot()
{
    ;
}

void ColorCode::CheckSameColorsSetting()
{
    if (mColorCnt < mPegCnt)
    {
        if (SameColorAction->isEnabled())
        {
            SameColorAction->setEnabled(false);
        }
        if (mActSameColorIcon->isEnabled())
        {
            mActSameColorIcon->setEnabled(false);
        }
        if (!SameColorAction->isChecked())
        {
            SameColorAction->setChecked(true);
        }
        if (!mActSameColorIcon->isChecked())
        {
            mActSameColorIcon->setChecked(true);
        }
    }
    else
    {
        if (!SameColorAction->isEnabled())
        {
            SameColorAction->setEnabled(true);
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
    SameColorAction->setChecked(checked);
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
        if ( mLevelSettings[i][0] == mColorCnt
             && mLevelSettings[i][1] == mPegCnt
             && mLevelSettings[i][2] == mDoubles )
        {
            ix = i;
            break;
        }
    }

    if (ix > -1)
    {
        QList<QAction *> levelacts = mLevelActions->actions();
        levelacts.at(ix)->setChecked(true);
    }
    else
    {
        QAction* act = mLevelActions->checkedAction();
        if (act != NULL)
        {
            act->setChecked(false);
        }
    }
}

void ColorCode::ResetGame()
{
    int i;

    if (mPegCnt > 0)
    {
        for (i = 0; i < mPegCnt; ++i)
        {
            if (mSolPegs[i] != NULL)
            {
                RemovePeg(mSolPegs[i]);
                mSolPegs[i] = NULL;
            }
        }

        delete [] mSolPegs;
        mSolPegs = NULL;
    }

    SetPegCnt();

    mSolPegs = new ColorPeg* [mPegCnt];
    for (int i = 0; i < mPegCnt; ++i)
    {
        mSolPegs[i] = NULL;
    }

    mSolRow->SetState(mPegCnt, false);

    ResetRows();

    mGameId = qrand();
    mGuessCnt = 0;

    SetColorCnt();
    
    CheckSameColorsSetting();

    mDoubles = (int) SameColorAction->isChecked();
    CheckLevel();
}

void ColorCode::ResetRows()
{
    for (int i = 0; i < ROW_CNT; ++i)
    {
        mPegRows[i]->Reset(mPegCnt);
        mHintBtns[i]->Reset(mPegCnt);
    }
}

void ColorCode::NewGame()
{
    ResetGame();
    QString doubles = (mDoubles == 1) ? tr("Yes") : tr("No");
    QString colors = QString::number(mColorCnt, 10);
    QString pegs = QString::number(mPegCnt, 10);
    mStatusLabel->setText(tr("Pegs of Same Color") + ": <b>" + doubles + "</b> :: " + tr("Slots") + ": <b>" + pegs + "</b> :: " + tr("Colors") + ": <b>" + colors + "</b> ");
    SetState(STATE_RUNNING);

    mCurRow = NULL;
    SetSolution();
    mSolver->NewGame(mColorCnt, mPegCnt, mDoubles, 2, ROW_CNT);
    NextRow();
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
        ShowMsgSlot(tr("Place Your pegs ..."));
    }
}

void ColorCode::ResolveRow()
{
    std::vector<int> res;
    std::vector<int> left1;
    std::vector<int> left0;
    std::vector<int> rowsol = mCurRow->GetSolution();
    mSolver->GuessIn(&rowsol);

    int i, p0, p1;
    for (i = 0; i < mPegCnt; ++i)
    {
        p0 = rowsol.at(i);
        p1 = mSolution.at(i);
        if (p0 == p1)
        {
            res.push_back(2);
        }
        else
        {
            left0.push_back(p0);
            left1.push_back(p1);
        }
    }

    if (res.size() == (unsigned) mPegCnt)
    {
        SetState(STATE_WON);
    }
    else
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
                    res.push_back(1);
                    left1.erase(left1.begin() + j);
                    break;
                }
            }
        }
    }
    mSolver->ResIn(&res);
    mHintBtns[mCurRow->GetIx()]->DrawHints(res);
}

void ColorCode::ResolveGame()
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
        case STATE_RUNNING:
        default:
            return;
        break;
    }

    ShowSolution();
}

void ColorCode::SetPegCnt()
{
    int pegcnt = mPegCntCmb->itemData(mPegCntCmb->currentIndex(), IdRole).toInt();
    pegcnt = max(2, min(5, pegcnt));
    mPegCnt = pegcnt;
    mXOffs = 160 - mPegCnt * 20;
}

void ColorCode::SetColorCnt()
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

void ColorCode::SetSolution()
{
    mSolution.clear();
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

void ColorCode::ShowSolution()
{
    ColorPeg* peg;
    for (int i = 0; i < mPegCnt; ++i)
    {
        peg = CreatePeg(mSolution.at(i));
        peg->setPos(mXOffs + i * 40 + 2, 72);
        peg->SetBtn(false);
        peg->SetEnabled(false);
        mSolPegs[i] = peg;
    }
}

void ColorCode::SetState(const int s)
{
    if (mGameState == s)
    {
        return;
    }

    mGameState = s;

    bool running = mGameState == STATE_RUNNING;

    RestartGameAction->setEnabled(running);
    GiveInAction->setEnabled(running);
    mActGetGuess->setEnabled(running);
}

void ColorCode::Scale()
{
    qreal w = geometry().width() - 4;
    qreal h = geometry().height() - 4;

    if (ShowStatusbarAction->isChecked())
    {
        h -= statusBar()->height();
    }
    if (ShowMenubarAction->isChecked())
    {
        h -= mMenuBar->height();
    }
    if (ShowToolbarAction->isChecked())
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
        ix = ColorCode::mMaxZ % mColorCnt;
    }

    PegType *pt = mTypesMap[ix];
    ColorPeg *peg;

    if (mPegBuff.empty())
    {
        peg = new ColorPeg;
        peg->SetPegType(pt);
        peg->SetBtn(true);
        peg->ShowLetter(mActShowLetter->isChecked());
        peg->SetId(ColorCode::mMaxZ);
        scene->addItem(peg);
        peg->setPos(mBtnPos[ix]);
        peg->setZValue(24);
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

    ++ColorCode::mMaxZ;

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
