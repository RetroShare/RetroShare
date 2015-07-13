#include "ui_BandwidthStatsWidget.h"
#include "BWGraph.h"

class BandwidthStatsWidget: public QWidget
{
    Q_OBJECT

public:
    BandwidthStatsWidget(QWidget *parent) ;

protected slots:
    void updateFriendSelection(int n);
    void updateServiceSelection(int n);
    void updateComboBoxes() ;
    void updateUpDownSelection(int n);
    void updateUnitSelection(int n);

private:
    Ui::BwStatsWidget ui;

    QTimer *mTimer ;
};
