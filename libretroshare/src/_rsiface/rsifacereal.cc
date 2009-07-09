#include "rsifacereal.h"

RsIfaceReal::RsIfaceReal(NotifyBase &callback) :
        RsIface(callback)
{
}

void RsIfaceReal::lockData()
{
// std::cerr << "RsIfaceReal::lockData()" << std::endl;
// return rsIfaceMutex.lock();  //Return doen't make any sense since the function returns void
    rsIfaceMutex.lock();
}

void RsIfaceReal::unlockData()
{
// std::cerr << "RsIfaceReal::unlockData()" << std::endl;
// return rsIfaceMutex.unlock();  //Return doen't make any sense since the function returns void
    rsIfaceMutex.unlock();
}

