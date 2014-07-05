#include "RsGxsUpdateBroadcastPage.h"
#include "RsGxsUpdateBroadcastBase.h"

RsGxsUpdateBroadcastPage::RsGxsUpdateBroadcastPage(RsGxsIfaceHelper *ifaceImpl, QWidget *parent, Qt::WindowFlags flags)
	: MainPage(parent, flags)
{
	mBase = new RsGxsUpdateBroadcastBase(ifaceImpl, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));
}

RsGxsUpdateBroadcastPage::~RsGxsUpdateBroadcastPage()
{
}

void RsGxsUpdateBroadcastPage::fillComplete()
{
	mBase->fillComplete();
}

void RsGxsUpdateBroadcastPage::setUpdateWhenInvisible(bool update)
{
	mBase->setUpdateWhenInvisible(update);
}

const std::list<RsGxsGroupId> &RsGxsUpdateBroadcastPage::getGrpIdsMeta()
{
	return mBase->getGrpIdsMeta();
}

void RsGxsUpdateBroadcastPage::getAllGrpIds(std::list<RsGxsGroupId> &grpIds)
{
	mBase->getAllGrpIds(grpIds);
}

const std::list<RsGxsGroupId> &RsGxsUpdateBroadcastPage::getGrpIds()
{
	return mBase->getGrpIds();
}

const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &RsGxsUpdateBroadcastPage::getMsgIdsMeta()
{
	return mBase->getMsgIdsMeta();
}

void RsGxsUpdateBroadcastPage::getAllMsgIds(std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgIds)
{
	mBase->getAllMsgIds(msgIds);
}

const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &RsGxsUpdateBroadcastPage::getMsgIds()
{
	return mBase->getMsgIds();
}

void RsGxsUpdateBroadcastPage::fillDisplay(bool complete)
{
	updateDisplay(complete);
	update(); // Qt flush
}

void RsGxsUpdateBroadcastPage::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	MainPage::showEvent(event);
}
