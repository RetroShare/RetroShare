#ifndef SUBSCRIBETOOLBUTTON_H
#define SUBSCRIBETOOLBUTTON_H

#include <QToolButton>

class SubscribeToolButton : public QToolButton
{
	Q_OBJECT

public:
	explicit SubscribeToolButton(QWidget *parent = 0);

	void setSubscribed(bool subscribed);
	void addSubscribedAction(QAction *action);

signals:
	void subscribe(bool subscribe);

private slots:
	void subscribePrivate();
	void unsubscribePrivate();

private:
	void updateUi();

private:
	bool mSubscribed;
	QList<QAction*> mSubscribedActions;
};

#endif // SUBSCRIBETOOLBUTTON_H
