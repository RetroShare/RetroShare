/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "aboutdialog.h"

#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QLayout>
#include <QDialogButtonBox>

#include "images.h"
#include "version.h"

AboutDialog::AboutDialog(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f) 
{
	setWindowTitle( tr("About SMPlayer") );

	logo = new QLabel(this);
	logo->setPixmap( Images::icon("logo", 64) );

	intro = new QLabel(this);
	intro->setWordWrap(true);

	foot = new QLabel(this);
	foot->setOpenExternalLinks(true);

	credits = new QTextEdit(this);
	credits->setReadOnly(true);
	credits->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

	ok_button = new QDialogButtonBox( QDialogButtonBox::Ok );
	connect( ok_button, SIGNAL(accepted()), this, SLOT(accept()) );

	QVBoxLayout * lright = new QVBoxLayout;
	lright->addWidget( intro );
	lright->addWidget( credits );
	lright->addWidget( foot );

	QVBoxLayout * lleft = new QVBoxLayout;
	lleft->addWidget( logo );
	lleft->addStretch(1);
	
	QHBoxLayout * lmain = new QHBoxLayout;
	lmain->addLayout( lleft );
	lmain->addLayout( lright );

	QHBoxLayout * lbutton = new QHBoxLayout;
	lbutton->addStretch(1);
	lbutton->addWidget( ok_button );
	lbutton->addStretch(1);

	QVBoxLayout * lwidget = new QVBoxLayout(this);
	lwidget->addLayout( lmain );
	//lwidget->addWidget( foot );
	lwidget->addLayout( lbutton );

	intro->setText( 
		"<b>SMPlayer</b> &copy; 2006-2007 RVM &lt;rvm@escomposlinux.org&gt;<br><br>"
		"<b>" + tr("Version: %1").arg(smplayerVersion()) + "</b><br>" +
/*
#if KDE_SUPPORT
        tr("Compiled with KDE support") + "<br>" +
#endif
*/
        "<br>" +
        tr("Compiled with Qt %1").arg(QT_VERSION_STR) + "<br><br>" 
		"<i>" +
		tr("This program is free software; you can redistribute it and/or modify "
	    "it under the terms of the GNU General Public License as published by "
	    "the Free Software Foundation; either version 2 of the License, or "
  	    "(at your option) any later version.") + "</i>");

	credits->setText(
		 tr("Translators:") + 
         "<ul>" +
         trad(tr("German"), "Henrikx") + 
		 trad(tr("Slovak"), "Sweto &lt;peter.mendel@gmail.com&gt;") +
		 trad(tr("Italian"), "Giancarlo Scola &lt;giancarlo@codexcoop.it&gt;") +
         trad(tr("French"), tr("%1, %2 and %3")
			.arg("Olivier g &lt;1got@caramail.com&gt;")
			.arg("Temet &lt;goondy@free.fr&gt;")
			.arg("Kud Gray &lt;kud.gray@gmail.com&gt;") ) +
		 trad(tr("Simplified-Chinese"), "Tim Green &lt;iamtimgreen@gmail.com&gt;") +
         trad(tr("Russian"), "Yurkovsky Andrey &lt;anyr@tut.by&gt;") + 
         trad(tr("Hungarian"), "Charles Barcza &lt;kbarcza@blackpanther.hu&gt;") + 
         trad(tr("Polish"), tr("%1 and %2")
            .arg("qla &lt;qla0@vp.pl&gt;")
            .arg("Jarek &lt;ajep9691@wp.pl&gt;") ) +
         trad(tr("Japanese"), "Nardog &lt;nardog@e2umail.com&gt;") + 
         trad(tr("Dutch"), "Wesley S. &lt;wesley@ubuntu-nl.org&gt;") + 
         trad(tr("Ukrainian"), "Motsyo Gennadi &lt;drool@altlinux.ru&gt;") + 
         trad(tr("Portuguese - Brazil"), "Ventura &lt;ventura.barbeiro@terra.com.br&gt;") + 
         trad(tr("Georgian"), "George Machitidze &lt;giomac@gmail.com&gt;") + 
         trad(tr("Czech"), QString::fromUtf8("Martin Dvořák &lt;martin.dvorak@centrum.cz&gt;")) +
         trad(tr("Bulgarian"), "&lt;marzeliv@mail.bg&gt;") +
         trad(tr("Turkish"), "alper er &lt;alperer@gmail.com&gt;") +
         trad(tr("Swedish"), "Leif Larsson &lt;leif.larsson@gmail.com&gt;") +
         trad(tr("Serbian"), "Kunalagon Umuhanik &lt;kunalagon@gmail.com&gt;") + 
         trad(tr("Traditional Chinese"), "Hoopoe &lt;dai715.tw@yahoo.com.tw&gt;") + 
         trad(tr("Romanian"), "DoruH &lt;doruhushhush@hotmail.com&gt;") + 
         trad(tr("Portuguese - Portugal"), "Waxman &lt;waxman.pt@gmail.com&gt;") +
         "</ul>" +
		 tr("Logo designed by %1").arg("Charles Barcza &lt;kbarcza@blackpanther.hu&gt;") +
         "<br>"
		);

	QString url;
	#ifdef Q_OS_WIN
	url = tr("http://smplayer.sourceforge.net/en/windows/download.php",
          "If the web page is translated into your language you can "
          "change the URL so it points to the download page in the translation."
          "Otherwise leave as is.");
	#else
	url = tr("http://smplayer.sourceforge.net/en/linux/download.php",
          "If the web page is translated into your language you can "
          "change the URL so it points to the download page in the translation."
          "Otherwise leave as is.");
	#endif

	foot->setText(
		 tr("Get updates at: %1")
         .arg("<br><a href=\"" + url + "\">" + url +"</a>") );

	/*
	adjustSize();
	setFixedSize( sizeHint() );
	*/
}

AboutDialog::~AboutDialog() {
}

QString AboutDialog::trad(const QString & lang, const QString & author) {
	return "<li>"+ tr("<b>%1</b>: %2").arg(lang).arg(author) + "</li>";
}

#include "moc_aboutdialog.cpp"
