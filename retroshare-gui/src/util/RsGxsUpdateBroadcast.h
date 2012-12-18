#ifndef RSGXSUPDATEBROADCAST_H
#define RSGXSUPDATEBROADCAST_H

#include <QObject>

#include <gxs/rsgxsifaceimpl.h>

class RsGxsUpdateBroadcast : public QObject
{
    Q_OBJECT
public:
    explicit RsGxsUpdateBroadcast(RsGxsIfaceImpl* ifaceImpl, float dt, QObject *parent = 0);

    void startMonitor();
    void update();

signals:

    void msgsChanged(const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > & msgIds);
    void grpsChanged(const std::list<RsGxsGroupId>& grpIds);

public slots:

    void fastPoll();
    void slowPoll();

private:

    RsGxsIfaceImpl* mIfaceImpl;
    float mDt;
};

#endif // RSGXSUPDATEBROADCAST_H
