#include <turtle/p3turtle.h>

class MonitoredTurtleClient: public RsTurtleClientService
{
public:
    virtual void addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir) {}
    virtual void removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id) {}
    virtual void connectToTurtleRouter(p3turtle*p) { p->registerTunnelService(this) ; }

    bool handleTunnelRequest(const TurtleFileHash& hash,const RsPeerId& peer_id);
    void provideFileHash(const RsFileHash& hash);

private:
    std::map<RsFileHash,FileInfo> _local_files ;
};

