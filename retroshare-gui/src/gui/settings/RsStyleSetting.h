#ifndef RSSTYLESETTING_H
#define RSSTYLESETTING_H

#include <QColor>
#include <QWidget>

/**
 * @brief The RsStyleSetting class is here to receive property from qss StyleSheet files.
 * You can add these lines to qss file to get it work:
 * RsStyleSetting {
 * 	qproperty-rsIconColor: #FF039BD5;
 * 	qproperty-rsIconColorOnNotify: #FFFF990D;
 * 	qproperty-rsIconMarginOnNotify: 0;
 * }

 */
class RsStyleSetting : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QColor rsIconColor WRITE setRsIconColor DESIGNABLE true)
	Q_PROPERTY(QColor rsIconColorOnNotify WRITE setRsIconColorOnNotify DESIGNABLE true)
	Q_PROPERTY(uint rsIconMarginOnNotify WRITE setRsIconMarginOnNotify DESIGNABLE true)

public:
	explicit RsStyleSetting(QWidget *parent = 0);

	void setRsIconColor(const QColor color);
	void setRsIconColorOnNotify(const QColor color);
	void setRsIconMarginOnNotify(const uint margin);

};

#endif // RSSTYLESETTING_H
