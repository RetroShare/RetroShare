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

#include "assstyles.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include "colorutils.h"

AssStyles::AssStyles() {
	fontname = "Arial";
	fontsize = 20;
	primarycolor = 0xFFFFFF;
	backcolor = 0;
	bold = false;
	italic = false;
	halignment = 2; // Centered
	valignment = 0; // Bottom
	borderstyle = 1; // Outline
	outline = 1;
	shadow = 2;
	marginl = 20;
	marginr = 20;
	marginv = 8;
}

void AssStyles::save(QSettings * set) {
	qDebug("AssStyles::save");

	set->setValue("styles/fontname", fontname);
	set->setValue("styles/fontsize", fontsize);
	set->setValue("styles/primarycolor", primarycolor);
	set->setValue("styles/backcolor", backcolor);
	set->setValue("styles/bold", bold);
	set->setValue("styles/italic", italic);
	set->setValue("styles/halignment", halignment);
	set->setValue("styles/valignment", valignment);
	set->setValue("styles/borderstyle", borderstyle);
	set->setValue("styles/outline", outline);
	set->setValue("styles/shadow", shadow);
	set->setValue("styles/marginl", marginl);
	set->setValue("styles/marginr", marginr);
	set->setValue("styles/marginv", marginv);
}

void AssStyles::load(QSettings * set) {
	qDebug("AssStyles::load");

	fontname = set->value("styles/fontname", fontname).toString();
	fontsize = set->value("styles/fontsize", fontsize).toInt();
	primarycolor = set->value("styles/primarycolor", primarycolor).toInt();
	backcolor = set->value("styles/backcolor", backcolor).toInt();
	bold = set->value("styles/bold", bold).toBool();
	italic = set->value("styles/italic", italic).toBool();
	halignment = set->value("styles/halignment", halignment).toInt();
	valignment = set->value("styles/valignment", valignment).toInt();
	borderstyle = set->value("styles/borderstyle", borderstyle).toInt();
	outline = set->value("styles/outline", outline).toInt();
	shadow = set->value("styles/shadow", shadow).toInt();
	marginl = set->value("styles/marginl", marginl).toInt();
	marginr = set->value("styles/marginr", marginr).toInt();
	marginv = set->value("styles/marginv", marginv).toInt();
}

bool AssStyles::exportStyles(const QString & filename) {
	qDebug("AssStyles::exportStyles: filename: %s", filename.toUtf8().constData());

	QFile f(filename);
	if (f.open(QFile::WriteOnly)) {
		QTextStream out(&f);

		int alignment = halignment;
		if (valignment == 1) alignment += 3; // Middle
		else
		if (valignment == 2) alignment += 6; // Top

		out << "[Script Info]" << endl;
		out << "ScriptType: v4.00+" << endl;
		out << "Collisions: Normal" << endl;
		out << endl;
		out << "[V4+ Styles]" << endl;
		out << "Format: Name, Fontname, Fontsize, PrimaryColour, BackColour, Bold, Italic, Alignment, BorderStyle, Outline, Shadow, MarginL, MarginR, MarginV" << endl;
		out << "Style: Default,";
		out << fontname << "," ;
		out << fontsize << "," ;
		out << "&H" << ColorUtils::colorToAABBGGRR(primarycolor) << "," ;
		out << "&H" << ColorUtils::colorToAABBGGRR(backcolor) << "," ;
		out << (bold ? -1 : 0) << "," ;
		out << (italic ? -1 : 0) << "," ;
		out << alignment << "," ;
		out << borderstyle << "," ;
		out << outline << "," ;
		out << shadow << "," ;
		out << marginl << "," ;
		out << marginr << "," ;
		out << marginv;
		out << endl;

		f.close();
		return true;
	}
	return false;
}
