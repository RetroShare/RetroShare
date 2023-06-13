#ifndef CUSTOMFRAMEH_H
#define CUSTOMFRAMEH_H

#include <QFrame>
#include <QPixmap>

// This class is made to implement the background image in a Qframe or any widget

class CustomFrame : public QFrame
{
    Q_OBJECT

public:
    explicit CustomFrame(QWidget *parent = nullptr);
    void setPixmap(QPixmap pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap backgroundImage;
};

#endif
