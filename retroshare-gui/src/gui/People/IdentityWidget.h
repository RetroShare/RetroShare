#ifndef IDENTITYWIDGET_H
#define IDENTITYWIDGET_H

#include "gui/common/FlowLayout.h"
#include <QWidget>
#include <QGraphicsScene>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

namespace Ui {
class IdentityWidget;
}

class IdentityWidget : public FlowLayoutItem
{
	Q_OBJECT

public:
	explicit IdentityWidget(const RsGxsIdGroup& gxs_group_info, QString name = QString(), QWidget *parent = 0);
	explicit IdentityWidget(const RsPeerDetails& gpg_details, QString name = QString(), QWidget *parent = 0);
	~IdentityWidget();

	//Start QWidget Properties
	QSize sizeHint();
	//Start FlowLayoutItem Properties
	virtual const QPixmap getImage();
	virtual const QPixmap getDragImage();
	virtual void setIsSelected(bool value);
	//virtual bool isSelected();
	virtual void setIsCurrent(bool value);
	//virtual bool isCurrent();

	//End Properties
	const bool isGXS() { return _isGXS; }
	const RsGxsIdGroup& groupInfo() const { return _group_info; }
	const RsPeerDetails& details() const { return _details; }

private:
	bool _isGXS;
	RsGxsIdGroup _group_info;
	RsPeerDetails _details;
	QGraphicsScene* _scene;

	Ui::IdentityWidget *ui;
};

#endif // IDENTITYWIDGET_H
