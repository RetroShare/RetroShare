#ifndef GXSVIEWGROUP_H
#define GXSVIEWGROUP_H

#include <QWidget>

namespace Ui {
    class GxsViewGroup;
}

class GxsViewGroup : public QWidget {
    Q_OBJECT
public:
    GxsViewGroup(QWidget *parent = 0);
    ~GxsViewGroup();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::GxsViewGroup *ui;
};

#endif // GXSVIEWGROUP_H
