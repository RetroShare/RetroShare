#include "ui_BandwidthStatsWidget.h"
#include "BWGraph.h"

class BandwidthStatsWidget: public QWidget
{
public:
    BandwidthStatsWidget(QWidget *parent) ;

protected slots:
    void updateFriendSelection(int n);
    void updateServiceSelection(int n);

private:
    Ui::BwStatsWidget ui;
};
