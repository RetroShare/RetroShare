#ifndef __IM_history_item__
#define __IM_history_item__

#include <QDateTime>
#include <QString>



class IMHistoryItem 
{
public:
   IMHistoryItem();

   IMHistoryItem(const QString senderID,
                 const QString receiverID,
                 const QString text,
                 const QDateTime time);

   QDateTime time() const;
   QString   sender();
   QString   receiver();
   QString text() const;

   void setTime(QDateTime time);
   void setTime(QString time);
   void setSender(QString sender);
   void setReceiver(QString receiver);
   void setText(QString text);

   bool operator<(const IMHistoryItem& item) const;
protected:

   QDateTime vTime;
   QString   vSender;
   QString   vReceiver;
   QString   vText;

} ;

#endif

