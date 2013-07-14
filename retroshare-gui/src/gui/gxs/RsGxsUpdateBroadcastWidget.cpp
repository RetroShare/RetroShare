#include "RsGxsUpdateBroadcastWidget.h"
#include "RsGxsUpdateBroadcastBase.h"

RsGxsUpdateBroadcastWidget::RsGxsUpdateBroadcastWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
	mBase = new RsGxsUpdateBroadcastBase(ifaceImpl, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));
}

RsGxsUpdateBroadcastWidget::~RsGxsUpdateBroadcastWidget()
{
}

void RsGxsUpdateBroadcastWidget::setUpdateWhenInvisible(bool update)
{
	mBase->setUpdateWhenInvisible(update);
}

std::list<RsGxsGroupId> &RsGxsUpdateBroadcastWidget::getGrpIds()
{
	return mBase->getGrpIds();
}

std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &RsGxsUpdateBroadcastWidget::getMsgIds()
{
	return mBase->getMsgIds();
}

void RsGxsUpdateBroadcastWidget::fillDisplay(bool initialFill)
{
	updateDisplay(initialFill);
	update(); // Qt flush
}

void RsGxsUpdateBroadcastWidget::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	QWidget::showEvent(event);
}
