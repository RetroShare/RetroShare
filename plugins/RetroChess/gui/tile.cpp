#include "tile.h"
#include "validation.h"
#include "chess.h"
#include "../interface/rsRetroChess.h"

validation *valid = new validation();

/*extern int count,turn;
extern QWidget *myWidget;
extern Tile *click1;
extern Tile *tile[8][8];
*/
void validate(Tile *temp,int c);
void disOrange();


void Tile::mousePressEvent(QMouseEvent *event)
{
    validate(++((RetroChessWindow*)parentWidget())->count);
    std::string peer_id = ((RetroChessWindow*)parentWidget())->mPeerId;
    rsRetroChess->chess_click(peer_id, this->row,this->col,((RetroChessWindow*)parentWidget())->count);
}

void Tile::display(char elem)
{
    this->pieceName=elem;

    if(this->pieceColor && this->piece)
    {
        switch(elem)
        {
            case 'P': this->setPixmap(QPixmap(":/images/pawn_white.svg"));
                      break;
            case 'R': this->setPixmap(QPixmap(":/images/rook_white.svg"));
                      break;
            case 'H': this->setPixmap(QPixmap(":/images/knight_white.svg"));
                      break;
            case 'K': this->setPixmap(QPixmap(":/images/king_white.svg"));
                      break;
            case 'Q': this->setPixmap(QPixmap(":/images/queen_white.svg"));
                      break;
            case 'B': this->setPixmap(QPixmap(":/images/bishop_white.svg"));
                      break;
        }
    }

    else if(this->piece)
    {
        switch(elem)
        {
        case 'P': this->setPixmap(QPixmap(":/images/pawn_black.svg"));
                  break;
        case 'R': this->setPixmap(QPixmap(":/images/rook_black.svg"));
                  break;
        case 'H': this->setPixmap(QPixmap(":/images/knight_black.svg"));
                  break;
        case 'K': this->setPixmap(QPixmap(":/images/king_black.svg"));
                  break;
        case 'Q': this->setPixmap(QPixmap(":/images/queen_black.svg"));
                  break;
        case 'B': this->setPixmap(QPixmap(":/images/bishop_black.svg"));
                  break;
        }
    }
    else
        this->clear();
}

void Tile::validate(int c)
{
    Tile *temp = this;
    int retValue,i;

    if(c==1)
    {
        if(temp->piece && (temp->pieceColor==((RetroChessWindow*)parentWidget())->turn))
        {
            //texp[max++]=temp->tileNum;
            retValue=((RetroChessWindow*)parentWidget())->chooser(temp);

            if(retValue)
            {
                ((RetroChessWindow*)parentWidget())->click1= new Tile();
                temp->setStyleSheet("QLabel {background-color: green;}");
                ((RetroChessWindow*)parentWidget())->click1=temp;
            }
            else
            {
                //temp->setStyleSheet("QLabel {background-color: red;}");
                ((RetroChessWindow*)parentWidget())->count=0;
            }
        }
        else
        {
            //qDebug()<<"Rascel, clicking anywhere";
            ((RetroChessWindow*)parentWidget())->count=0;
        }
    }

    else
    {

        if(temp->tileNum==((RetroChessWindow*)parentWidget())->click1->tileNum)
        {
            ((RetroChessWindow*)parentWidget())->click1->tileDisplay();
            ((RetroChessWindow*)parentWidget())->disOrange();
            ((RetroChessWindow*)parentWidget())->max=0;
            ((RetroChessWindow*)parentWidget())->count=0;
        }

        for(i=0;i<((RetroChessWindow*)parentWidget())->max;i++)
        {            
            if(temp->tileNum==((RetroChessWindow*)parentWidget())->texp[i])
            {
                ((RetroChessWindow*)parentWidget())->click1->piece=0;
                temp->piece=1;

                temp->pieceColor=((RetroChessWindow*)parentWidget())->click1->pieceColor;
                temp->pieceName=((RetroChessWindow*)parentWidget())->click1->pieceName;

                ((RetroChessWindow*)parentWidget())->click1->display(((RetroChessWindow*)parentWidget())->click1->pieceName);
                temp->display(((RetroChessWindow*)parentWidget())->click1->pieceName);

                ((RetroChessWindow*)parentWidget())->click1->tileDisplay();
                temp->tileDisplay();

                retValue=((RetroChessWindow*)parentWidget())->check(((RetroChessWindow*)parentWidget())->click1);
                /*
                if(retValue)
                {
                    tile[wR][wC]->setStyleSheet("QLabel {background-color: red;}");
                }
                */

                ((RetroChessWindow*)parentWidget())->disOrange();

                ((RetroChessWindow*)parentWidget())->max=0;

                ((RetroChessWindow*)parentWidget())->turn=(((RetroChessWindow*)parentWidget())->turn+1)%2;
                ((RetroChessWindow*)parentWidget())->count=0;
            }

            else
                ((RetroChessWindow*)parentWidget())->count=1;
        }
    }
}

void Tile::tileDisplay()
{

    if(this->tileColor)
        this->setStyleSheet("QLabel {background-color: rgb(120, 120, 90);}:hover{background-color: rgb(170,85,127);}");
    else
        this->setStyleSheet("QLabel {background-color: rgb(211, 211, 158);}:hover{background-color: rgb(170,95,127);}");
}

