/*******************************************************************************
 * gui/style/StyleDialog.cpp                                                   *
 *                                                                             *
 * Copyright (c) 2006 Crypton         <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QColorDialog>

#include "gui/settings/rsharesettings.h"

#include "StyleDialog.h"
#include "gui/style/RSStyle.h"

/** Default constructor */
StyleDialog::StyleDialog(RSStyle &style, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	ui.headerFrame->setHeaderImage(QPixmap(":/icons/collections.png"));
	ui.headerFrame->setHeaderText(tr("Define Style"));

	/* Load window postion */
	QByteArray geometry = Settings->valueFromGroup("StyleDialog", "Geometry", QByteArray()).toByteArray();
	if (geometry.isEmpty() == false) {
		restoreGeometry(geometry);
	}

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(ui.color1Button, SIGNAL(clicked()), this, SLOT(chooseColor()));
	connect(ui.color2Button, SIGNAL(clicked()), this, SLOT(chooseColor()));

	/* Initialize style combobox */
	ui.styleComboBox->addItem(tr("None"), RSStyle::STYLETYPE_NONE);
	ui.styleComboBox->addItem(tr("Solid"), RSStyle::STYLETYPE_SOLID);
	ui.styleComboBox->addItem(tr("Gradient"), RSStyle::STYLETYPE_GRADIENT);

	ui.styleComboBox->setCurrentIndex(style.styleType);
	connect(ui.styleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showButtons()));

	/* Add pushbuttons and labels */
	pushButtons.append(ui.color1Button);
	pushButtons.append(ui.color2Button);

	labels.append(ui.color1Label);
	labels.append(ui.color2Label);

	/* Set pushbuttons visible */
	showButtons();

	/* Init colors */
	for (int i = 0; i < pushButtons.size(); ++i) {
		if (i < style.colors.size()) {
			colors[pushButtons[i]] = style.colors[i];
		} else {
			colors[pushButtons[i]] = Qt::white;
		}
	}

	drawButtons();
	drawPreview();
}

/** Destructor. */
StyleDialog::~StyleDialog()
{
	/* Save window postion */
	Settings->setValueToGroup("StyleDialog", "Geometry", saveGeometry());
}

int StyleDialog::neededColors()
{
	return RSStyle::neededColors((RSStyle::StyleType) ui.styleComboBox->itemData(ui.styleComboBox->currentIndex()).toInt());
}

void StyleDialog::getStyle(RSStyle &style)
{
	style.styleType = (RSStyle::StyleType) ui.styleComboBox->itemData(ui.styleComboBox->currentIndex()).toInt();

	style.colors.clear();

	int count = qMin(neededColors(), pushButtons.size());
	for (int i = 0; i < count; ++i) {
		style.colors.append(colors[pushButtons[i]]);
	}
}

void StyleDialog::showButtons()
{
	int count = neededColors();
	for (int i = 0; i < pushButtons.size(); ++i) {
		pushButtons[i]->setVisible(i < count);
		labels[i]->setVisible(i < count);
	}

	drawPreview();
}

void StyleDialog::drawButtons()
{
	QPixmap pxm(16,14);

	QMap<QPushButton*, QColor>::iterator it;
	for (it = colors.begin(); it != colors.end(); ++it) {
		pxm.fill(it.value());
		it.key()->setIcon(pxm);
	}
}

void StyleDialog::drawPreview()
{
	RSStyle style;
	getStyle(style);

	QString widgetSheet;

	QString styleSheet = style.getStyleSheet();
	if (styleSheet.isEmpty() == false) {
		widgetSheet = QString(".QWidget{%1}").arg(styleSheet);
	}

	ui.previewWidget->setStyleSheet(widgetSheet);
}

void StyleDialog::chooseColor()
{
	QPushButton *button = dynamic_cast<QPushButton*>(sender());
	if (button == NULL) {
		return;
	}

	QColor color = QColorDialog::getColor(colors[button]);
	if (color.isValid()) {
		colors[button] = color;

		drawButtons();
		drawPreview();
	}
}
