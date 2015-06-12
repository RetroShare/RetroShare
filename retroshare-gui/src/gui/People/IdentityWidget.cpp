#include "gui/People/IdentityWidget.h"
#include "ui_IdentityWidget.h"

#include "gui/common/AvatarDefs.h"
#include <gui/gxs/GxsIdDetails.h>

#include <QDrag>
#include <QMimeData>
#include <QGraphicsProxyWidget>

IdentityWidget::IdentityWidget(QString name/*=QString()*/, QWidget *parent/*=0*/) :
  FlowLayoutItem(name, parent),
  ui(new Ui::IdentityWidget)
{
	ui->setupUi(this);
	_haveGXSId = false;
	_havePGPDetail = false;

	m_myName = name;
	ui->labelName->setText(m_myName);
	ui->labelName->setToolTip(m_myName);
	QFont font = ui->labelName->font();
	font.setItalic(false);
	ui->labelName->setFont(font);

	_keyId="";
	ui->labelKeyId->setText(_keyId);
	ui->labelKeyId->setToolTip(_keyId);
	ui->labelKeyId->setVisible(false);

	ui->labelGXSId->setText(_keyId);
	ui->labelGXSId->setToolTip(_keyId);
	ui->labelGXSId->setVisible(false);

	ui->pbAdd->setVisible(false);
	QObject::connect(ui->pbAdd, SIGNAL(clicked()), this, SLOT(pbAdd_clicked()));

	_scene = new QGraphicsScene(this);
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

void IdentityWidget::updateData(const RsGxsIdGroup &gxs_group_info)
{
	//if (_group_info != gxs_group_info) {
		_group_info = gxs_group_info;
		_haveGXSId = true;

		m_myName = QString::fromUtf8(_group_info.mMeta.mGroupName.c_str());
	ui->labelName->setText(m_myName);
		if (_havePGPDetail) {
			ui->labelName->setToolTip(tr("GXS name:").append(" "+m_myName).append("\n")
			                          .append(tr("PGP name:").append(" "+_nickname)));
		} else {//if (m_myName != _nickname)
			ui->labelName->setToolTip(tr("GXS name:").append(" "+m_myName));
		}//else (m_myName != _nickname)


		_gxsId = QString::fromStdString(_group_info.mMeta.mGroupId.toStdString());
		ui->labelGXSId->setText(_gxsId);
		ui->labelGXSId->setToolTip(tr("GXS id:").append(" "+_gxsId));

		if (!_havePGPDetail) {
	QFont font = ui->labelName->font();
			font.setItalic(false);
	ui->labelName->setFont(font);

			_keyId=QString::fromStdString(_group_info.mMeta.mGroupId.toStdString());
			ui->labelKeyId->setText(_keyId);
			ui->labelKeyId->setToolTip(tr("GXS id:").append(" "+_keyId));
	ui->labelKeyId->setVisible(false);

	/// (TODO) Get real ident icon
			QImage image = GxsIdDetails::makeDefaultIcon(RsGxsId(_group_info.mMeta.mGroupId));
			if (_avatar != image) {
				_avatar = image;
				_scene->clear();
				_scene->addPixmap(QPixmap::fromImage(image.scaled(ui->graphicsView->width(),ui->graphicsView->height())));
				emit imageUpdated();
			}//if (_avatar != image)
		}//if (!_havePGPDetail)

	//}//if (_group_info != gxs_group_info)
}

void IdentityWidget::updateData(const RsPeerDetails &pgp_details)
{
	//if (_details != pgp_details) {
		_details = pgp_details;
		_havePGPDetail = true;

		_nickname = QString::fromUtf8(_details.name.c_str());
		if (!_haveGXSId) m_myName = _nickname;
		ui->labelName->setText(m_myName);
		if (_haveGXSId) {
			ui->labelName->setToolTip(tr("GXS name:").append(" "+m_myName).append("\n")
			                          .append(tr("PGP name:").append(" "+_nickname)));
		} else {//if (m_myName != _nickname)
			ui->labelName->setToolTip(tr("PGP name:").append(" "+_nickname));
		}//else (m_myName != _nickname)

		QFont font = ui->labelName->font();
		font.setItalic(true);
		ui->labelName->setFont(font);

		_keyId = QString::fromStdString(_details.gpg_id.toStdString());
		ui->labelKeyId->setText(_keyId);
		ui->labelKeyId->setToolTip(tr("PGP id:").append(" "+_keyId));

		QPixmap avatar;
		AvatarDefs::getAvatarFromGpgId(_details.gpg_id.toStdString(), avatar);
		if (_avatar != avatar.toImage()) {
			_avatar = avatar.toImage();
			_scene->clear();
			_scene->addPixmap(avatar.scaled(ui->graphicsView->width(),ui->graphicsView->height()));
			emit imageUpdated();
		}//if (_avatar != avatar.toImage())

	//}//if (_details != gpg_details)
}

void IdentityWidget::updateData(const RsGxsIdGroup &gxs_group_info, const RsPeerDetails &pgp_details)
{
	updateData(gxs_group_info);
	updateData(pgp_details);
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
	ui->labelGXSId->setVisible(value && (_haveGXSId && _havePGPDetail));
	ui->pbAdd->setVisible(value);
}
/*
bool IdentityWidget::isCurrent()
{
	return m_isCurrent;
}*/

void IdentityWidget::pbAdd_clicked()
{
	emit addButtonClicked();
}
