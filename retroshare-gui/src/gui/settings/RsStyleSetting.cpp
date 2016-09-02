#include "RsStyleSetting.h"

#include "rsharesettings.h"

RsStyleSetting::RsStyleSetting(QWidget *parent) : QWidget(parent)
{
	//Set Default color while StyleSheet overwrite it
	if (Settings) {
		Settings->setRsIconColor(QColor(0xFF039BD5));
		Settings->setRsIconColorOnNotify(QColor(0xFFFF990D));
		Settings->setRsIconMarginOnNotify(0);
	}
}

void RsStyleSetting::setRsIconColor(const QColor color)
{
	if (Settings)
		Settings->setRsIconColor(color);
}

void RsStyleSetting::setRsIconColorOnNotify(const QColor color)
{
	if (Settings)
		Settings->setRsIconColorOnNotify(color);
}

void RsStyleSetting::setRsIconMarginOnNotify(const uint margin)
{
	if (Settings)
		Settings->setRsIconMarginOnNotify(margin);
}
