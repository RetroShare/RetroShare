/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CARDANIMATIONLOCK_H
#define CARDANIMATIONLOCK_H

#include <QtCore/QObject>

class CardAnimationLock:public QObject
{
    Q_OBJECT
public:
    virtual ~CardAnimationLock();
    
    static CardAnimationLock & getInst();

    inline void setDemoMode(bool isDemo){m_demoStarted=isDemo;}
    inline bool isDemoRunning()const{return m_demoStarted;}

    inline bool animationsEnabled()const{return m_aniEnabled;}
    inline void enableAnimations(bool enabled){m_aniEnabled=enabled;}

    // The lock just sends a single not to allow user interaction,
    // and calls the static lockUserInteraction function of CardStack.
    // The signal is used by mainwindow to know when not to allow menu
    // items to be clicked.
    void lock();
    void unlock();

signals:
    void animationStarted();
    void animationComplete();

protected:
    CardAnimationLock();

private:
    bool         m_aniEnabled;
    bool         m_demoStarted;
};

#endif
