#include "RsGxsUpdateBroadcastWidget.h"
#include "RsGxsUpdateBroadcastBase.h"

RsGxsUpdateBroadcastWidget::RsGxsUpdateBroadcastWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
	mBase = new RsGxsUpdateBroadcastBase(ifaceImpl, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));

	mInterfaceHelper = ifaceImpl;
}

RsGxsUpdateBroadcastWidget::~RsGxsUpdateBroadcastWidget()
{
}

void RsGxsUpdateBroadcastWidget::fillComplete()
{
	mBase->fillComplete();
}

void RsGxsUpdateBroadcastWidget::setUpdateWhenInvisible(bool update)
{
	mBase->setUpdateWhenInvisible(update);
}

const std::list<RsGxsGroupId> &RsGxsUpdateBroadcastWidget::getGrpIds()
{
	return mBase->getGrpIds();
}

const std::list<RsGxsGroupId> &RsGxsUpdateBroadcastWidget::getGrpIdsMeta()
{
	return mBase->getGrpIdsMeta();
}

void RsGxsUpdateBroadcastWidget::getAllGrpIds(std::list<RsGxsGroupId> &grpIds)
{
	mBase->getAllGrpIds(grpIds);
}

const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &RsGxsUpdateBroadcastWidget::getMsgIds()
{
	return mBase->getMsgIds();
}

const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &RsGxsUpdateBroadcastWidget::getMsgIdsMeta()
{
	return mBase->getMsgIdsMeta();
}

void RsGxsUpdateBroadcastWidget::getAllMsgIds(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds)
{
	mBase->getAllMsgIds(msgIds);
}

void RsGxsUpdateBroadcastWidget::fillDisplay(bool complete)
{
	updateDisplay(complete);
	update(); // Qt flush
}

void RsGxsUpdateBroadcastWidget::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	QWidget::showEvent(event);
}
