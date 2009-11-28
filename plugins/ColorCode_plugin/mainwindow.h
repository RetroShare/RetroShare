#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include <QMainWindow>

struct PegType
{
    int ix;
    QRadialGradient grad;
    QColor  pencolor;
};

//typedef std::vector<PegType> TypesMap;

class QAction;
class QActionGroup;
class QComboBox;
class QGraphicsItem;
class QGraphicsTextItem;
class QGraphicsScene;
class QGraphicsView;
class ColorPeg;
class PegRow;
class RowHint;
class Msg;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow();

        static const int STATE_RUNNING;
        static const int STATE_WON;
        static const int STATE_LOST;
        static const int STATE_GAVE_UP;

        static const int MAX_COLOR_CNT;
        static const int LEVEL_COLOR_CNTS[3];

        int ROW_CNT;
        int POS_CNT;

    public slots:
        void PegPressSlot(ColorPeg *cp);
        void PegReleasedSlot(ColorPeg *cp);
        void RowSolutionSlot(int ix);
        void HintPressedSlot(int ix);
        void ShowMsgSlot(QString msg);
        void RemovePegSlot(ColorPeg* cp);

    protected :
        void resizeEvent(QResizeEvent* e);
        void contextMenuEvent(QContextMenuEvent* e);

    private slots:
        void InitRows();
        void InitPegBtns();
        void NewGameSlot();
        void RestartGameSlot();
        void GiveInSlot();
        void ResetRows();
        void AboutSlot();
        void AboutQtSlot();
        void ShowToolbarSlot();
        void ShowMenubarSlot();
        void ShowStatusbarSlot();
        void SameColorSlot();
        void AutoCloseSlot();
        void ForceLevelSlot();

        void RandRowSlot();
        void PrevRowSlot();
        void ClearRowSlot();

        void LevelChangedSlot();

        void UpdateRowMenuSlot();

        void TestSlot();

    private:
        enum Level { LEVEL_EASY,
                     LEVEL_MEDIUM,
                     LEVEL_HARD };

        static Level mLevel;
        static int mColorCnt;
        static int mMaxZ;

        int mGameState;

        std::vector<int> mSolution;

        QGraphicsScene* scene;
        QGraphicsView* view;
        QSize* mOrigSize;

        Msg* mMsg;

        PegRow* mCurRow;

        std::vector<ColorPeg *> mPegBuff;
        PegRow* mSolRow;
        PegRow* mPegRows[10];
        RowHint* mHintBtns[10];

        PegType* mTypesMap[10];
        ColorPeg* mPegBtns[10];
        ColorPeg* mSolPegs[4];
        QPoint mBtnPos[10];

        void FillTypesMap();

        void InitSolution();
        void InitActions();
        void InitMenus();
        void InitToolBars();

        void SetLevel();
        void SetState(const int s);
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
        QToolBar* mGameToolbar;
        QToolBar* mLevelToolbar;

        QComboBox* mLevelCmb;

        QActionGroup* mLevelActions;
        QAction* NewGameAction;
        QAction* RestartGameAction;
        QAction* GiveInAction;
        QAction* ShowToolbarAction;
        QAction* ShowMenubarAction;
        QAction* ShowStatusbarAction;
        QAction* SameColorAction;
        QAction* AutoCloseAction;
        QAction* AboutAction;
        QAction* AboutQtAction;
        QAction* ExitAction;

        QAction* mActLevelEasy;
        QAction* mActLevelMedium;
        QAction* mActLevelHard;

        QAction* RandRowAction;
        QAction* PrevRowAction;
        QAction* ClearRowAction;

};

#endif // MAINWINDOW_H
