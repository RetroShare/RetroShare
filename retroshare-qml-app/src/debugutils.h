#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <QDebug>

//To switch between debugging and normal mode, un-/comment next line
#define DEBUGGING
#ifdef DEBUGGING
        #define myDebug(line) qDebug() << "| FILE:" <<  __FILE__ << " | LINE_NUMBER:"\
         << __LINE__ << " | FUNCTION:" << __FUNCTION__ << " | CONTENT:" << line
#else
    #define myDebug(line)
#endif

#endif // DEBUGUTILS_H
