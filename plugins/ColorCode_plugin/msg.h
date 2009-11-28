#ifndef MSG_H
#define MSG_H

#include <QGraphicsTextItem>
#include <QFont>
#include <QTextDocument>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <iostream>

class Msg : public QGraphicsTextItem
{
public:
    Msg();
    ~Msg();

    void ShowMsg(const QString str);
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private:
    /*
    QTextDocument* mTd;
    QTextBlock mTb;
    */
    QTextLayout* mLay;
    QFont mFont;
    QRectF mUpdateRect;
};

#endif // MSG_H
