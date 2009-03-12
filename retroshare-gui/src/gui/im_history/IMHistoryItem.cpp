#include "IMHistoryItem.h"

//============================================================================

IMHistoryItem::IMHistoryItem()
{
}

//============================================================================

IMHistoryItem::IMHistoryItem(const QString senderID,
                             const QString receiverID,
                             const QString text,
                             const QDateTime time)
{
    setTime(time);
    setReceiver(receiverID);
    setText(text);
    setSender(senderID);
}

//============================================================================

QDateTime
IMHistoryItem::time() const
{
    return vTime;
}

//============================================================================

QString
IMHistoryItem::sender()
{
    return vSender;
}

//============================================================================

QString
IMHistoryItem::receiver()
{
    return vReceiver ;
}

//============================================================================

QString
IMHistoryItem::text() const
{
    return vText;
}

//============================================================================

void
IMHistoryItem::setTime(QDateTime time)
{
    vTime = time;
}

//============================================================================

void
IMHistoryItem::setSender(QString sender)
{
    vSender = sender ;
}

//============================================================================

void
IMHistoryItem::setReceiver(QString receiver)
{
    vReceiver = receiver;
}

//============================================================================

void
IMHistoryItem::setText(QString text)
{
    vText = text ;
}

//============================================================================

//! after qSort() older messages will become first
bool
IMHistoryItem::operator<(const IMHistoryItem& item) const
{
    return (vTime< item.time()) ;
}