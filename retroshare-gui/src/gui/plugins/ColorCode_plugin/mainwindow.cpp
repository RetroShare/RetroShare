#include <QtGui>

#include "mainwindow.h"
#include "colorpeg.h"
#include "pegrow.h"
#include "rowhint.h"
#include "msg.h"
#include "about.h"

using namespace std;

const int IdRole = Qt::UserRole;

MainWindow::Level MainWindow::mLevel = LEVEL_EASY;
int MainWindow::mColorCnt = 0;
int MainWindow::mMaxZ     = 0;

const int MainWindow::STATE_RUNNING = 0;
const int MainWindow::STATE_WON     = 1;
const int MainWindow::STATE_LOST    = 2;
const int MainWindow::STATE_GAVE_UP = 3;

const int MainWindow::MAX_COLOR_CNT = 10;
const int MainWindow::LEVEL_COLOR_CNTS[3] = {6, 8, MainWindow::MAX_COLOR_CNT};

MainWindow::MainWindow()
{
    ROW_CNT     = 10;
    POS_CNT     = 4;

    //setMaximumSize(334, 649);

    mOrigSize = NULL;

    setWindowTitle(tr("ColorCode"));
    setWindowIcon(QIcon(QPixmap(":/img/cc16.png")));
    setIconSize(QSize(16, 16));

    mMsg = new Msg;
    scene = new QGraphicsScene(0, 0, 320, 560);

    scene->setBackgroundBrush(QImage(":/img/bm03.png"));
    //scene->setItemIndexMethod(scene->NoIndex);

    view = new QGraphicsView;
    view->setScene(scene);
    view->setGeometry(0, 0, 320, 540);
    view->setDragMode(QGraphicsView::NoDrag);
    view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    //view->setContextMenuPolicy(Qt::ActionsContextMenu);
    //view->setContextMenuPolicy(Qt::DefaultContextMenu);
    view->setContextMenuPolicy(Qt::NoContextMenu);
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setCentralWidget(view);
    scene->addItem(mMsg);
    mMsg->setPos(20, 0);

    //view->setBackgroundBrush(QImage(":/img/metal5.png"));
    //view->setCacheMode(QGraphicsView::CacheBackground);

    mCurRow = NULL;
    mColorCnt = MAX_COLOR_CNT;

    FillTypesMap();

    InitActions();
    InitMenus();
    InitToolBars();

    statusBar();    

    InitSolution();
    InitRows();
    InitPegBtns();

    NewGame();
}

void MainWindow::Scale()
{
    qreal w = geometry().width() - 4;
    qreal h = geometry().height() - 4;

    if (ShowStatusbarAction->isChecked())
    {
        h -= statusBar()->height();
    }
    if (ShowMenubarAction->isChecked())
    {
        h -= menuBar()->height();
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
        mOrigSize = new QSize(320, 541);
        scene->setSceneRect(QRectF(0, 0, 320, 540));
    }
    else
    {
        qreal sc = min(w / mOrigSize->width(), h / mOrigSize->height());
        view->resetTransform();
        view->scale(sc, sc);
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent* e)
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

void MainWindow::UpdateRowMenuSlot()
{
    if (mGameState != STATE_RUNNING || mCurRow == NULL)
    {
        RandRowAction->setEnabled(false);
        PrevRowAction->setEnabled(false);
        ClearRowAction->setEnabled(false);
        return;
    }
    else
    {
        RandRowAction->setEnabled(true);
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

void MainWindow::resizeEvent (QResizeEvent* e)
{
    Q_UNUSED(e);
    Scale();
}

void MainWindow::NewGame()
{
    SetLevel();
    ResetRows();
    SetState(STATE_RUNNING);

    mCurRow = NULL;
    SetSolution();
    NextRow();
}

void MainWindow::RestartGameSlot()
{
    for (int i = 0; i < 4; ++i)
    {
        if (mSolPegs[i] != NULL)
        {
            RemovePeg(mSolPegs[i]);
            mSolPegs[i] = NULL;
        }
    }
    ResetRows();
    SetState(STATE_RUNNING);

    mCurRow = NULL;
    NextRow();
}

void MainWindow::NewGameSlot()
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

void MainWindow::GiveInSlot()
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
/*
void MainWindow::AboutSlot()
{
    QMessageBox::about( this,
                        tr("About"),
                        tr("<b>ColorCode</b><br>A needful game to train your brain ;-)<br><br>Version: 0.0.1<br>Author: Dirk Laebisch"));
}
*/
void MainWindow::AboutSlot()
{
    About ab(this);
    ab.exec();
}

void MainWindow::AboutQtSlot()
{
    QMessageBox::aboutQt( this,
                        tr("About Qt"));
}

void MainWindow::SetSolution()
{
    static bool firstrun = true;

    if (firstrun)
    {
        firstrun = false;
        QTime midnight(0, 0, 0);
        qsrand(midnight.secsTo(QTime::currentTime()));
    }

    mSolution.clear();    
    int i, rndm;
    int check[mColorCnt];
    for (i = 0; i < mColorCnt; ++i)
    {
        check[i] = 0;
    }

    for (i = 0; i < POS_CNT; ++i)
    {
        rndm = qrand() % mColorCnt;
        if (!SameColorAction->isChecked() && check[rndm] != 0)
        {
            --i;
            continue;
        }
        mSolution.push_back(rndm);
        check[rndm] = 1;
        if (mSolPegs[i] != NULL)
        {
            RemovePeg(mSolPegs[i]);
            mSolPegs[i] = NULL;
        }
    }
    mSolRow->update(mSolRow->boundingRect());
}

void MainWindow::ShowSolution()
{
    ColorPeg* peg;
    for (int i = 0; i < POS_CNT; ++i)
    {
        peg = CreatePeg(mSolution.at(i));
        peg->setPos(i * 40 + 80, 62);
        peg->SetBtn(false);
        peg->SetEnabled(false);
        mSolPegs[i] = peg;
    }
}

void MainWindow::ResetRows()
{
    for (int i = 0; i < ROW_CNT; ++i)
    {
        mPegRows[i]->Reset();
        mHintBtns[i]->Reset();
    }
}

void MainWindow::FillTypesMap()
{
    int ix;

    QRadialGradient grad = QRadialGradient(20, 20, 40, 5, 5);
    QRadialGradient grad2 = QRadialGradient(18, 18, 18, 12, 12);

    ix = 0;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FFFF80"));
    grad.setColorAt(1.0, QColor("#C05800"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;
    

    ix = 1;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FF3300"));
    grad.setColorAt(1.0, QColor("#400040"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 2;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#33CCFF"));
    grad.setColorAt(1.0, QColor("#000080"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 3;
    mTypesMap[ix] = new PegType;
    grad2.setColorAt(0.0, Qt::white);
    grad2.setColorAt(0.5, QColor("#f8f8f0"));
    grad2.setColorAt(0.7, QColor("#f0f0f0"));
    //grad2.setColorAt(0.85, QColor("#d8dEFF"));
    grad2.setColorAt(1.0, QColor("#d0d0d0"));
    mTypesMap[ix]->grad = grad2;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 4;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#808080"));
    grad.setColorAt(1.0, Qt::black);
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 5;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#66FF33"));
    grad.setColorAt(1.0, QColor("#385009"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 6;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FF9900"));
    grad.setColorAt(1.0, QColor("#A82A00"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::red;

    ix = 7;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#BA88FF"));
    grad.setColorAt(1.0, QColor("#38005D"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 8;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#00FFFF"));
    grad.setColorAt(1.0, QColor("#004040"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;

    ix = 9;
    mTypesMap[ix] = new PegType;
    grad.setColorAt(0.0, QColor("#FFC0FF"));
    grad.setColorAt(1.0, QColor("#800080"));
    mTypesMap[ix]->grad = grad;
    mTypesMap[ix]->ix = ix;
    mTypesMap[ix]->pencolor = Qt::green;
}

void MainWindow::InitActions()
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
    //connect(ExitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(ExitAction, SIGNAL(triggered()), this, SLOT(close()));

    ShowToolbarAction = new QAction(tr("Show Toolbar"), this);
    ShowToolbarAction->setCheckable(true);
    ShowToolbarAction->setChecked(true);
    //ShowToolbarAction->setIcon(QIcon(":/img/configure-toolbars.png"));
    ShowToolbarAction->setShortcut(tr("Ctrl+T"));
    connect(ShowToolbarAction, SIGNAL(triggered()), this, SLOT(ShowToolbarSlot()));

    ShowMenubarAction = new QAction(tr("Show Menubar"), this);
    ShowMenubarAction->setCheckable(true);
    ShowMenubarAction->setChecked(true);
    //ShowToolbarAction->setIcon(QIcon(":/img/configure-toolbars.png"));
    ShowMenubarAction->setShortcut(tr("Ctrl+M"));
    connect(ShowMenubarAction, SIGNAL(triggered()), this, SLOT(ShowMenubarSlot()));

    ShowStatusbarAction = new QAction(tr("Show Statusbar"), this);
    ShowStatusbarAction->setCheckable(true);
    ShowStatusbarAction->setChecked(true);
    //ShowToolbarAction->setIcon(QIcon(":/img/configure-toolbars.png"));
    ShowStatusbarAction->setShortcut(tr("Ctrl+S"));
    connect(ShowStatusbarAction, SIGNAL(triggered()), this, SLOT(ShowStatusbarSlot()));

    SameColorAction = new QAction(tr("Allow Pegs of the Same Color"), this);
    SameColorAction->setCheckable(true);
    SameColorAction->setChecked(true);
    //SameColorAction->setIcon(QIcon(":/img/configure-toolbars.png"));
    SameColorAction->setShortcut(tr("Ctrl+Shift+C"));
    connect(SameColorAction, SIGNAL(triggered()), this, SLOT(SameColorSlot()));

    AutoCloseAction = new QAction(tr("Close Rows when the 4th Peg is placed"), this);
    AutoCloseAction->setCheckable(true);
    AutoCloseAction->setChecked(false);
    //AutoCloseAction->setIcon(QIcon(":/img/configure-toolbars.png"));
    AutoCloseAction->setShortcut(tr("Ctrl+4"));
    connect(AutoCloseAction, SIGNAL(triggered()), this, SLOT(AutoCloseSlot()));

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

    mActLevelEasy = new QAction(tr("Easy - 6 Colors"), this);
    mActLevelEasy->setData(int(LEVEL_EASY));
    mActLevelEasy->setCheckable(true);
    mActLevelEasy->setChecked(false);
    connect(mActLevelEasy, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelMedium = new QAction(tr("Medium - 8 Colors"), this);
    mActLevelMedium->setData(int(LEVEL_MEDIUM));
    mActLevelMedium->setCheckable(true);
    mActLevelMedium->setChecked(true);
    //mActLevelMedium->setEnabled(false);
    connect(mActLevelMedium, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));

    mActLevelHard = new QAction(tr("Hard - 10 Colors"), this);
    mActLevelHard->setData(int(LEVEL_HARD));
    mActLevelHard->setCheckable(true);
    mActLevelHard->setChecked(false);
    connect(mActLevelHard, SIGNAL(triggered()), this, SLOT(ForceLevelSlot()));
}

void MainWindow::InitMenus()
{
    GameMenu = menuBar()->addMenu(tr("&Game"));
    GameMenu->addAction(NewGameAction);
    GameMenu->addAction(RestartGameAction);
    GameMenu->addAction(GiveInAction);
    GameMenu->addSeparator();
    GameMenu->addAction(ExitAction);

    RowMenu = menuBar()->addMenu(tr("&Row"));
    RowMenu->addAction(RandRowAction);
    RowMenu->addAction(PrevRowAction);
    RowMenu->addAction(ClearRowAction);
    connect(RowMenu, SIGNAL(aboutToShow()), this, SLOT(UpdateRowMenuSlot()));

    SettingsMenu = menuBar()->addMenu(tr("&Settings"));
    SettingsMenu->addAction(ShowMenubarAction);
    SettingsMenu->addAction(ShowToolbarAction);
    SettingsMenu->addAction(ShowStatusbarAction);
    SettingsMenu->addSeparator();

    LevelMenu = SettingsMenu->addMenu(tr("Level"));
    LevelMenu->addAction(mActLevelEasy);
    LevelMenu->addAction(mActLevelMedium);
    LevelMenu->addAction(mActLevelHard);
    mLevelActions = new QActionGroup(LevelMenu);
    mActLevelEasy->setActionGroup(mLevelActions);
    mActLevelMedium->setActionGroup(mLevelActions);
    mActLevelHard->setActionGroup(mLevelActions);

    SettingsMenu->addSeparator();
    SettingsMenu->addAction(SameColorAction);
    SettingsMenu->addAction(AutoCloseAction);

    HelpMenu = menuBar()->addMenu(tr("&Help"));
    HelpMenu->addAction(AboutAction);
    HelpMenu->addAction(AboutQtAction);

    RowContextMenu = new QMenu();
    RowContextMenu->addAction(RandRowAction);
    RowContextMenu->addAction(PrevRowAction);
    RowContextMenu->addAction(ClearRowAction);
    RowContextMenu->addSeparator();
    RowContextMenu->addAction(NewGameAction);
    RowContextMenu->addAction(RestartGameAction);
    RowContextMenu->addAction(GiveInAction);
    RowContextMenu->addSeparator();
    RowContextMenu->addAction(ExitAction);

    addActions(menuBar()->actions());
}

void MainWindow::TestSlot()
{
}

void MainWindow::InitToolBars()
{
    mGameToolbar = addToolBar(tr("Game"));
    mGameToolbar->setAllowedAreas(Qt::NoToolBarArea);
    mGameToolbar->setFloatable(false);
    mGameToolbar->setIconSize(QSize(16, 16));
    mGameToolbar->addAction(NewGameAction);
    mGameToolbar->addAction(RestartGameAction);
    mGameToolbar->addAction(GiveInAction);
    mGameToolbar->addSeparator();
    mGameToolbar->addAction(AboutAction);

    mLevelCmb = new QComboBox();
    mLevelCmb->setLayoutDirection(Qt::LeftToRight);
    mLevelCmb->setFixedWidth(132);
    mLevelCmb->addItem(tr("Easy - 6 Colors"), int(LEVEL_EASY));
    mLevelCmb->addItem(tr("Medium - 8 Colors"), int(LEVEL_MEDIUM));
    mLevelCmb->addItem(tr("Hard - 10 Colors"), int(LEVEL_HARD));
    mLevelCmb->setCurrentIndex(1);
    connect(mLevelCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(LevelChangedSlot()));

    mLevelToolbar = addToolBar(tr("Level"));
    mLevelToolbar->setAllowedAreas(Qt::NoToolBarArea);
    mLevelToolbar->setFloatable(false);
    mLevelToolbar->setIconSize(QSize(16, 16));
    mLevelToolbar->setLayoutDirection(Qt::RightToLeft);
    mLevelToolbar->addWidget(mLevelCmb);
}

void MainWindow::ShowToolbarSlot()
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

void MainWindow::ShowMenubarSlot()
{
    if (!ShowMenubarAction->isChecked())
    {
        menuBar()->hide();
    }
    else
    {
        menuBar()->show();
    }
    Scale();
}

void MainWindow::ShowStatusbarSlot()
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

void MainWindow::SameColorSlot()
{
    int r = QMessageBox::Yes;
    if (mGameState == STATE_RUNNING && mCurRow != NULL)
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

void MainWindow::AutoCloseSlot()
{

}

void MainWindow::LevelChangedSlot()
{
    int ix = mLevelCmb->itemData(mLevelCmb->currentIndex(), IdRole).toInt();
    if (mLevelActions->checkedAction()->data().toInt() != ix)
    {
        QList<QAction *> list = LevelMenu->actions();
        for (int i = 0; i < list.size(); ++i)
        {
            if (list.at(i)->data().toInt() == ix)
            {
                list.at(i)->setChecked(true);
                break;
            }
        }
    }

    int r = QMessageBox::Yes;
    if (mGameState == STATE_RUNNING && mCurRow != NULL)
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

void MainWindow::ForceLevelSlot()
{
    int i = mLevelActions->checkedAction()->data().toInt();

    int ix = mLevelCmb->findData(i);

    if (ix != -1 && mLevelCmb->currentIndex() != ix)
    {
        mLevelCmb->setCurrentIndex(ix);
    }
}

void MainWindow::SetLevel()
{
    int ix = mLevelCmb->itemData(mLevelCmb->currentIndex(), IdRole).toInt();
    ix = max(0, min(2, ix));
    Level lv = Level(ix);

    if (mLevel == lv)
    {
        return;
    }

    mLevel = lv;
    mColorCnt = LEVEL_COLOR_CNTS[int(mLevel)];

    int xpos = 260;
    int ystart = 120 + (MAX_COLOR_CNT - mColorCnt) * 40;
    int ypos;
    int i;

    for (i = 0; i < MAX_COLOR_CNT; ++i)
    {
        ypos = ystart + i * 40;
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

void MainWindow::InitSolution()
{
    mSolRow = new PegRow();
    mSolRow->setPos(78, 60);
    mSolRow->SetActive(false);
    scene->addItem(mSolRow);

    for (int i = 0; i < POS_CNT; ++i)
    {
        mSolPegs[i] = NULL;
    }
}

void MainWindow::InitRows()
{

    PegRow* row;
    RowHint* hint;

    int xpos = 78;
    int ypos = 120;
    int i;

    for (i = 0; i < ROW_CNT; ++i)
    {
        row = new PegRow();
        row->SetIx(i);
        row->setPos(QPoint(xpos, ypos + (ROW_CNT - (i + 1)) * 40));
        row->setZValue(++MainWindow::mMaxZ);
        row->SetActive(false);
        scene->addItem(row);
        mPegRows[i] = row;
        connect(row, SIGNAL(RemovePegSignal(ColorPeg*)), this, SLOT(RemovePegSlot(ColorPeg*)));
        connect(row, SIGNAL(RowSolutionSignal(int)), this, SLOT(RowSolutionSlot(int)));

        hint = new RowHint;
        hint->SetIx(i);
        hint->setPos(QPoint(20, ypos + (ROW_CNT - (i + 1)) * 40));
        hint->setZValue(++MainWindow::mMaxZ);
        hint->SetActive(false);
        scene->addItem(hint);
        mHintBtns[i] = hint;
        connect(hint, SIGNAL(HintPressedSignal(int)), this, SLOT(HintPressedSlot(int)));
    }
}

void MainWindow::InitPegBtns()
{
    for (int i = 0; i < MAX_COLOR_CNT; ++i)
    {
        mPegBtns[i] = NULL;
    }
}

ColorPeg* MainWindow::CreatePeg(int ix)
{
    if (ix < 0 || ix >= mColorCnt)
    {
        ix = MainWindow::mMaxZ % mColorCnt;
    }

    PegType *pt = mTypesMap[ix];
    ColorPeg *peg;

    if (mPegBuff.empty())
    {
        peg = new ColorPeg;
        peg->SetPegType(pt);
        peg->SetBtn(true);
        peg->SetId(MainWindow::mMaxZ);
        scene->addItem(peg);
        peg->setPos(mBtnPos[ix]);
        peg->setZValue(MainWindow::mMaxZ);
        scene->clearSelection();
        scene->update(mBtnPos[ix].x(), mBtnPos[ix].y(), 38, 38);
        peg->setSelected(true);

        connect(peg, SIGNAL(PegPressSignal(ColorPeg *)), this, SLOT(PegPressSlot(ColorPeg *)));
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
        //peg->SetId(MainWindow::mMaxZ);
        peg->setPos(mBtnPos[ix]);
        //peg->setZValue(MainWindow::mMaxZ);
        scene->clearSelection();
        scene->update(mBtnPos[ix].x(), mBtnPos[ix].y(), 38, 38);
        peg->setSelected(true);
    }



    ++MainWindow::mMaxZ;

    return peg;
}

void MainWindow::RemovePeg(ColorPeg* cp)
{
    if (cp != NULL)
    {
        cp->Reset();
        cp->setVisible(false);
        mPegBuff.push_back(cp);
    }
}

void MainWindow::RemovePegSlot(ColorPeg* cp)
{
    RemovePeg(cp);
}

void MainWindow::ShowMsgSlot(QString msg)
{
    statusBar()->showMessage(msg);
    mMsg->ShowMsg(msg);
}

void MainWindow::PegPressSlot(ColorPeg* cp)
{
    if (cp == NULL) { return; }
    if (cp->IsBtn())
    {
        cp->SetBtn(false);
        mPegBtns[cp->GetPegType()->ix] = CreatePeg(cp->GetPegType()->ix);
    }
    else
    {
    }
    cp->setZValue(++MainWindow::mMaxZ);
}

void MainWindow::PegReleasedSlot(ColorPeg* cp)
{
    if (cp == NULL || !cp) { return; }

    bool snapped = false;
    QList<QGraphicsItem *> list = scene->items(QPointF(cp->pos().x() + 18, cp->pos().y() + 18));
    int i = 0;
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

void MainWindow::RowSolutionSlot(int ix)
{
    std::vector<int> s = mPegRows[ix]->GetSolution();
    if (s.size() == 4)
    {
        if (AutoCloseAction->isChecked())
        {
            HintPressedSlot(ix);
        }
        else
        {
            mHintBtns[ix]->SetActive(true);
            ShowMsgSlot(tr("Press the Hint Field if You're done."));
        }
    }
    else
    {
        mHintBtns[ix]->SetActive(false);
        ShowMsgSlot(tr("Place Your pegs ..."));
    }

}

void MainWindow::HintPressedSlot(int)
{
    mCurRow->CloseRow();
    ResolveRow();
    NextRow();
    ResolveGame();
}

void MainWindow::RandRowSlot()
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

    for (i = 0; i < POS_CNT; ++i)
    {
        rndm = qrand() % mColorCnt;
        if (!SameColorAction->isChecked() && check[rndm] != 0)
        {
            --i;
            continue;
        }

        check[rndm] = 1;

        peg = CreatePeg(rndm);
        mCurRow->ForceSnap(peg, i);
    }
}

void MainWindow::PrevRowSlot()
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
    for (i = 0; i < POS_CNT; ++i)
    {
        peg = CreatePeg(prev.at(i));
        mCurRow->ForceSnap(peg, i);
    }
}

void MainWindow::ClearRowSlot()
{
    if (mCurRow == NULL || mGameState != STATE_RUNNING)
    {
        return;
    }

    mCurRow->ClearRow();
}

void MainWindow::ResolveRow()
{
    std::vector<int> res;
    std::vector<int> left1;
    std::vector<int> left0;
    std::vector<int> rowsol = mCurRow->GetSolution();
    int i, p0, p1;
    for (i = 0; i < POS_CNT; ++i)
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

    if (res.size() == (unsigned) POS_CNT)
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
    mHintBtns[mCurRow->GetIx()]->DrawHints(res);
}

void MainWindow::NextRow()
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
        mCurRow->SetActive(true);
        ShowMsgSlot(tr("Place Your pegs ..."));
    }
}

void MainWindow::ResolveGame()
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

void MainWindow::SetState(const int s)
{
    if (mGameState == s)
    {
        return;
    }

    mGameState = s;

    bool running = mGameState == STATE_RUNNING;

    RestartGameAction->setEnabled(running);
    GiveInAction->setEnabled(running);
}
