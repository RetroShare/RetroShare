/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2009, RetroShare Team
 * Copyright (C) 2008 Unipro, Russia (http://ugene.unipro.ru)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "AboutDialog.h"
#include "HelpDialog.h"

#include <retroshare/rsdisc.h>
#include <retroshare/rspeers.h>
#include "settings/rsharesettings.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QBrush>
#include <QMessageBox>
#include <QStyle>
#include <assert.h>

AboutDialog::AboutDialog(QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    setupUi(this);

    QHBoxLayout* l = new QHBoxLayout();
    l->setMargin(0);
    l->addStretch(1);
    l->addStretch(1);
    frame->setContentsMargins(0, 0, 0, 0);
    frame->setLayout(l);
    tWidget = NULL;
    installAWidget();
    
	QString title = tr("About RetroShare ");    
	
    /* get libretroshare version */
    std::string version;
    if (rsDisc->getPeerVersion(rsPeers->getOwnId(), version))
    {
		title += QString::fromStdString(version);
    }
	setWindowTitle(title);
        
#ifdef Q_OS_WIN
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#endif
}

void AboutDialog::installAWidget() {
    assert(tWidget == NULL);
    AWidget* aWidget = new AWidget();
    QVBoxLayout* l = (QVBoxLayout*)frame->layout();
    l->insertWidget(0, aWidget);
    l->setStretchFactor(aWidget, 100);
}

void AboutDialog::installTWidget() {
    assert(tWidget == NULL);
    tWidget = new TBoard();
    QLabel* npLabel = new NextPieceLabel(tWidget);
    tWidget->setNextPieceLabel(npLabel);

    QWidget* pan = new QWidget();
    QVBoxLayout* vl = new QVBoxLayout(pan);
    QLabel* topRecLabel = new QLabel(tr("Max score: %1").arg(tWidget->getMaxScore()));
    QLabel* scoreLabel = new QLabel(pan);
    QLabel* levelLabel = new QLabel(pan);
    vl->addStretch();
    vl->addWidget(topRecLabel);
    vl->addStretch();
    vl->addWidget(npLabel);
    vl->addStretch();
    vl->addWidget(scoreLabel);
    vl->addWidget(levelLabel);
    vl->addStretch();

    QHBoxLayout* l = (QHBoxLayout*)frame->layout();
    l->insertWidget(0, pan);
    l->insertWidget(0, tWidget);
    QRect cRect = frame->contentsRect();
    int height = tWidget->heightForWidth(cRect.width());
    tWidget->setFixedSize(cRect.width() * cRect.height() / height, cRect.height());
    npLabel->setFixedSize(tWidget->squareWidth()*4, tWidget->squareHeight()*5);
    l->setStretchFactor(tWidget, 100);
    connect(tWidget, SIGNAL(scoreChanged(int)), SLOT(sl_scoreChanged(int)));
    connect(tWidget, SIGNAL(levelChanged(int)), SLOT(sl_levelChanged(int)));
    connect(this, SIGNAL(si_scoreChanged(QString)), scoreLabel, SLOT(setText(QString)));
    connect(this, SIGNAL(si_levelChanged(QString)), levelLabel, SLOT(setText(QString)));
    tWidget->setFocus();
    tWidget->start();
}

void AboutDialog::switchPages() {
    QLayoutItem* li = NULL;
    QLayout* l = frame->layout();
    while ((li = l->takeAt(0)) && li->widget()) {
        li->widget()->deleteLater();
    }
    if (tWidget==NULL) {
        installTWidget();
    } else {
        tWidget = NULL;
        installAWidget();
    }
    updateTitle();
}

void AboutDialog::sl_scoreChanged(int sc) {
    emit si_scoreChanged(tr("Score: %1").arg(sc));
}

void AboutDialog::sl_levelChanged(int level) {
    emit si_levelChanged(tr("Level: %1").arg(level));
}


void AboutDialog::updateTitle() {
    if (tWidget == NULL) 
	{
		QString title = tr("About RetroShare ");    
		
		/* get libretroshare version */
		std::string version;
		if (rsDisc->getPeerVersion(rsPeers->getOwnId(), version))
		{
			title += QString::fromStdString(version);
		}
		setWindowTitle(title);
	} 
	else 
	{
        setWindowTitle(tr("Have fun ;-)"));
    }
}

void AboutDialog::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_T) {
        switchPages();
    } else if (tWidget!=NULL && (e->key() == Qt::Key_P || e->key() == Qt::Key_Pause)) {
        tWidget->pause();
    }
    QDialog::keyPressEvent(e);
}

void AboutDialog::mousePressEvent(QMouseEvent *e) {
    QPoint globalPos = mapToGlobal(e->pos());
    QPoint framePos = frame->mapFromGlobal(globalPos);
    if (frame->contentsRect().contains(framePos)) {
        switchPages();
    }
    QDialog::mousePressEvent(e);
}

void AboutDialog::on_help_button_clicked()
{
    HelpDialog helpdlg (this);
    helpdlg.exec();
}

AWidget::AWidget() {
    setMouseTracking(true);

    QImage image(":/images/about.png");
    QPainter p(&image);
    p.setPen(Qt::black);
    QFont font = p.font();
    font.setBold(true);
    font.setPointSizeF(11);
    p.setFont(font);

	QString title = tr("About RetroShare ");    
	
	/* get libretroshare version */
	std::string version;
	if (rsDisc->getPeerVersion(rsPeers->getOwnId(), version))
    {
	    QString versionq = QString::fromStdString("RetroShare version : \n") + QString::fromStdString(version);
        p.drawText(QRect(10, 10, width()-10, 60), versionq);
		
    }

    /* Draw Qt's version number */
    p.drawText(QRect(10, 50, width()-10, 60), QString("Qt %1 : \n%2").arg(tr("version"), QT_VERSION_STR));

    p.end();
	
    image1 = image2 = image;
    setFixedSize(image1.size());

    int w = image1.width();
    int h = image1.height();
    heightField1.resize(w*h);
    heightField2.resize(w*h);

    density = 5;
    page = 0;

    startTimer(15);
}


void AWidget::timerEvent(QTimerEvent* e) {
    drawWater((QRgb*)image1.bits(), (QRgb*)image2.bits());
    calcWater(page, density);
    page ^= 1;

    if (qrand()  % 128 == 0) {
        int r = 3 + qRound((double) qrand() * 4 / RAND_MAX);
        int h = 300 + qrand() * 200 / RAND_MAX;
        int x = 1 + r + qrand()%(image1.width() -2*r-1);
        int y = 1 + r + qrand()%(image1.height()-2*r-1);
        addBlob(x, y, r, h);
    }

    update();
    QObject::timerEvent(e);
}


void AWidget::paintEvent(QPaintEvent* e) {
    QWidget::paintEvent(e);
    
    QPainter p(this);
    p.drawImage(0, 0, image2);
}

void AWidget::mouseMoveEvent(QMouseEvent* e) {
    QPoint point = e->pos();
    addBlob(point.x() - 15,point.y(), 5, 400);
}


void AWidget::calcWater(int npage, int density) { 
    int w = image1.width();
    int h = image1.height();
    int count = w + 1;


    // Set up the pointers
    int *newptr;
    int *oldptr;
    if(npage == 0) {
        newptr = &heightField1.front();
        oldptr = &heightField2.front();
    } else {
        newptr = &heightField2.front();
        oldptr = &heightField1.front();
    }

    for (int y = (h-1)*w; count < y; count += 2) {
        for (int x = count+w-2; count < x; count++) {
            // This does the eight-pixel method.  It looks much better.
            int newh  = ((oldptr[count + w]
            + oldptr[count - w]
            + oldptr[count + 1]
            + oldptr[count - 1]
            + oldptr[count - w - 1]
            + oldptr[count - w + 1]
            + oldptr[count + w - 1]
            + oldptr[count + w + 1]
            ) >> 2 ) - newptr[count];

            newptr[count] =  newh - (newh >> density);
        }
    }
}

void AWidget::addBlob(int x, int y, int radius, int height) {
    int w = image1.width();
    int h = image1.height();

    // Set up the pointers
    int *newptr;
//    int *oldptr;
    if (page == 0) {
        newptr = &heightField1.front();
//        oldptr = &heightField2.front();
    } else {
        newptr = &heightField2.front();
//        oldptr = &heightField1.front();
    }

    int rquad = radius * radius;

    int left=-radius, right = radius;
    int top=-radius, bottom = radius;

    // Perform edge clipping...
    if (x - radius < 1) left -= (x-radius-1);
    if (y - radius < 1) top  -= (y-radius-1);
    if (x + radius > w-1) right -= (x+radius-w+1);
    if (y + radius > h-1) bottom-= (y+radius-h+1);

    for(int cy = top; cy < bottom; cy++) {
        int cyq = cy*cy;
        for(int cx = left; cx < right; cx++) {
            if (cx*cx + cyq < rquad) {
                newptr[w*(cy+y) + (cx+x)] += height;
            }
        }
    }
}


void AWidget::drawWater(QRgb* srcImage,QRgb* dstImage) {
    int w = image1.width();
    int h = image1.height();

    int offset = w + 1;
    int lIndex;
    int lBreak = w * h;

    int *ptr = &heightField1.front();

    for (int y = (h-1)*w; offset < y; offset += 2) {
        for (int x = offset+w-2; offset < x; offset++) {
            int dx = ptr[offset] - ptr[offset+1];
            int dy = ptr[offset] - ptr[offset+w];

            lIndex = offset + w*(dy>>3) + (dx>>3);
            if(lIndex < lBreak && lIndex > 0) {
                QRgb c = srcImage[lIndex];
                c = shiftColor(c, dx);
                dstImage[offset] = c;
            }
            offset++;
            dx = ptr[offset] - ptr[offset+1];
            dy = ptr[offset] - ptr[offset+w];

            lIndex = offset + w*(dy>>3) + (dx>>3);
            if(lIndex < lBreak && lIndex > 0) {
                QRgb c = srcImage[lIndex];
                c = shiftColor(c, dx);
                dstImage[offset] = c;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// T
TBoard::TBoard(QWidget *parent) {
    Q_UNUSED(parent);
    
    setFocusPolicy(Qt::StrongFocus);
    isStarted = false;
    isPaused = false;
    clearBoard();
    nextPiece.setRandomShape();
    score = 0;
    level = 0;

    maxScore = Settings->value("/about/maxsc").toInt();
}
TBoard::~TBoard() {
    int oldMax = Settings->value("/about/maxsc").toInt();
    int newMax = qMax(maxScore, score);
    if (oldMax < newMax) {
        Settings->setValue("/about/maxsc", newMax);
    }
}

int TBoard::heightForWidth ( int w ) const {
    return qRound(BoardHeight * float(w)/BoardWidth);
}


void TBoard::start() {
    if (isPaused) {
        return;
    }

    isStarted = true;
    isWaitingAfterLine = false;
    numLinesRemoved = 0;
    numPiecesDropped = 0;
    maxScore = qMax(score, maxScore);
    score = 0;
    level = 1;
    clearBoard();

    emit linesRemovedChanged(numLinesRemoved);
    emit scoreChanged(score);
    emit levelChanged(level);

    newPiece();
    timer.start(timeoutTime(), this);
}

void TBoard::pause() {
    if (!isStarted) {
        return;
    }

    isPaused = !isPaused;
    if (isPaused) {
        timer.stop();
    } else {
        timer.start(timeoutTime(), this);
    }
    update();
}

void TBoard::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    
    painter.setPen(Qt::black);
    painter.drawRect(frameRect());  
    QRect rect = boardRect();
    painter.fillRect(rect, Qt::white);
    
    if (isPaused) {
        painter.drawText(rect, Qt::AlignCenter, tr("Pause"));
        return;
    }

    int boardTop = rect.bottom() - BoardHeight*squareHeight();

    for (int i = 0; i < BoardHeight; ++i) {
        for (int j = 0; j < BoardWidth; ++j) {
            TPiece::Shape shape = shapeAt(j, BoardHeight - i - 1);
            if (shape != TPiece::NoShape) {
                drawSquare(painter, rect.left() + j * squareWidth(), boardTop + i * squareHeight(), shape);
            }
        }
    }
    if (curPiece.shape() != TPiece::NoShape) {
        for (int i = 0; i < 4; ++i) {
            int x = curX + curPiece.x(i);
            int y = curY - curPiece.y(i);
            drawSquare(painter, rect.left() + x * squareWidth(), boardTop + (BoardHeight - y - 1) * squareHeight(), curPiece.shape());
        }
    }
}

void TBoard::keyPressEvent(QKeyEvent *event) {
    if (!isStarted || isPaused || curPiece.shape() == TPiece::NoShape) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        tryMove(curPiece, curX - 1, curY);
        break;
    case Qt::Key_Right:
        tryMove(curPiece, curX + 1, curY);
        break;
    case Qt::Key_Down:
        tryMove(curPiece.rotatedRight(), curX, curY);
        break;
    case Qt::Key_Up:
        tryMove(curPiece.rotatedLeft(), curX, curY);
        break;
    case Qt::Key_Space:
        dropDown();
        break;
    case Qt::Key_Control:
        oneLineDown();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void TBoard::timerEvent(QTimerEvent *event) {
    if (event->timerId() == timer.timerId()) {
        if (isWaitingAfterLine) {
            isWaitingAfterLine = false;
            newPiece();
            timer.start(timeoutTime(), this);
        } else {
            oneLineDown();
        }
    } else {
        QWidget::timerEvent(event);
    }
}
void TBoard::clearBoard() {
    for (int i = 0; i < BoardHeight * BoardWidth; ++i) {
        board[i] = TPiece::NoShape;
    }
}

void TBoard::dropDown() {
    int dropHeight = 0;
    int newY = curY;
    while (newY > 0) {
        if (!tryMove(curPiece, curX, newY - 1)) {
            break;
        }
        newY--;
        dropHeight++;
    }
    pieceDropped(dropHeight);
}
void TBoard::oneLineDown() {
    if (!tryMove(curPiece, curX, curY - 1))
        pieceDropped(0);
}
void TBoard::pieceDropped(int dropHeight) {
    for (int i = 0; i < 4; ++i) {
        int x = curX + curPiece.x(i);
        int y = curY - curPiece.y(i);
        shapeAt(x, y) = curPiece.shape();
    }

    ++numPiecesDropped;
    if (numPiecesDropped % 50 == 0) {
        ++level;
        timer.start(timeoutTime(), this);
        emit levelChanged(level);
    }

    score += dropHeight + 7;
    emit scoreChanged(score);
    removeFullLines();

    if (!isWaitingAfterLine) {
        newPiece();
    }
}

void TBoard::removeFullLines() {
    int numFullLines = 0;

    for (int i = BoardHeight - 1; i >= 0; --i) {
        bool lineIsFull = true;

        for (int j = 0; j < BoardWidth; ++j) {
            if (shapeAt(j, i) == TPiece::NoShape) {
                lineIsFull = false;
                break;
            }
        }

        if (lineIsFull) {
            ++numFullLines;
            for (int k = i; k < BoardHeight - 1; ++k) {
                for (int j = 0; j < BoardWidth; ++j) {
                    shapeAt(j, k) = shapeAt(j, k + 1);
                }
            }
            for (int j = 0; j < BoardWidth; ++j) {
                shapeAt(j, BoardHeight - 1) = TPiece::NoShape;
            }
        }
    }

    if (numFullLines > 0) {
        numLinesRemoved += numFullLines;
        score += 10 * numFullLines;
        emit linesRemovedChanged(numLinesRemoved);
        emit scoreChanged(score);

        timer.start(500, this);
        isWaitingAfterLine = true;
        curPiece.setShape(TPiece::NoShape);
        update();
    }
}
void TBoard::newPiece() {
    curPiece = nextPiece;
    nextPiece.setRandomShape();
    showNextPiece();
    curX = BoardWidth / 2 + 1;
    curY = BoardHeight - 1 + curPiece.minY();

    if (!tryMove(curPiece, curX, curY)) {
        curPiece.setShape(TPiece::NoShape);
        timer.stop();
        isStarted = false;
    }
}

void TBoard::showNextPiece() {
    if (!nextPieceLabel) {
        return;
    }

    int dx = nextPiece.maxX() - nextPiece.minX() + 1;
    int dy = nextPiece.maxY() - nextPiece.minY() + 1;

    QPixmap pixmap(dx * squareWidth(), dy * squareHeight());
    QPainter painter(&pixmap);
    painter.fillRect(pixmap.rect(), nextPieceLabel->palette().background());

    for (int i = 0; i < 4; ++i) {
        int x = nextPiece.x(i) - nextPiece.minX();
        int y = nextPiece.y(i) - nextPiece.minY();
        drawSquare(painter, x * squareWidth(), y * squareHeight(), nextPiece.shape());
    }
    nextPieceLabel->setPixmap(pixmap);
}

bool TBoard::tryMove(const TPiece &newPiece, int newX, int newY) {
    for (int i = 0; i < 4; ++i) {
        int x = newX + newPiece.x(i);
        int y = newY - newPiece.y(i);
        if (x < 0 || x >= BoardWidth || y < 0 || y >= BoardHeight) {
            return false;
        }
        if (shapeAt(x, y) != TPiece::NoShape) {
            return false;
        }
    }

    curPiece = newPiece;
    curX = newX;
    curY = newY;
    update();
    return true;
}

void TBoard::drawSquare(QPainter &painter, int x, int y, TPiece::Shape shape) {
    static const QRgb colorTable[8] = { 0x000000, 0xCC6666, 0x66CC66, 0x6666CC, 0xCCCC66, 0xCC66CC, 0x66CCCC, 0xDAAA00};

    QColor color = colorTable[int(shape)];
    painter.fillRect(x + 1, y + 1, squareWidth() - 2, squareHeight() - 2, color);

    painter.setPen(color.light());
    painter.drawLine(x, y + squareHeight() - 1, x, y);
    painter.drawLine(x, y, x + squareWidth() - 1, y);

    painter.setPen(color.dark());
    painter.drawLine(x + 1, y + squareHeight() - 1, x + squareWidth() - 1, y + squareHeight() - 1);
    painter.drawLine(x + squareWidth() - 1, y + squareHeight() - 1, x + squareWidth() - 1, y + 1);
}


void TPiece::setRandomShape() {
    setShape(TPiece::Shape(qrand() % 7 + 1));
}


void TPiece::setShape(TPiece::Shape shape) {
    static const int coordsTable[8][4][2] = {
        { { 0, 0 },   { 0, 0 },   { 0, 0 },   { 0, 0 } },
        { { 0, -1 },  { 0, 0 },   { -1, 0 },  { -1, 1 } },
        { { 0, -1 },  { 0, 0 },   { 1, 0 },   { 1, 1 } },
        { { 0, -1 },  { 0, 0 },   { 0, 1 },   { 0, 2 } },
        { { -1, 0 },  { 0, 0 },   { 1, 0 },   { 0, 1 } },
        { { 0, 0 },   { 1, 0 },   { 0, 1 },   { 1, 1 } },
        { { -1, -1 }, { 0, -1 },  { 0, 0 },   { 0, 1 } },
        { { 1, -1 },  { 0, -1 },  { 0, 0 },   { 0, 1 } }
    };

    for (int i = 0; i < 4 ; i++) {
        for (int j = 0; j < 2; ++j) {
            coords[i][j] = coordsTable[shape][i][j];
        }
    }
    pieceShape = shape;
}
int TPiece::minX() const {
    int min = coords[0][0];
    for (int i = 1; i < 4; ++i) {
        min = qMin(min, coords[i][0]);
    }
    return min;
}

int TPiece::maxX() const {
    int max = coords[0][0];
    for (int i = 1; i < 4; ++i) {
        max = qMax(max, coords[i][0]);
    }
    return max;
}

int TPiece::minY() const {
    int min = coords[0][1];
    for (int i = 1; i < 4; ++i) {
        min = qMin(min, coords[i][1]);
    }
    return min;
}

int TPiece::maxY() const {
    int max = coords[0][1];
    for (int i = 1; i < 4; ++i) {
        max = qMax(max, coords[i][1]);
    }
    return max;
}

TPiece TPiece::rotatedLeft() const {
    if (pieceShape == SquareShape) {
        return *this;
    }

    TPiece result;
    result.pieceShape = pieceShape;
    for (int i = 0; i < 4; ++i) {
        result.setX(i, y(i));
        result.setY(i, -x(i));
    }
    return result;
}

TPiece TPiece::rotatedRight() const {
    if (pieceShape == SquareShape) {
        return *this;
    }
    TPiece result;
    result.pieceShape = pieceShape;
    for (int i = 0; i < 4; ++i) {
        result.setX(i, -y(i));
        result.setY(i, x(i));
    }
    return result;
}

NextPieceLabel::NextPieceLabel( QWidget* parent /* = 0*/ ) : QLabel(parent)
{
    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::white);
    setPalette(p);
    setFrameShape(QFrame::Box);
    setAlignment(Qt::AlignCenter);
    setAutoFillBackground(true);
}
