!include("../Common/retroshare_plugin.pri"): error("Could not include file ../Common/retroshare_plugin.pri")

CONFIG += qt uic qrc resources

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
	QT += widgets
}

SOURCES = p3ranking.cc LinksDialog.cpp rsrankitems.cc AddLinksDialog.cpp LinksCloudPlugin.cpp
HEADERS = rsrank.h p3ranking.h LinksDialog.h rsrankitems.h AddLinksDialog.h LinksCloudPlugin.h
FORMS   = LinksDialog.ui AddLinksDialog.ui

TARGET = LinksCloud

RESOURCES = LinksCloud_images.qrc lang/LinksCloud_lang.qrc

TRANSLATIONS +=  \
			lang/LinksCloud_ca_ES.ts \
			lang/LinksCloud_cs.ts \
			lang/LinksCloud_da.ts \
			lang/LinksCloud_de.ts \
			lang/LinksCloud_el.ts \
			lang/LinksCloud_en.ts \
			lang/LinksCloud_es.ts \
			lang/LinksCloud_fi.ts \
			lang/LinksCloud_fr.ts \
			lang/LinksCloud_hu.ts \
			lang/LinksCloud_it.ts \
			lang/LinksCloud_ja_JP.ts \
			lang/LinksCloud_ko.ts \
			lang/LinksCloud_nl.ts \
			lang/LinksCloud_pl.ts \
			lang/LinksCloud_ru.ts \
			lang/LinksCloud_sv.ts \
			lang/LinksCloud_tr.ts \
			lang/LinksCloud_zh_CN.ts