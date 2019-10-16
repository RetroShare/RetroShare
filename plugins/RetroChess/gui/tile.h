#ifndef TILE_H
#define TILE_H
#include <QLabel>
#include <QDebug>

class Tile: public QLabel
{
public:

    //Fields
    int tileColor,piece,pieceColor,row,col,tileNum;
    char pieceName;

    //Constructors
    Tile(QWidget* pParent=0, Qt::WindowFlags f=0) : QLabel(pParent, f) {};
    Tile(const QString& text, QWidget* pParent = 0, Qt::WindowFlags f = 0) : QLabel(text, pParent, f){};

    //Methods
    void mousePressEvent(QMouseEvent *event);
    void display(char elem);
    void tileDisplay();
    void validate(int c);
};

void validate_tile(int row, int col, int c);

#endif // TILE_H
