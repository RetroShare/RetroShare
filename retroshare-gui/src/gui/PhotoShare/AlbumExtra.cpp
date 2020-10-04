/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumExtra.cpp                            *
 *                                                                             *
 * Copyright (C) 2018 by Robert Fernie  <retroshare.project@gmail.com>         *
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

#include "AlbumExtra.h"
#include "ui_AlbumExtra.h"

AlbumExtra::AlbumExtra(QWidget *parent) :
	QWidget(NULL),
	ui(new Ui::AlbumExtra)
{
	ui->setupUi(this);
	setUp();
}

AlbumExtra::~AlbumExtra()
{
	delete ui;
}

void AlbumExtra::setUp()
{

}

void AlbumExtra::setShareMode(uint32_t mode)
{
	ui->comboBox_shareMode->setCurrentIndex(mode);
}

void AlbumExtra::setCaption(const std::string &str)
{
	ui->lineEdit_Caption->setText(QString::fromStdString(str));
}

void AlbumExtra::setPhotographer(const std::string &str)
{
	ui->lineEdit_Photographer->setText(QString::fromStdString(str));
}

void AlbumExtra::setWhere(const std::string &str)
{
	ui->lineEdit_Where->setText(QString::fromStdString(str));
}

void AlbumExtra::setWhen(const std::string &str)
{
	ui->lineEdit_When->setText(QString::fromStdString(str));
}

uint32_t AlbumExtra::getShareMode()
{
	return ui->comboBox_shareMode->currentIndex();
}

std::string AlbumExtra::getCaption()
{
	return ui->lineEdit_Caption->text().toStdString();
}

std::string AlbumExtra::getPhotographer()
{
	return ui->lineEdit_Photographer->text().toStdString();
}

std::string AlbumExtra::getWhere()
{
	return ui->lineEdit_Where->text().toStdString();
}

std::string AlbumExtra::getWhen()
{
	return ui->lineEdit_When->text().toStdString();
}

