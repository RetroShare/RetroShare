/*
 * ChanGroupDelegate.h
 *
 *  Created on: Sep 7, 2009
 *      Author: alex
 */

#ifndef CHANGROUPDELEGATE_H_
#define CHANGROUPDELEGATE_H_

#include "common/RSItemDelegate.h"

class ChanGroupDelegate : public RSItemDelegate
{
public:
	ChanGroupDelegate(QObject *parent = 0);

	virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif /* CHANGROUPDELEGATE_H_ */
