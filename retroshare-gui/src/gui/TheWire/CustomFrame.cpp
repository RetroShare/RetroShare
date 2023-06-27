#include "CustomFrame.h"
#include <QPainter>

// Constructor
CustomFrame::CustomFrame(QWidget *parent) : QFrame(parent)
{
    // Any initializations for this frame.
}

// Overriding the inbuilt paint function
void CustomFrame::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    QPainter painter(this);
    painter.drawPixmap(rect(), backgroundImage);
}

// Function to set the member variable 'backgroundImage'
void CustomFrame::setPixmap(QPixmap pixmap){
    backgroundImage = pixmap;
}
