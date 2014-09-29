#include "gui/People/IdentityWidget.h"
#include "ui_IdentityWidget.h"

#include "gui/common/AvatarDefs.h"
#include <gui/gxs/GxsIdDetails.h>

#include <QDrag>
#include <QMimeData>
#include <QGraphicsProxyWidget>

IdentityWidget::IdentityWidget(const RsGxsIdGroup &gxs_group_info, QString name/*=QString()*/, QWidget *parent/*=0*/) :
  FlowLayoutItem(name, parent),
  _group_info(gxs_group_info),
  ui(new Ui::IdentityWidget)
{
	ui->setupUi(this);
	_isGXS=true;

	std::string desc_string = _group_info.mMeta.mGroupName ;
	QString idName = QString::fromUtf8(desc_string.c_str());
	if (name=="") m_myName = idName;
	ui->labelName->setText(m_myName);
	QFont font = ui->labelName->font();
	font.setItalic(false);
	ui->labelKeyId->setText(QString::fromStdString(_group_info.mMeta.mGroupId.toStdString()));
	ui->labelName->setFont(font);
	ui->labelKeyId->setVisible(false);

	ui->pbAdd->setVisible(false);
	_scene = new QGraphicsScene(this);
	/// (TODO) Get real ident icon
	QImage image = GxsIdDetails::makeDefaultIcon(RsGxsId(_group_info.mMeta.mGroupId));
	_scene->addPixmap(QPixmap::fromImage(image.scaled(ui->graphicsView->width(),ui->graphicsView->height())));
	//scene->setBackgroundBrush(QBrush(QColor(qrand()%255,qrand()%255,qrand()%255)));
	//scene->addText(QString("Hello, %1").arg(getName()));
	ui->graphicsView->setScene(_scene);

	//To grab events
	ui->graphicsView->setEnabled(false);

	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	setIsCurrent(false);
	setIsSelected(false);
	setAcceptDrops(true);
}

IdentityWidget::IdentityWidget(const RsPeerDetails &pgp_details, QString name/*=QString()*/, QWidget *parent/*=0*/) :
  FlowLayoutItem(name, parent),
  _details(pgp_details),
  ui(new Ui::IdentityWidget)
{
	ui->setupUi(this);
	_isGXS=false;

	QString idName = QString::fromUtf8(pgp_details.name.c_str());
	if (name=="") m_myName = idName;
	ui->labelName->setText(m_myName);
	QFont font = ui->labelName->font();
	font.setItalic(true);
	ui->labelKeyId->setText(QString::fromStdString(pgp_details.gpg_id.toStdString()));
	ui->labelName->setFont(font);
	ui->labelKeyId->setVisible(false);

	ui->pbAdd->setVisible(false);
	_scene = new QGraphicsScene(this);
	/// (TODO) Get real ident icon
	QPixmap avatar;
	AvatarDefs::getAvatarFromSslId(pgp_details.id.toStdString(), avatar);
	_scene->addPixmap(avatar.scaled(ui->graphicsView->width(),ui->graphicsView->height()));
	//scene->setBackgroundBrush(QBrush(QColor(qrand()%255,qrand()%255,qrand()%255)));
	//scene->addText(QString("Hello, %1").arg(getName()));
	ui->graphicsView->setScene(_scene);

	//To grab events
	ui->graphicsView->setEnabled(false);

	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	setIsCurrent(false);
	setIsSelected(false);
	setAcceptDrops(true);
}

IdentityWidget::~IdentityWidget()
{
	delete _scene;
	delete ui;
}

QSize IdentityWidget::sizeHint()
{
	QSize size;
	size.setHeight(ui->graphicsView->size().height() + ui->labelName->size().height());
	size.setWidth(ui->graphicsView->size().width() > ui->labelName->size().width()
	              ?ui->graphicsView->size().width() : ui->labelName->size().width());
	return size;
}

const QPixmap IdentityWidget::getImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	return ui->graphicsView->grab();
	//return this->grab(); //QT5
#else
	return QPixmap::grabWidget(ui->graphicsView);
	//return QPixmap::grabWidget(this);
#endif
}

const QPixmap IdentityWidget::getDragImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	return ui->graphicsView->grab();
	//return this->grab(); //QT5
#else
	return QPixmap::grabWidget(ui->graphicsView);
	//return QPixmap::grabWidget(this);
#endif
}

void IdentityWidget::setIsSelected(bool value)
{
	m_isSelected=value;
	QFont font=ui->labelName->font();
	font.setBold(value);
	ui->labelName->setFont(font);
}
/*
bool IdentityWidget::isSelected()
{
	return m_isSelected;
}*/

void IdentityWidget::setIsCurrent(bool value)
{
	m_isCurrent=value;
	ui->labelKeyId->setVisible(value);
	ui->pbAdd->setVisible(value);
}
/*
bool IdentityWidget::isCurrent()
{
	return m_isCurrent;
}*/
