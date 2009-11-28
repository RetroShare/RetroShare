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
#include "paths.h"
#include "mplayerversion.h"

#include <QFile>

//#define TRANS_ORIG
#define TRANS_LIST
//#define TRANS_TABLE

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
		mplayer_version = tr("Using MPlayer %1").arg(MplayerVersion::toString(pref->mplayer_detected_version)) + "<br><br>";
	}

	info->setText( 
		"<b>SMPlayer</b> &copy; 2006-2008 Ricardo Villalba &lt;rvm@escomposlinux.org&gt;<br><br>"
		"<b>" + tr("Version: %1").arg(smplayerVersion()) + "</b>" +
#if PORTABLE_APP
                " (" + tr("Portable Edition") + ")" +
#endif
        "<br>" +
        tr("Using Qt %1 (compiled with Qt %2)").arg(qVersion()).arg(QT_VERSION_STR) + "<br><br>" +
		mplayer_version +
		tr("Visit our web for updates:") +"<br>"+ 
        link("http://smplayer.berlios.de") + "<br>" + 
        link("http://smplayer.sf.net") + 
        "<br><br>" +
		tr("Get help in our forum:") +"<br>" + link("http://smplayer.berlios.de/forums") +
        "<br><br>" +
		tr("You can support SMPlayer by making a donation.") +" "+
		link("https://sourceforge.net/donate/index.php?group_id=185512", tr("More info"))
		//link("http://www.qt-apps.org/content/donate.php?content=61041", tr("More info"))
	);


	QString license_file = Paths::doc("gpl.html", pref->language);
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

	translators->setHtml( getTranslators() );

	contributions->setText(
        tr("SMPlayer logo by %1").arg("Charles Barcza &lt;kbarcza@blackpanther.hu&gt;") + "<br><br>" +
		tr("The following people have contributed with patches "
		   "(see the changelog for details):") +
		"<pre>" +
        QString(
		"corentin1234 <corentin1234@hotmail.com>\n"
		"Florin Braghis <florin@libertv.ro>\n"
		"Francesco Cosoleto <cosoleto@users.sourceforge.net>\n"
		"Glaydus <glaydus@gmail.com>\n"
		"Kamil Dziobek <turbos11@gmail.com>\n"
		"LoRd_MuldeR (http://forum.doom9.org/member.php?u=78667)\n"
		"Matthias Petri <matt@endboss.org>\n"
		"profoX <wesley@ubuntu.com>\n"
		"redxii <redxii1234@gmail.com>\n"
		"Sikon <sikon@users.sourceforge.net>\n"
		"Simon <hackykid@users.sourceforge.net>\n"
		"Stanislav Maslovski <s_i_m@users.sourceforge.net>\n"
		"Tanguy Krotoff <tkrotoff@gmail.com>\n"
		).replace("<", "&lt;").replace(">", "&gt;") + 
		"</pre>" +
		tr("If there's any omission, please report.")
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

	adjustSize();
}

About::~About() {
}

QString About::getTranslators() {
	return QString(
		 tr("The following people have contributed with translations:") + 
#ifndef TRANS_TABLE
         "<ul>" +
#else
         "<table>" +
#endif
         trad(tr("German"), "Henrikx <henrikx@users.sourceforge.net>") + 
		 trad(tr("Slovak"), "Sweto <peter.mendel@gmail.com>") +
		 trad(tr("Italian"), "Giancarlo Scola <scola.giancarlo@libero.it>") +
         trad(tr("French"), QStringList() 
			<< "Olivier g <1got@caramail.com>"
			<< "Temet <goondy@free.fr>"
			<< "Erwann MEST <kud.gray@gmail.com>") +
		 trad(tr("Simplified-Chinese"), "Tim Green <iamtimgreen@gmail.com>") +
         trad(tr("Russian"), QString::fromUtf8("Белый Владимир <wiselord1983@gmail.com>"))+ 
         trad(tr("Hungarian"), QStringList()
			<< "Charles Barcza <kbarcza@blackpanther.hu>"
			<< "CyberDragon <cyberdragon777@gmail.com>") + 
         trad(tr("Polish"), QStringList()
			<< "qla <qla0@vp.pl>"
			<< "Jarek <ajep9691@wp.pl>" ) +
         trad(tr("Japanese"), "Nardog <nardog@e2umail.com>") + 
         trad(tr("Dutch"), QStringList()
			<< "profoX <wesley@ubuntu-nl.org>"
			<< "BalaamsMiracle"
			<< "Kristof Bal <kristof.bal@gmail.com>") +
         trad(tr("Ukrainian"), QStringList()
			<< "Motsyo Gennadi <drool@altlinux.ru>"
			<< "Oleksandr Kovalenko <alx.kovalenko@gmail.com>" ) +
         trad(tr("Portuguese - Brazil"), "Ventura <ventura.barbeiro@terra.com.br>") + 
         trad(tr("Georgian"), "George Machitidze <giomac@gmail.com>") + 
         trad(tr("Czech"), QStringList()
			<< QString::fromUtf8("Martin Dvořák <martin.dvorak@centrum.cz>")
			<< QString::fromUtf8("Jaromír Smrček <jaromir.smrcek@zoner.com>") ) +
         trad(tr("Bulgarian"), "<marzeliv@mail.bg>") +
         trad(tr("Turkish"), "alper er <alperer@gmail.com>") +
         trad(tr("Swedish"), "Leif Larsson <leif.larsson@gmail.com>") +
         trad(tr("Serbian"), "Kunalagon Umuhanik <kunalagon@gmail.com>") + 
         trad(tr("Traditional Chinese"), "Hoopoe <dai715.tw@yahoo.com.tw>") + 
         trad(tr("Romanian"), "DoruH <DoruHushHush@gmail.com>") + 
         trad(tr("Portuguese - Portugal"), QStringList()
			<< "Waxman <waxman.pt@gmail.com>"
			<< QString::fromUtf8("Sérgio Marques <contatica@netcabo.pt>") ) +
		trad(tr("Greek"), "my80s <wamy80s@gmail.com>") +
		trad(tr("Finnish"), "peeaivo <peeaivo@gmail.com>") +
		trad(tr("Korean"), "Heesu Yoon <imsu30@gmail.com>") +
		trad(tr("Macedonian"), "Marko Doda <mark0d0da@gmail.com>") +
		trad(tr("Basque"), "Piarres Beobide <pi@beobide.net>") +
		trad(tr("Catalan"), QString::fromUtf8("Roger Calvó <rcalvoi@yahoo.com>")) +
		trad(tr("Slovenian"), "Janez Troha <janez.troha@gmail.com>") +
		trad(tr("Arabic"), "Muhammad Nour Hajj Omar <arabianheart@live.com>") +
		trad(tr("Kurdish"), "Si_murg56 <simurg56@gmail.com>") +
		trad(tr("Galician"), "Miguel Branco <mgl.branco@gmail.com>") +
#ifndef TRANS_TABLE
        "</ul>");
#else
        "</table>");
#endif
}

QString About::trad(const QString & lang, const QString & author) {
	return trad(lang, QStringList() << author);
}

QString About::trad(const QString & lang, const QStringList & authors) {
#ifdef TRANS_ORIG
	QString s;

	switch (authors.count()) {
		case 2: s = tr("%1 and %2"); break;
		case 3: s = tr("%1, %2 and %3"); break;
		case 4: s = tr("%1, %2, %3 and %4"); break;
		case 5: s = tr("%1, %2, %3, %4 and %5"); break;
		default: s = "%1";
	}

	for (int n = 0; n < authors.count(); n++) {
		QString author = authors[n];
		s = s.arg(author.replace("<", "&lt;").replace(">", "&gt;"));
	}

	return "<li>"+ tr("<b>%1</b>: %2").arg(lang).arg(s) + "</li>";
#endif

#ifdef TRANS_LIST
	QString s = "<ul>";;
	for (int n = 0; n < authors.count(); n++) {
		QString author = authors[n];
		s += "<li>"+ author.replace("<", "&lt;").replace(">", "&gt;") + "</li>";
	}
	s+= "</ul>";

	return "<li>"+ tr("<b>%1</b>: %2").arg(lang).arg(s) + "</li>";
#endif

#ifdef TRANS_TABLE
	QString s;
	for (int n = 0; n < authors.count(); n++) {
		QString author = authors[n];
		s += author.replace("<", "&lt;").replace(">", "&gt;");
		if (n < (authors.count()-1)) s += "<br>";
	}

	return QString("<tr><td align=right><b>%1</b></td><td>%2</td></tr>").arg(lang).arg(s);
#endif
}

QString About::link(const QString & url, QString name) {
	if (name.isEmpty()) name = url;
	return QString("<a href=\"" + url + "\">" + name +"</a>");
}

QString About::contr(const QString & author, const QString & thing) {
	return "<li>"+ tr("<b>%1</b> (%2)").arg(author).arg(thing) +"</li>";
}

QSize About::sizeHint () const {
	return QSize(518, 326);
}

#include "moc_about.cpp"
