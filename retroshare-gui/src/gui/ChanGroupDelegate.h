/*
 * ChanGroupDelegate.h
 *
 *  Created on: Sep 7, 2009
 *      Author: alex
 */

#ifndef CHANGROUPDELEGATE_H_
#define CHANGROUPDELEGATE_H_

#include <QItemDelegate>

class ChanGroupDelegate : public QItemDelegate
{
	virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif /* CHANGROUPDELEGATE_H_ */
