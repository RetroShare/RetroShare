#ifndef KEY_ITEM_MODEL_H
#define KEY_ITEM_MODEL_H

#include <QAbstractItemModel>
#include <retroshare/rspeers.h>

#define COLUMN_CHECK 0
#define COLUMN_PEERNAME    1
#define COLUMN_I_AUTH_PEER 2
#define COLUMN_PEER_AUTH_ME 3
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5
#define COLUMN_COUNT 6


class pgpid_item_model : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit pgpid_item_model(std::list<RsPgpId> &neighs, float &font_height, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void data_updated(std::list<RsPgpId> &new_neighs);

private:
    std::list<RsPgpId> &neighs;
    float &font_height;
};

#endif // KEY_ITEM_MODEL_H
