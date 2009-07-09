#include "rsifacereal.h"

RsIfaceReal::RsIfaceReal(NotifyBase &callback) :
        RsIface(callback)
{
}


        RsIfaceReal(NotifyBase &callback) :
            RsIface(callback)
        { return; }

        virtual void lockData()
        {
//		std::cerr << "RsIfaceReal::lockData()" << std::endl;
                return rsIfaceMutex.lock();
        }

        virtual void unlockData()
        {
//		std::cerr << "RsIfaceReal::unlockData()" << std::endl;
                return rsIfaceMutex.unlock();
        }

private:
        RsMutex rsIfaceMutex;
};

RsIface *createRsIface(NotifyBase &cb)
{
        return new RsIfaceReal(cb);
}
