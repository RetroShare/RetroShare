/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, Thomas Kister
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QTextBrowser>
#include <QtXml>

#include "HandleRichText.h"
#include "gui/RetroShareLink.h"
#include "util/ObjectPainter.h"

#include <iostream>

/**
 * The type of embedding we'd like to do
 */
enum EmbeddedType
{
	Ahref,		///< into <a></a>
	Img			///< into <img/>
};

/**
 * Base class for storing information about a given kind of embedding.
 *
 * Its only constructor is protected so it is impossible to instantiate it, and
 * at the same time derived classes have to provide a type.
 */
class EmbedInHtml
{
protected:
	EmbedInHtml(EmbeddedType newType) : myType(newType) {}

public:
	const EmbeddedType myType;
	QRegExp myRE;
};

/**
 * This class is used to store information for embedding links into <a></a> tags.
 */
class EmbedInHtmlAhref : public EmbedInHtml
{
public:
	EmbedInHtmlAhref() : EmbedInHtml(Ahref)
	{
		myRE.setPattern("(\\bretroshare://[^\\s]*)|(\\bhttps?://[^\\s]*)|(\\bfile://[^\\s]*)|(\\bwww\\.[^\\s]*)");
	}
};

/**
  * This class is used to store information for embedding smileys into <img/> tags.
  *
  * By default the QRegExp the variables are empty, which means it must be
  * filled at runtime, typically when the smileys set is loaded. It can be
  * either done by hand or by using one of the helper methods available.
  *
  * Note: The QHash uses only *one* smiley per key (unlike soon-to-be-upgraded
  * code out there).
  */
class EmbedInHtmlImg : public EmbedInHtml
{
public:
	EmbedInHtmlImg() : EmbedInHtml(Img) {}

	QHash<QString,QString> smileys;
};

/* global instance for embedding emoticons */
static EmbedInHtmlImg defEmbedImg;

RsHtml::RsHtml()
{
}

void RsHtml::initEmoticons(const QHash< QString, QString >& hash)
{
	QString newRE;
	for(QHash<QString,QString>::const_iterator it = hash.begin(); it != hash.end(); ++it)
		foreach(QString smile, it.key().split("|")) {
			if (smile.isEmpty()) {
				continue;
			}
			defEmbedImg.smileys.insert(smile, it.value());
			newRE += "(" + QRegExp::escape(smile) + ")|";
		}
	newRE.chop(1);	// remove last |
	defEmbedImg.myRE.setPattern(newRE);
}

bool RsHtml::canReplaceAnchor(QDomDocument &/*doc*/, QDomElement &/*element*/, const RetroShareLink &link)
{
	switch (link.type()) {
	case RetroShareLink::TYPE_UNKNOWN:
	case RetroShareLink::TYPE_FILE:
	case RetroShareLink::TYPE_PERSON:
	case RetroShareLink::TYPE_FORUM:
	case RetroShareLink::TYPE_CHANNEL:
	case RetroShareLink::TYPE_SEARCH:
	case RetroShareLink::TYPE_MESSAGE:
		// not yet implemented
		break;

	case RetroShareLink::TYPE_CERTIFICATE:
		return true;
	}

	return false;
}

void RsHtml::anchorTextForImg(QDomDocument &/*doc*/, QDomElement &/*element*/, const RetroShareLink &link, QString &text)
{
	text = link.niceName();
}

void RsHtml::anchorStylesheetForImg(QDomDocument &/*doc*/, QDomElement &/*element*/, const RetroShareLink &link, QString &styleSheet)
{
	switch (link.type()) {
	case RetroShareLink::TYPE_UNKNOWN:
	case RetroShareLink::TYPE_FILE:
	case RetroShareLink::TYPE_PERSON:
	case RetroShareLink::TYPE_FORUM:
	case RetroShareLink::TYPE_CHANNEL:
	case RetroShareLink::TYPE_SEARCH:
	case RetroShareLink::TYPE_MESSAGE:
		// not yet implemented
		break;

	case RetroShareLink::TYPE_CERTIFICATE:
		styleSheet = "";
		break;
	}
}

void RsHtml::replaceAnchorWithImg(QDomDocument &doc, QDomElement &element, QTextDocument *textDocument, const RetroShareLink &link)
{
	if (!textDocument) {
		return;
	}

	if (!link.valid()) {
		return;
	}

	if (element.childNodes().length() != 1) {
		return;
	}

	if (!canReplaceAnchor(doc, element, link)) {
		return;
	}

	QString imgText;
	anchorTextForImg(doc, element, link, imgText);

	QString styleSheet;
	anchorStylesheetForImg(doc, element, link, styleSheet);

	QDomNode childNode = element.firstChild();


	/* build resource name */
	QString resourceName = QString("%1_%2.png").arg(link.type()).arg(imgText);

	if (!textDocument->resource(QTextDocument::ImageResource, QUrl(resourceName)).isValid()) {
		/* draw a button on a pixmap */
		QPixmap pixmap;
		ObjectPainter::drawButton(imgText, styleSheet, pixmap);

		/* add the image to the resource cache of the text document */
		textDocument->addResource(QTextDocument::ImageResource, QUrl(resourceName), QVariant(pixmap));
	}

	element.removeChild(childNode);

	/* replace text of the anchor with <img> */
	QDomElement img = doc.createElement("img");
	img.setAttribute("src", resourceName);

	element.appendChild(img);
}

/**
 * Parses a DOM tree and replaces text by HTML tags.
 * The tree is traversed depth-first, but only through children of Element type
 * nodes. Any other kind of node is terminal.
 *
 * If the node is of type Text, its data is checked against the user-provided
 * regular expression. If there is a match, the text is cut in three parts: the
 * preceding part that will be inserted before, the part to be replaced, and the
 * following part which will be itself checked against the regular expression.
 *
 * The part to be replaced is sent to a user-provided functor that will create
 * the necessary embedding and return a new Element node to be inserted.
 *
 * @param[in] doc The whole DOM tree, necessary to create new nodes
 * @param[in,out] currentElement The current node (which is of type Element)
 * @param[in] embedInfos The regular expression and the type of embedding to use
 */
void RsHtml::embedHtml(QTextDocument *textDocument, QDomDocument& doc, QDomElement& currentElement, EmbedInHtml& embedInfos, ulong flag)
{
	if(embedInfos.myRE.pattern().length() == 0)	// we'll get stuck with an empty regexp
		return;

	QDomNodeList children = currentElement.childNodes();
	for(uint index = 0; index < children.length(); index++) {
		QDomNode node = children.item(index);
		if(node.isElement()) {
			// child is an element, we skip it if it's an <a> tag
			QDomElement element = node.toElement();
			if(element.tagName().toLower() == "head") {
				// skip it
			} else if (element.tagName().toLower() == "a") {
				// skip it
				if (embedInfos.myType == Ahref) {
					// but add title if not available
					if (element.attribute("title").isEmpty()) {
						RetroShareLink link(element.attribute("href"));
						if (link.valid()) {
							QString title = link.title();
							if (!title.isEmpty()) {
								element.setAttribute("title", title);
							}
							if (textDocument && (flag & RSHTML_FORMATTEXT_REPLACE_LINKS)) {
								replaceAnchorWithImg(doc, element, textDocument, link);
							}
						}
					} else {
						if (textDocument && (flag & RSHTML_FORMATTEXT_REPLACE_LINKS)) {
							RetroShareLink link(element.attribute("href"));
							if (link.valid()) {
								replaceAnchorWithImg(doc, element, textDocument, link);
							}
						}
					}
				}
			} else {
				embedHtml(textDocument, doc, element, embedInfos, flag);
			}
		}
		else if(node.isText()) {
			// child is a text, we parse it
			QString tempText = node.toText().data();
			if(embedInfos.myRE.indexIn(tempText) == -1)
				continue;

			// there is at least one link inside, we start replacing
			int currentPos = 0;
			int nextPos = 0;
			while((nextPos = embedInfos.myRE.indexIn(tempText, currentPos)) != -1) {
				// if nextPos == 0 it means the text begins by a link
				if(nextPos > 0) {
					QDomText textPart = doc.createTextNode(tempText.mid(currentPos, nextPos - currentPos));
					currentElement.insertBefore(textPart, node);
					index++;
				}

				// inserted tag
				QDomElement insertedTag;
				switch(embedInfos.myType) {
					case Ahref:
							{
								insertedTag = doc.createElement("a");
								insertedTag.setAttribute("href", embedInfos.myRE.cap(0));
								insertedTag.appendChild(doc.createTextNode(embedInfos.myRE.cap(0)));

								RetroShareLink link(embedInfos.myRE.cap(0));
								if (link.valid()) {
									QString title = link.title();
									if (!title.isEmpty()) {
										insertedTag.setAttribute("title", title);
									}

									if (textDocument && (flag & RSHTML_FORMATTEXT_REPLACE_LINKS)) {
										replaceAnchorWithImg(doc, insertedTag, textDocument, link);
									}
								}
							}
							break;
					case Img:
							{
								insertedTag = doc.createElement("img");
								const EmbedInHtmlImg& embedImg = static_cast<const EmbedInHtmlImg&>(embedInfos);
								insertedTag.setAttribute("src", embedImg.smileys[embedInfos.myRE.cap(0)]);
							}
							break;
				}

				currentElement.insertBefore(insertedTag, node);
				index++;

				currentPos = nextPos + embedInfos.myRE.matchedLength();
			}

			// text after the last link, only if there's one, don't touch the index
			// otherwise decrement the index because we're going to remove node
			if(currentPos < tempText.length()) {
				QDomText textPart = doc.createTextNode(tempText.mid(currentPos));
				currentElement.insertBefore(textPart, node);
			}
			else
				index--;

			currentElement.removeChild(node);
		}
	}
}

QString RsHtml::formatText(QTextDocument *textDocument, const QString &text, ulong flag)
{
	if (flag == 0 || text.isEmpty()) {
		// nothing to do
		return text;
	}

	QDomDocument doc;
	if (doc.setContent(text) == false) {
		// convert text with QTextBrowser
		QTextBrowser textBrowser;
		textBrowser.setText(text);
		doc.setContent(textBrowser.toHtml());
	}

	QDomElement body = doc.documentElement();
	if (flag & RSHTML_FORMATTEXT_EMBED_SMILEYS) {
		embedHtml(textDocument, doc, body, defEmbedImg, flag);
	}
	if (flag & RSHTML_FORMATTEXT_EMBED_LINKS) {
		EmbedInHtmlAhref defEmbedAhref;
		embedHtml(textDocument, doc, body, defEmbedAhref, flag);
	}

	QString formattedText = doc.toString(-1);  // -1 removes any annoying carriage return misinterpreted by QTextEdit

	unsigned int optimizeFlag = 0;
	if (flag & RSHTML_FORMATTEXT_REMOVE_FONT) {
		optimizeFlag |= RSHTML_OPTIMIZEHTML_REMOVE_FONT;
	}
	if (flag & RSHTML_FORMATTEXT_REMOVE_COLOR) {
		optimizeFlag |= RSHTML_OPTIMIZEHTML_REMOVE_COLOR;
	}
	if (optimizeFlag || (flag & RSHTML_FORMATTEXT_OPTIMIZE)) {
		optimizeHtml(formattedText, optimizeFlag);
	}

	return formattedText;
}

static void findElements(QDomDocument& doc, QDomElement& currentElement, const QString& nodeName, const QString& nodeAttribute, QStringList &elements)
{
	if(nodeName.isEmpty()) {
		return;
	}

	QDomNodeList children = currentElement.childNodes();
	for (uint index = 0; index < children.length(); index++) {
		QDomNode node = children.item(index);
		if (node.isElement()) {
			QDomElement element = node.toElement();
			if (QString::compare(element.tagName(), nodeName, Qt::CaseInsensitive) == 0) {
				if (nodeAttribute.isEmpty()) {
					// use text
					elements.append(element.text());
				} else {
					QString attribute = element.attribute(nodeAttribute);
					if (attribute.isEmpty() == false) {
						elements.append(attribute);
					}
				}
				continue;
			}
			findElements(doc, element, nodeName, nodeAttribute, elements);
		}
	}
}

bool RsHtml::findAnchors(const QString &text, QStringList& urls)
{
	QDomDocument doc;
	if (doc.setContent(text) == false) {
		// convert text with QTextBrowser
		QTextBrowser textBrowser;
		textBrowser.setText(text);
		doc.setContent(textBrowser.toHtml());
	}

	QDomElement body = doc.documentElement();
	findElements(doc, body, "a", "href", urls);

	return true;
}

static void removeElement(QDomElement& parentElement, QDomElement& element)
{
	QDomNodeList children = element.childNodes();
	while (children.length() > 0) {
		QDomNode childElement = element.removeChild(children.item(children.length() - 1));
		parentElement.insertAfter(childElement, element);
	}
	parentElement.removeChild(element);
}

static void optimizeHtml(QDomDocument& doc, QDomElement& currentElement, unsigned int flag)
{
	if (currentElement.tagName().toLower() == "html") {
		// change <html> to <span>
		currentElement.setTagName("span");
	}

	QDomNode styleNode;
	bool addBR = false;

	QDomNodeList children = currentElement.childNodes();
	for (uint index = 0; index < children.length(); ) {
		QDomNode node = children.item(index);

		// compress style attribute
		styleNode = node.attributes().namedItem("style");
		if (styleNode.isAttr()) {
			QDomAttr styleAttr = styleNode.toAttr();
			QString style = styleAttr.value().simplified();
			style.replace("margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px;", "margin:0px 0px 0px 0px;");
			style.replace("; ", ";");

			if (flag & (RSHTML_OPTIMIZEHTML_REMOVE_FONT | RSHTML_OPTIMIZEHTML_REMOVE_COLOR)) {
				QStringList styles = style.split(';');
				style.clear();
				foreach (QString pair, styles) {
					if (!pair.trimmed().isEmpty()) {
						QStringList keyvalue = pair.split(':');
						if (keyvalue.length() == 2) {
							QString key = keyvalue.at(0).trimmed();

							if (flag & RSHTML_OPTIMIZEHTML_REMOVE_FONT) {
								if (key == "font-family" ||
									key == "font-size" ||
									key == "font-weight" ||
									key == "font-style") {
									continue;
								}
							}
							if (flag & RSHTML_OPTIMIZEHTML_REMOVE_COLOR) {
								if (key == "color") {
									continue;
								}
							}
							style += key + ":" + keyvalue.at(1).trimmed() + ";";
						} else {
							style += pair + ";";
						}
					}
				}
			}
			if (style.isEmpty()) {
				node.attributes().removeNamedItem("style");
				styleNode.clear();
			} else {
				styleAttr.setValue(style);
			}
		}

		if (node.isElement()) {
			QDomElement element = node.toElement();

			// not <p>
			if (addBR && element.tagName().toLower() != "p") {
				// add <br> after a removed <p> but not before a <p>
				QDomElement elementBr = doc.createElement("br");
				currentElement.insertBefore(elementBr, element);
				addBR = false;
				++index;
			}

			// <body>
			if (element.tagName().toLower() == "body") {
				if (element.attributes().length() == 0) {
					// remove <body> without attributes
					removeElement(currentElement, element);
					// no ++index;
					continue;
				}
				// change <body> to <span>
				element.setTagName("span");
			}

			// <head>
			if (element.tagName().toLower() == "head") {
				// remove <head>
				currentElement.removeChild(node);
				// no ++index;
				continue;
			}

			// iterate children
			optimizeHtml(doc, element, flag);

			// <p>
			if (element.tagName().toLower() == "p") {
				// <p style="...">
				if (element.attributes().size() == 1 && styleNode.isAttr()) {
					QString style = styleNode.toAttr().value().simplified();
					if (style == "margin:0px 0px 0px 0px;-qt-block-indent:0;text-indent:0px;" ||
						style.startsWith("-qt-paragraph-type:empty;margin:0px 0px 0px 0px;-qt-block-indent:0;text-indent:0px;")) {

						if (addBR) {
							// add <br> after a removed <p> before a removed <p>
							QDomElement elementBr = doc.createElement("br");
							currentElement.insertBefore(elementBr, element);
							++index;
						}
						// remove Qt standard <p> or empty <p>
						index += element.childNodes().length();
						removeElement(currentElement, element);
						addBR = true;
						continue;
					}

					// check for blockquote (not ready)
					// style="margin-top:12px;margin-bottom:12px;margin-left:40px;margin-right:40px;-qt-block-indent:0;text-indent:0px;"
//					int count = 0; // should be 6
//					QStringList styles = style.split(';');
//					foreach (QString pair, styles) {
//						if (!pair.trimmed().isEmpty()) {
//							QStringList keyvalue = pair.split(':');
//							if (keyvalue.length() == 2) {
//								QString key = keyvalue.at(0).trimmed();
//								QString value = keyvalue.at(1).trimmed();

//								if ((key == "margin-top" || key == "margin-bottom") && value == "12px") {
//									++count;
//									continue;
//								}
//								if (key == "margin-left" || key == "margin-right") {
//									++count;
//									continue;
//								}
//								if (key == "-qt-block-indent" && value == "0") {
//									++count;
//									continue;
//								}
//								if (key == "text-indent" && value == "0px") {
//									++count;
//									continue;
//								}
//								count = 0;
//								break;
//							} else {
//								count = 0;
//								break;
//							}
//						}
//					}
//					if (count == 6) {
//						// change to "blockquote"
//						element.setTagName("blockquote");
//						element.attributes().removeNamedItem("style");
//						element.setAttribute("type", "cite");
//					}
				}
				addBR = false;
			}
		}
		++index;
	}
}

void RsHtml::optimizeHtml(QTextEdit *textEdit, QString &text, unsigned int flag)
{
	if (textEdit->toHtml() == QTextDocument(textEdit->toPlainText()).toHtml()) {
		text = textEdit->toPlainText();
		std::cerr << "Optimized text to " << text.length() << " bytes , instead of " << textEdit->toHtml().length() << std::endl;
		return;
	}

	text = textEdit->toHtml();

	optimizeHtml(text, flag);
}

void RsHtml::optimizeHtml(QString &text, unsigned int flag)
{
	int originalLength = text.length();

	// remove doctype
	text.remove(QRegExp("<!DOCTYPE[^>]*>"));

	QDomDocument doc;
	if (doc.setContent(text) == false) {
		return;
	}

	QDomElement body = doc.documentElement();
	::optimizeHtml(doc, body, flag);
	text = doc.toString(-1);

	std::cerr << "Optimized text to " << text.length() << " bytes , instead of " << originalLength << std::endl;
}

QString RsHtml::toHtml(QString text, bool realHtml)
{
	// replace "\n" from the optimized html with "<br>"
	text.replace("\n", "<br>");
	if (!realHtml) {
		return text;
	}

	QTextDocument doc;
	doc.setHtml(text);
	return doc.toHtml();
}
