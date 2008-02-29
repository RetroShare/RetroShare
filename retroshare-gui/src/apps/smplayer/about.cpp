/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

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

#include "about.h"
#include "images.h"
#include "version.h"
#include "global.h"
#include "preferences.h"
#include "helper.h"

#include <QFile>

using namespace Global;

About::About(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f) 
{
	setupUi(this);
	setWindowIcon( Images::icon("logo", 64) );

	logo->setPixmap( Images::icon("logo", 64) );
	contrib_icon->setPixmap( Images::icon("contributors" ) );
	translators_icon->setPixmap( Images::icon("translators" ) );
	license_icon->setPixmap( Images::icon("license" ) );

	QString mplayer_version;
	if (pref->mplayer_detected_version > 0) {
		mplayer_version = tr("Using MPlayer SVN r%1").arg(pref->mplayer_detected_version) + "<br><br>";
	}

	info->setText( 
		"<b>SMPlayer</b> &copy; 2006-2008 Ricardo Villalba &lt;rvm@escomposlinux.org&gt;<br><br>"
		"<b>" + tr("Version: %1").arg(smplayerVersion()) + "</b>" +
        "<br>" +
        tr("Compiled with Qt %1").arg(QT_VERSION_STR) + "<br><br>" +
		mplayer_version +
		tr("Visit our web for updates:") +"<br>"+ 
        link("http://smplayer.berlios.de") + "<br>" + 
        link("http://smplayer.sf.net") + 
        "<br><br>" +
		tr("Get help in our forum:") +"<br>" + link("http://smplayer.berlios.de/forums") +
        "<br><br>" +
		tr("You can support SMPlayer by making a donation.") +" "+
		link("https://sourceforge.net/donate/index.php?group_id=185512", tr("More info"))
	);


	QString license_file = Helper::doc("gpl.html", pref->language);
	if (QFile::exists(license_file)) {
		QFont fixed_font;
		fixed_font.setStyleHint(QFont::TypeWriter);
		fixed_font.setFamily("Courier");
		license->setFont(fixed_font);

		QFile f(license_file);
		if (f.open(QIODevice::ReadOnly)) {
			license->setText(QString::fromUtf8(f.readAll().constData()));
		}
		f.close();
	} else {
		license->setText(
		"<i>" +
		tr("This program is free software; you can redistribute it and/or modify "
	    "it under the terms of the GNU General Public License as published by "
	    "the Free Software Foundation; either version 2 of the License, or "
  	    "(at your option) any later version.") + "</i>");
	}

	translators->setText( getTranslators() );

	contributions->setText(
		tr("Some people have helped with their contributions:") +
		"<ul>" +
		contr("Charles Barcza &lt;kbarcza@blackpanther.hu&gt;", tr("SMPlayer logo")) +
		contr("Florin Braghis &lt;florin@libertv.ro&gt;", tr("file associations for Windows")) +
		contr("Matvey Kozhev &lt;sikon@lucidfox.org&gt;", tr("manpage for linux")) +
		contr("profoX", tr("support for colored console output")) + 
		contr("redxii", tr("icon for file associations")) +
		contr("LoRd_MuldeR", tr("patch for file associations")) +
		"</ul>"
	);

	// Copy the background color ("window") of the tab widget to the "base" color of the qtextbrowsers
	// Problem, it doesn't work with some styles, so first we change the "window" color of the tab widgets.
	info_tab->setAutoFillBackground(true);
	contributions_tab->setAutoFillBackground(true);
	translations_tab->setAutoFillBackground(true);
	license_tab->setAutoFillBackground(true);
	
	QPalette pal = info_tab->palette();
	pal.setColor(QPalette::Window, palette().color(QPalette::Window) );
	
	info_tab->setPalette(pal);
	contributions_tab->setPalette(pal);
	translations_tab->setPalette(pal);
	license_tab->setPalette(pal);
	
	QPalette p = info->palette();
	//p.setBrush(QPalette::Base, info_tab->palette().window());
	p.setColor(QPalette::Base, info_tab->palette().color(QPalette::Window));

	info->setPalette(p);
	contributions->setPalette(p);
	translators->setPalette(p);
	//license->setPalette(p);
}

About::~About() {
}

QString About::getTranslators() {
	return QString(
		 tr("The following people have contributed with translations:") + 
         "<ul>" +
         trad(tr("German"), "Henrikx") + 
		 trad(tr("Slovak"), "Sweto &lt;peter.mendel@gmail.com&gt;") +
		 trad(tr("Italian"), "Giancarlo Scola &lt;giancarlo@codexcoop.it&gt;") +
         trad(tr("French"), tr("%1, %2 and %3")
			.arg("Olivier g &lt;1got@caramail.com&gt;")
			.arg("Temet &lt;goondy@free.fr&gt;")
			.arg("Kud Gray &lt;kud.gray@gmail.com&gt;") ) +
		 trad(tr("Simplified-Chinese"), "Tim Green &lt;iamtimgreen@gmail.com&gt;") +
         trad(tr("Russian"), QString::fromUtf8("Белый Владимир &lt;wiselord1983@gmail.com&gt;"))+ 
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
		 trad(tr("Greek"), "my80s &lt;wamy80s@gmail.com&gt;") +
		 trad(tr("Finnish"), "peeaivo &lt;peeaivo@gmail.com&gt;") +
	trad(tr("Korean"), "Heesu Yoon &lt;imsu30@gmail.com&gt;") +
         "</ul>");
}

QString About::trad(const QString & lang, const QString & author) {
	return "<li>"+ tr("<b>%1</b>: %2").arg(lang).arg(author) + "</li>";
}

QString About::link(const QString & url, QString name) {
	if (name.isEmpty()) name = url;
	return QString("<a href=\"" + url + "\">" + name +"</a>");
}

QString About::contr(const QString & author, const QString & thing) {
	return "<li>"+ tr("<b>%1</b> (%2)").arg(author).arg(thing) +"</li>";
}

#include "moc_about.cpp"
