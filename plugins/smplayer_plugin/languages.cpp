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

#include "languages.h"

QMap<QString,QString> Languages::list() {
	QMap<QString,QString> l;

	l["aa"] = tr("Afar");
	l["ab"] = tr("Abkhazian");
	l["ae"] = tr("Avestan");
	l["af"] = tr("Afrikaans");
	l["ak"] = tr("Akan");
	l["am"] = tr("Amharic");
	l["an"] = tr("Aragonese");
	l["ar"] = tr("Arabic");
	l["as"] = tr("Assamese");
	l["av"] = tr("Avaric");
	l["ay"] = tr("Aymara");
	l["az"] = tr("Azerbaijani");
	l["ba"] = tr("Bashkir");
	l["be"] = tr("Belarusian");
	l["bg"] = tr("Bulgarian");
	l["bh"] = tr("Bihari");
	l["bi"] = tr("Bislama");
	l["bm"] = tr("Bambara");
	l["bn"] = tr("Bengali");
	l["bo"] = tr("Tibetan");
	l["br"] = tr("Breton");
	l["bs"] = tr("Bosnian");
	l["ca"] = tr("Catalan");
	l["ce"] = tr("Chechen");
	l["co"] = tr("Corsican");
	l["cr"] = tr("Cree");
	l["cs"] = tr("Czech");
	l["cu"] = tr("Church");
	l["cv"] = tr("Chuvash");
	l["cy"] = tr("Welsh");
	l["da"] = tr("Danish");
	l["de"] = tr("German");
	l["dv"] = tr("Divehi");
	l["dz"] = tr("Dzongkha");
	l["ee"] = tr("Ewe");
	l["el"] = tr("Greek");
	l["en"] = tr("English");
	l["eo"] = tr("Esperanto");
	l["es"] = tr("Spanish");
	l["et"] = tr("Estonian");
	l["eu"] = tr("Basque");
	l["fa"] = tr("Persian");
	l["ff"] = tr("Fulah");
	l["fi"] = tr("Finnish");
	l["fj"] = tr("Fijian");
	l["fo"] = tr("Faroese");
	l["fr"] = tr("French");
	l["fy"] = tr("Frisian");
	l["ga"] = tr("Irish");
	l["gd"] = tr("Gaelic");
	l["gl"] = tr("Galician");
	l["gn"] = tr("Guarani");
	l["gu"] = tr("Gujarati");
	l["gv"] = tr("Manx");
	l["ha"] = tr("Hausa");
	l["he"] = tr("Hebrew");
	l["hi"] = tr("Hindi");
	l["ho"] = tr("Hiri");
	l["hr"] = tr("Croatian");
	l["ht"] = tr("Haitian");
	l["hu"] = tr("Hungarian");
	l["hy"] = tr("Armenian");
	l["hz"] = tr("Herero");
	l["ch"] = tr("Chamorro");
	l["ia"] = tr("Interlingua");
	l["id"] = tr("Indonesian");
	l["ie"] = tr("Interlingue");
	l["ig"] = tr("Igbo");
	l["ii"] = tr("Sichuan");
	l["ik"] = tr("Inupiaq");
	l["io"] = tr("Ido");
	l["is"] = tr("Icelandic");
	l["it"] = tr("Italian");
	l["iu"] = tr("Inuktitut");
	l["ja"] = tr("Japanese");
	l["jv"] = tr("Javanese");
	l["ka"] = tr("Georgian");
	l["kg"] = tr("Kongo");
	l["ki"] = tr("Kikuyu");
	l["kj"] = tr("Kuanyama");
	l["kk"] = tr("Kazakh");
	l["kl"] = tr("Greenlandic");
	l["km"] = tr("Khmer");
	l["kn"] = tr("Kannada");
	l["ko"] = tr("Korean");
	l["kr"] = tr("Kanuri");
	l["ks"] = tr("Kashmiri");
	l["ku"] = tr("Kurdish");
	l["kv"] = tr("Komi");
	l["kw"] = tr("Cornish");
	l["ky"] = tr("Kirghiz");
	l["la"] = tr("Latin");
	l["lb"] = tr("Luxembourgish");
	l["lg"] = tr("Ganda");
	l["li"] = tr("Limburgan");
	l["ln"] = tr("Lingala");
	l["lo"] = tr("Lao");
	l["lt"] = tr("Lithuanian");
	l["lu"] = tr("Luba-Katanga");
	l["lv"] = tr("Latvian");
	l["mg"] = tr("Malagasy");
	l["mh"] = tr("Marshallese");
	l["mi"] = tr("Maori");
	l["mk"] = tr("Macedonian");
	l["ml"] = tr("Malayalam");
	l["mn"] = tr("Mongolian");
	l["mo"] = tr("Moldavian");
	l["mr"] = tr("Marathi");
	l["ms"] = tr("Malay");
	l["mt"] = tr("Maltese");
	l["my"] = tr("Burmese");
	l["na"] = tr("Nauru");
	l["nb"] = trUtf8("Bokmål");
	l["nd"] = tr("Ndebele");
	l["ne"] = tr("Nepali");
	l["ng"] = tr("Ndonga");
	l["nl"] = tr("Dutch");
	l["nn"] = tr("Norwegian");
	l["no"] = tr("Norwegian");
	l["nr"] = tr("Ndebele");
	l["nv"] = tr("Navajo");
	l["ny"] = tr("Chichewa");
	l["oc"] = tr("Occitan");
	l["oj"] = tr("Ojibwa");
	l["om"] = tr("Oromo");
	l["or"] = tr("Oriya");
	l["os"] = tr("Ossetian");
	l["pa"] = tr("Panjabi");
	l["pi"] = tr("Pali");
	l["pl"] = tr("Polish");
	l["ps"] = tr("Pushto");
	l["pt"] = tr("Portuguese");
	l["qu"] = tr("Quechua");
	l["rm"] = tr("Romansh");
	l["rn"] = tr("Rundi");
	l["ro"] = tr("Romanian");
	l["ru"] = tr("Russian");
	l["rw"] = tr("Kinyarwanda");
	l["sa"] = tr("Sanskrit");
	l["sc"] = tr("Sardinian");
	l["sd"] = tr("Sindhi");
	l["se"] = tr("Sami");
	l["sg"] = tr("Sango");
	l["si"] = tr("Sinhala");
	l["sk"] = tr("Slovak");
	l["sl"] = tr("Slovenian");
	l["sm"] = tr("Samoan");
	l["sn"] = tr("Shona");
	l["so"] = tr("Somali");
	l["sq"] = tr("Albanian");
	l["sr"] = tr("Serbian");
	l["ss"] = tr("Swati");
	l["st"] = tr("Sotho");
	l["su"] = tr("Sundanese");
	l["sv"] = tr("Swedish");
	l["sw"] = tr("Swahili");
	l["ta"] = tr("Tamil");
	l["te"] = tr("Telugu");
	l["tg"] = tr("Tajik");
	l["th"] = tr("Thai");
	l["ti"] = tr("Tigrinya");
	l["tk"] = tr("Turkmen");
	l["tl"] = tr("Tagalog");
	l["tn"] = tr("Tswana");
	l["to"] = tr("Tonga");
	l["tr"] = tr("Turkish");
	l["ts"] = tr("Tsonga");
	l["tt"] = tr("Tatar");
	l["tw"] = tr("Twi");
	l["ty"] = tr("Tahitian");
	l["ug"] = tr("Uighur");
	l["uk"] = tr("Ukrainian");
	l["ur"] = tr("Urdu");
	l["uz"] = tr("Uzbek");
	l["ve"] = tr("Venda");
	l["vi"] = tr("Vietnamese");
	l["vo"] = trUtf8("Volapük");
	l["wa"] = tr("Walloon");
	l["wo"] = tr("Wolof");
	l["xh"] = tr("Xhosa");
	l["yi"] = tr("Yiddish");
	l["yo"] = tr("Yoruba");
	l["za"] = tr("Zhuang");
	l["zh"] = tr("Chinese");
	l["zu"] = tr("Zulu");

	return l;
}

QMap<QString,QString> Languages::translations() {
	QMap <QString,QString> m;
	m["ar_SY"] = tr("Arabic");
	m["bg"] = tr("Bulgarian");
	m["ca"] = tr("Catalan");
	m["cs"] = tr("Czech");
	m["de"] = tr("German");
	m["el_GR"] = tr("Greek");
	m["en_US"] = tr("English");
	m["es"] = tr("Spanish");
	m["eu"] = tr("Basque");
	m["fi"] = tr("Finnish");
	m["fr"] = tr("French");
	m["gl"] = tr("Galician");
	m["hu"] = tr("Hungarian");
	m["it"] = tr("Italian");
	m["ja"] = tr("Japanese");
	m["ka"] = tr("Georgian");
	m["ko"] = tr("Korean");
	m["ku"] = tr("Kurdish");
	m["mk"] = tr("Macedonian");
	m["nl"] = tr("Dutch");
	m["pl"] = tr("Polish");
	m["pt_BR"] = tr("Portuguese - Brazil");
	m["pt_PT"] = tr("Portuguese - Portugal");
	m["ro_RO"] = tr("Romanian");
	m["ru_RU"] = tr("Russian");
	m["sk"] = tr("Slovak");
	m["sl_SI"] = tr("Slovenian");
	m["sr"] = tr("Serbian");
	m["sv"] = tr("Swedish");
	m["tr"] = tr("Turkish");
	m["uk_UA"] = tr("Ukrainian");
	m["zh_CN"] = tr("Simplified-Chinese");
	m["zh_TW"] = tr("Traditional Chinese");

	return m;
}

QMap<QString,QString> Languages::encodings() {
	QMap<QString,QString> l;

	l["Unicode"] = tr("Unicode");
	l["UTF-8"] = tr("UTF-8");
	l["ISO-8859-1"] = tr("Western European Languages");
	l["ISO-8859-15"] = tr("Western European Languages with Euro");
	l["ISO-8859-2"] = tr("Slavic/Central European Languages");
	l["ISO-8859-3"] = tr("Esperanto, Galician, Maltese, Turkish");
	l["ISO-8859-4"] = tr("Old Baltic charset");
	l["ISO-8859-5"] = tr("Cyrillic");
	l["ISO-8859-6"] = tr("Arabic");
	l["ISO-8859-7"] = tr("Modern Greek");
	l["ISO-8859-9"] = tr( "Turkish");
	l["ISO-8859-13"] = tr( "Baltic");
	l["ISO-8859-14"] = tr( "Celtic");
	l["ISO-8859-8"] = tr( "Hebrew charsets");
	l["KOI8-R"] = tr( "Russian");
	l["KOI8-U/RU"] = tr( "Ukrainian, Belarusian");
	l["CP936"] = tr( "Simplified Chinese charset");
	l["BIG5"] = tr( "Traditional Chinese charset");
	l["SHIFT-JIS"] = tr( "Japanese charsets");
	l["CP949"] = tr( "Korean charset");
	l["CP874"] = tr( "Thai charset");
	l["CP1251"] = tr( "Cyrillic Windows");
	l["CP1250"] = tr( "Slavic/Central European Windows");
	l["CP1256"] = tr( "Arabic Windows");

	return l;
}

#include "moc_languages.cpp"
