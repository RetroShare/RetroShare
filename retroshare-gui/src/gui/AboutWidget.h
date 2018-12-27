/*******************************************************************************
 * gui/AboutWidget.h                                                           *
 *                                                                             *
 * Copyright (C) 2012 Retroshare Team <retroshare.project@gmail.com>           *
 * Copyright (C) 2008 Unipro, Russia (http://ugene.unipro.ru)                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _GB2_ABOUT_DIALOG_
#define _GB2_ABOUT_DIALOG_

#include "ui_AboutWidget.h"

#include <QBasicTimer>
#include <QResizeEvent>
#include <QPointer>

#include <QDialog>
#include <QLabel>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>


class AWidget;
class TBoard;
class NextPieceLabel;

class AboutWidget : public QWidget, public Ui::AboutWidget
{
    Q_OBJECT
public:
    AboutWidget(QWidget *parent = 0);

private slots:
    void sl_scoreChanged(int);
    void sl_levelChanged(int);
    void on_help_button_clicked();
    void on_copy_button_clicked();

signals:
    void si_scoreChanged(QString);
    void si_maxScoreChanged(QString);
    void si_levelChanged(QString);

protected:
    //void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
private:
    void switchPages();
    void installAWidget();
    void installTWidget();
    void updateTitle();

    AWidget*     aWidget;
    TBoard*     tWidget;
};

//////////////////////////////////////////////////////////////////////////
// A
class AWidget : public QWidget {
    Q_OBJECT

public:
    AWidget();

    void switchState() ;

protected:
    void timerEvent(QTimerEvent* e);
    void paintEvent(QPaintEvent* e);
    //void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent *);
	//void keyPressEvent(QKeyEvent *e);

private:
	void initImages();
	void computeNextState();
    void calcWater(int npage, int density);

    void addBlob(int x, int y, int radius, int height);

    void drawWater(QRgb* srcImage, QRgb* dstImage);

    static QRgb shiftColor(QRgb color,int shift) {
        return qRgb(qBound(0, qRed(color) - shift,  255),
            qBound(0, qGreen(color) - shift,255),
            qBound(0, qBlue(color) - shift, 255));
    }
	void initGoL();
	void drawBitField();

    int			    page;
    int			    density;
    std::vector<int>    bitfield1;
    std::vector<int>    bitfield2;
    QImage          image1;
    QImage          image2;

    bool			mImagesReady ;
    int				mState;
    int 			mTimerId;
    float			mStep;
    int 			mMaxStep;
};

//////////////////////////////////////////////////////////////////////////
// T

class TPiece {
public:
    enum Shape { NoShape, ZShape, SShape, LineShape, TShape, SquareShape, LShape, MirroredLShape };

    TPiece() { setShape(NoShape); }

    void setRandomShape();
    void setShape(Shape shape);

    Shape shape() const { return pieceShape; }
    int x(int index) const { return coords[index][0]; }
    int y(int index) const { return coords[index][1]; }
    int minX() const;
    int maxX() const;
    int minY() const;
    int maxY() const;
    TPiece rotatedLeft() const;
    TPiece rotatedRight() const;

private:
    void setX(int index, int x) { coords[index][0] = x; }
    void setY(int index, int y) { coords[index][1] = y; }

    Shape pieceShape;
    int coords[4][2];
};

class TBoard : public QWidget {
    Q_OBJECT

public:
    TBoard(QWidget *parent = 0);
    ~TBoard();
    int heightForWidth ( int w ) const;
    int getScore() const {return score;}
    int getMaxScore() const {return qMax(maxScore, score);}
    int getLevel() const {return level;}
    void setNextPieceLabel(QLabel *label) {nextPieceLabel = label;}
    int squareWidth() const { return  boardRect().width()  / BoardWidth; }
    int squareHeight() const { return boardRect().height() / BoardHeight; }

public slots:
    void start();
    void pause();
    
signals:
    void scoreChanged(int score);
    void levelChanged(int level);
    void linesRemovedChanged(int numLines);

protected:
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void timerEvent(QTimerEvent *event);

private:

    enum { BoardWidth = 10, BoardHeight = 22 };

    TPiece::Shape &shapeAt(int x, int y) { return board[(y * BoardWidth) + x]; }
    int timeoutTime() const { return 1000 / (1 + level); }
    QRect boardRect() const {return QRect(1, 1, width()-2, height()-2);}
    QRect frameRect() const {return QRect(0, 0, width()-1, height()-1);}
    void clearBoard();
    void dropDown();
    void oneLineDown();
    void pieceDropped(int dropHeight);
    void removeFullLines();
    void newPiece();
    void showNextPiece();
    bool tryMove(const TPiece &newPiece, int newX, int newY);
    void drawSquare(QPainter &painter, int x, int y, TPiece::Shape shape);

    QBasicTimer timer;
    QPointer<QLabel> nextPieceLabel;
    bool isStarted;
    bool isPaused;
    bool isWaitingAfterLine;
    TPiece curPiece;
    TPiece nextPiece;
    int curX;
    int curY;
    int numLinesRemoved;
    int numPiecesDropped;
    int score;
    int maxScore;
    int level;
    TPiece::Shape board[BoardWidth * BoardHeight];
};

class NextPieceLabel : public QLabel {
public:
    NextPieceLabel(QWidget* parent = 0);
};


#endif
