#ifndef CIRCLEWIDGET_H
#define CIRCLEWIDGET_H

#include "gui/common/FlowLayout.h"
#include "gui/People/IdentityWidget.h"
#include <QWidget>
#include <QGraphicsScene>

#include <retroshare/rsgxscircles.h>

namespace Ui {
class CircleWidget;
}

class CircleWidget : public FlowLayoutItem
{
	Q_OBJECT

public:
	explicit CircleWidget(QString name = QString()
	                    , QWidget *parent = 0);
	~CircleWidget();
	void updateData(const RsGroupMetaData& gxs_group_info
	          , const RsGxsCircleDetails& details);

	//Start QWidget Properties
	QSize sizeHint();
	//Start FlowLayoutItem Properties
	virtual const QPixmap getImage();
	virtual const QPixmap getDragImage();
	//End Properties

	void addIdent(IdentityWidget* item);
	const QMap<RsGxsId, IdentityWidget*> idents() const {return _list;}

	const RsGroupMetaData& groupInfo() const { return _group_info;}
	const RsGxsCircleDetails& circleDetails() const {return _circle_details;}

signals:
	void askForGXSIdentityWidget(RsGxsId gxs_id);
	void askForPGPIdentityWidget(RsPgpId pgp_id);

private:
	void updateScene();

	QGraphicsScene* _scene;
	QMap<RsGxsId, IdentityWidget*> _list;
	RsGroupMetaData _group_info ;
	RsGxsCircleDetails _circle_details ;

	Ui::CircleWidget *ui;
};

#endif // CIRCLEWIDGET_H
