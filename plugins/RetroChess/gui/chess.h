#ifndef CHESS_H
#define CHESS_H

//#include "tile.h"
#include "validation.h"
#include "qwidget.h"

#include "retroshare/rspeers.h"

class RetroChessWindow : public QWidget
{
    Q_OBJECT

    void accessories();
    void chessBoard();
    RsPeerId p1id;
    RsPeerId p2id;
    std::string p1name;
    std::string p2name;
public:
    std::string mPeerId;
    explicit RetroChessWindow(std::string peerid, int player = 0, QWidget *parent = 0);
    ~RetroChessWindow();
    int currentplayer;
    int myid;

    //from global

    int wR,wC,bR,bC;
    Tile *click1;

    Tile *tile[8][8];

    int count,turn,max;
    int *texp;


    void disOrange();
    void validate_tile(int row, int col, int c);


    int flag,retVal;
    int chooser(Tile *temp);
    int validateBishop(Tile *temp);
    int validateQueen(Tile *temp);
    int validateKing(Tile *temp);
    int validateHorse(Tile *temp);
    int validateRook(Tile *temp);
    int validatePawn(Tile *temp);
    void orange();
    int check(Tile *temp);
};


QWidget* make_board();

#endif // CHESS_H
