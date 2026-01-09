/*******************************************************************************
 * retroshare-gui/src/gui/statusbar/hashingstatus.cpp                          *
 *                                                                             *
 * Copyright (c) 2008 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QLayout>
#include <QLabel>
#include <QMovie>
#include <QToolButton>

#include "retroshare/rsfiles.h"
#include "hashingstatus.h"
#include "gui/common/ElidedLabel.h"
#include "util/qtthreadsutils.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"

//#define DEBUG_HASHING_STATUS 1

HashingStatus::HashingStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(6);

    movie = new QMovie(":/images/loader/indicator-16.gif");
    movie->setSpeed(80); // 2x speed
    hashloader = new QLabel(this);
    hashloader->setMovie(movie);
    hbox->addWidget(hashloader);

    movie->jumpToNextFrame(); // to calculate the real width
    statusHashing = new ElidedLabel(this);
    hbox->addWidget(statusHashing);

    _compactMode = false;

    setLayout(hbox);

    hashloader->hide();
    statusHashing->hide();

	mEventHandlerId=0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }) ;}, mEventHandlerId, RsEventType::SHARED_DIRECTORIES );
}

void HashingStatus::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if(event->mType != RsEventType::SHARED_DIRECTORIES)
        return;

    const RsSharedDirectoriesEvent *fe = dynamic_cast<const RsSharedDirectoriesEvent*>(event.get());

	if(!fe)
        return;

 	switch (fe->mEventCode)
    {
    default:
        break;

    case RsSharedDirectoriesEventCode::HASHING_PROCESS_RESUMED:
        statusHashing->setText(mLastText);						// fallthrough
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "HASHING RESUMED" << std::endl;
#endif

    case RsSharedDirectoriesEventCode::HASHING_PROCESS_STARTED:
    {
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "HASHING STARTED" << std::endl;
#endif
        hashloader->show() ;
        hashloader->setMovie(movie) ;
        movie->start() ;

        statusHashing->setVisible(!_compactMode) ;
    }
    break;

    case RsSharedDirectoriesEventCode::HASHING_PROCESS_PAUSED:
    {
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "HASHING PAUSED" << std::endl;
#endif
        movie->stop() ;
        hashloader->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/resume.png")) ;

        mLastText = statusHashing->text();
        statusHashing->setText(QObject::tr("[Hashing is paused]"));
        setToolTip(QObject::tr("Click to resume the hashing process"));
    }
    break;

    case RsSharedDirectoriesEventCode::HASHING_PROCESS_FINISHED:
    {
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "HASHING FINISHED" << std::endl;
#endif
        movie->stop() ;
        statusHashing->setText(QString());
        statusHashing->hide() ;
        setToolTip(QString());
        hashloader->hide() ;
    }
    break;

    case RsSharedDirectoriesEventCode::HASHING_FILE:
    {
        QString msg = QString::number((unsigned long int)fe->mHashCounter+1) + "/" + QString::number((unsigned long int)fe->mTotalFilesToHash);

        msg += " (" + misc::friendlyUnit(fe->mTotalHashedSize) + " - "
                + QString::number(int(fe->mTotalHashedSize/double(fe->mTotalSizeToHash)*100.0)) + "%"
                + ((fe->mHashingSpeed>0)?("," + QString::number((double)fe->mHashingSpeed,'f',2) + " MB/s)"):(QString()))
                + " : " + QString::fromUtf8(fe->mFilePath.c_str()) ;

        statusHashing->setText(tr("Hashing file") + " " + msg);
        setToolTip(msg + "\n"+QObject::tr("Click to pause the hashing process"));
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "HASHING FILE " << msg.toStdString() << std::endl;
#endif
    }
    break;

	case RsSharedDirectoriesEventCode::SAVING_FILE_INDEX:
    {
#ifdef DEBUG_HASHING_STATUS
        std::cerr << "HashStatusEventHandler: received event " << (void*)fe->mEventCode << ": " ;
        std::cerr << "SAVING FILE INDEX" << std::endl;
#endif
        statusHashing->setText(tr("Saving file index..."));
    }
    break;

    };
}

HashingStatus::~HashingStatus()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
    delete(movie);
}

void HashingStatus::mousePressEvent(QMouseEvent *)
{
	rsFiles->togglePauseHashingProcess() ;
}

