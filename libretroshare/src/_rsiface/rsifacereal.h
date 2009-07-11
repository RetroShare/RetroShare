#ifndef RSIFACEREAL_H
#define RSIFACEREAL_H

/*************************** THE REAL RSIFACE (with MUTEXES) *******/

#include "util/rsthreads.h"
#include "rsiface.h"
#include "notifybase.h"

class RsIfaceReal: public RsIface
{
public:
    RsIfaceReal(NotifyBase &callback);

    virtual void lockData();
    virtual void unlockData();

private:
    RsMutex rsIfaceMutex;
};

#endif // RSIFACEREAL_H
