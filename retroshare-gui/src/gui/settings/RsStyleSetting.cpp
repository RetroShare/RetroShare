#include "RsStyleSetting.h"

#include "rsharesettings.h"

///Exemple of line to add to qss file:
/// RsStyleSetting {
///	qproperty-rsIconColor: "FF039BD5";
///	qproperty-rsIconColorOnNotify: "FFFF990D";
///	qproperty-rsIconMarginOnNotify: 0;
///}

RsStyleSetting::RsStyleSetting(QWidget *parent) : QWidget(parent)
{
	//Set Default color while StyleSheet overwrite it
	if (Settings) {
		Settings->setRsIconColor("FF039BD5");
		Settings->setRsIconColorOnNotify("FFFF990D");
		Settings->setRsIconMarginOnNotify(0);
	}
}

void RsStyleSetting::setRsIconColor(const QString color)//QString to be Qt4 Compatible
{
	if (Settings)
		Settings->setRsIconColor(color);
}

void RsStyleSetting::setRsIconColorOnNotify(const QString color)//QString to be Qt4 Compatible
{
	if (Settings)
		Settings->setRsIconColorOnNotify(color);
}

void RsStyleSetting::setRsIconMarginOnNotify(const uint margin)
{
	if (Settings)
		Settings->setRsIconMarginOnNotify(margin);
}
