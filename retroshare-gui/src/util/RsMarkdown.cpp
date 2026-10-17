/*******************************************************************************
 * util/RsMarkdown.cpp                                                         *
 *                                                                             *
 * Copyright (c) 2026 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "util/RsMarkdown.h"
#include <algorithm>
#include <QPair>
#include <QVector>

#include <QRegularExpression>
#include <QStringList>
#include <QTextDocument>

#ifdef USE_CMARK
#include <cmark.h>
#endif

bool RsMarkdown::hasCMark()
{
#ifdef USE_CMARK
	return true;
#else
	return false;
#endif
}

bool RsMarkdown::isHtml(const QString& text)
{
	const QString trimmed = text.trimmed();
	if (!trimmed.startsWith(QLatin1Char('<')))
		return false;

	// "<https://...>" is a Markdown autolink, Qt doesn't know such a tag
	return Qt::mightBeRichText(trimmed);
}

bool RsMarkdown::containsMarkdown(const QString& text)
{
	if (text.trimmed().isEmpty() || isHtml(text))
		return false;

	static const QRegularExpression::PatternOptions ml = QRegularExpression::MultilineOption;
	// NOTE: keep these linear, messages may contain ~200 KB base64 image lines
	static const QRegularExpression patterns[] = {
	    QRegularExpression("!?\\[[^\\]\\n]*\\]\\([^)\\n]+\\)"),               // [link](url) ![img](src)
	    QRegularExpression("^ {0,3}#{1,6}[ \\t]+\\S", ml),                    // # heading
	    QRegularExpression("^ {0,3}>", ml),                                   // > quote
	    QRegularExpression("^ {0,3}[-*+][ \\t]+\\S", ml),                     // - bullet list
	    QRegularExpression("^ {0,3}\\d{1,9}[.)][ \\t]+\\S", ml),              // 1. ordered list
	    QRegularExpression("^ {0,3}(```|~~~)", ml),                           // ``` fenced code
	    QRegularExpression("^ {0,3}([-*_])([ \\t]*\\1){2,}[ \\t]*$", ml),    // --- rule
	    QRegularExpression("^[ \\t]*\\S[^\\n]*\\n {0,3}=+[ \\t]*$", ml),          // setext heading (linear on long lines)
	    QRegularExpression("\\*\\*[^*\\s][^*\\n]*\\*\\*"),                    // **strong**
	    QRegularExpression("__[^_\\s][^_\\n]*__"),                            // __strong__
	    QRegularExpression("(^|[^\\w*])\\*[^*\\s][^*\\n]*\\*(?![\\w*])", ml), // *em*
	    QRegularExpression("(^|[^\\w_])_[^_\\s][^_\\n]*_(?![\\w_])", ml),     // _em_
	    QRegularExpression("`[^`\\n]+`"),                                     // `code`
	    QRegularExpression("<(https?|retroshare):[^>\\s]+>",                  // <autolink>
	                       QRegularExpression::CaseInsensitiveOption),
	};

	for (const QRegularExpression& re : patterns)
		if (re.match(text).hasMatch())
			return true;

	return false;
}

/* Strip punctuation which is very likely not part of the URL */
static void splitUrlTrailer(QString& url, QString& trailer, const QString& before)
{
	static const QString trailing = QStringLiteral(".,;:!?'\"~");

	while (!url.isEmpty()) {
		const QChar c = url.at(url.size() - 1);
		bool strip = trailing.contains(c);
		if (c == QLatin1Char('*') || c == QLatin1Char('_'))
			strip = (before.count(c) % 2) == 1; // closes an *emphasis* opened before the URL
		else if (c == QLatin1Char(')'))
			strip = url.count(QLatin1Char('(')) < url.count(QLatin1Char(')'));
		else if (c == QLatin1Char(']'))
			strip = url.count(QLatin1Char('[')) < url.count(QLatin1Char(']'));

		if (!strip)
			break;

		trailer.prepend(c);
		url.chop(1);
	}
}

/* Linkify a piece of a line known not to be inside a code span */
static QString linkifySegment(const QString& segment)
{
	static const QRegularExpression urlRe("(^|\\s|\\()((?:https?|retroshare)://[^\\s<>`]+)",
	                                      QRegularExpression::CaseInsensitiveOption);

	QString result;
	int last = 0;

	QRegularExpressionMatchIterator it = urlRe.globalMatch(segment);
	while (it.hasNext()) {
		QRegularExpressionMatch match = it.next();
		const int start = match.capturedStart(2);

		// inside the label of a [link](...)? leave it alone (links can't nest)
		const QString before = segment.left(start);
		if (before.count(QLatin1Char('[')) > before.count(QLatin1Char(']')))
			continue;

		// destination of a [link](url)
		if (match.captured(1) == QLatin1String("(") && start >= 2 && segment.at(start - 2) == QLatin1Char(']'))
			continue;

		QString url = match.captured(2);
		QString trailer;
		splitUrlTrailer(url, trailer, before);
		if (url.indexOf(QLatin1String("://")) + 3 >= url.size())
			continue; // only a scheme

		result += segment.mid(last, start - last);
		result += QLatin1Char('<') + url + QLatin1Char('>') + trailer;
		last = match.capturedEnd(2);
	}

	result += segment.mid(last);
	return result;
}

/* Linkify a line, skipping `code spans` */
static QString linkifyLine(const QString& line)
{
	QString out;
	const int n = line.size();
	int segStart = 0;
	int i = 0;

	while (i < n) {
		if (line.at(i) != QLatin1Char('`')) {
			++i;
			continue;
		}

		int j = i;
		while (j < n && line.at(j) == QLatin1Char('`'))
			++j;
		const int runLength = j - i;

		// find the closing backtick run of the same length
		int close = -1;
		int k = j;
		while (k < n) {
			if (line.at(k) == QLatin1Char('`')) {
				int l = k;
				while (l < n && line.at(l) == QLatin1Char('`'))
					++l;
				if (l - k == runLength) {
					close = l;
					break;
				}
				k = l;
			} else {
				++k;
			}
		}

		if (close < 0) {
			i = j; // unmatched backticks are literal text
			continue;
		}

		out += linkifySegment(line.mid(segStart, i - segStart));
		out += line.mid(i, close - i);
		i = close;
		segStart = i;
	}

	out += linkifySegment(line.mid(segStart));
	return out;
}

QString RsMarkdown::linkifyBareUrls(const QString& markdown)
{
	static const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})");

	QStringList lines = markdown.split(QLatin1Char('\n'));
	bool inFence = false;
	QChar fenceChar;
	int fenceLength = 0;

	for (int index = 0; index < lines.size(); ++index) {
		QString& line = lines[index];

		QRegularExpressionMatch fence = fenceRe.match(line);
		if (fence.hasMatch()) {
			const QString marker = fence.captured(1);
			if (!inFence) {
				inFence = true;
				fenceChar = marker.at(0);
				fenceLength = marker.size();
				continue;
			}
			if (marker.at(0) == fenceChar && marker.size() >= fenceLength
			        && line.mid(fence.capturedEnd()).trimmed().isEmpty()) {
				inFence = false;
				continue;
			}
		}

		if (inFence)
			continue;

		// indented code block (or deeply nested list content): don't touch,
		// RsHtml::formatText() will still linkify plain URLs outside <pre>.
		if (line.startsWith(QLatin1String("    ")) || line.startsWith(QLatin1Char('\t')))
			continue;

		line = linkifyLine(line);
	}

	return lines.join(QLatin1Char('\n'));
}

QString RsMarkdown::escapeQuotes(const QString& markdown)
{
	static const QRegularExpression fenceRe("^ {0,3}(`{3,}|~{3,})");
	static const QRegularExpression quoteRe("^( {0,3})>");

	QStringList lines = markdown.split(QLatin1Char('\n'));
	bool inFence = false;
	QChar fenceChar;
	int fenceLength = 0;

	for (QString& line : lines) {
		QRegularExpressionMatch fence = fenceRe.match(line);
		if (fence.hasMatch()) {
			const QString marker = fence.captured(1);
			if (!inFence) {
				inFence = true;
				fenceChar = marker.at(0);
				fenceLength = marker.size();
			} else if (marker.at(0) == fenceChar && marker.size() >= fenceLength
			           && line.mid(fence.capturedEnd()).trimmed().isEmpty()) {
				inFence = false;
			}
			continue;
		}

		if (inFence)
			continue;

		// "\>" is a literal '>' for cmark, the following ones are plain text
		QRegularExpressionMatch quote = quoteRe.match(line);
		if (quote.hasMatch())
			line.insert(quote.capturedEnd(1), QLatin1Char('\\'));
	}

	return lines.join(QLatin1Char('\n'));
}

QString RsMarkdown::renderTaskLists(const QString& html)
{
	// "[ ]" / "[x]" at the start of a list item (GitHub task lists) become check boxes
	static const QRegularExpression taskRe(
	        "(<li[^>]*>\\s*(?:<p[^>]*>)?(?:<span[^>]*>)?)\\[([ xX])\\](?=[ \\t]|<|$)");
	static const QRegularExpression tagRe("<(/?)(ul|ol|li)\\b[^>]*>", QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression itemStartRe("^\\s*(?:<p[^>]*>)?(?:<span[^>]*>)?");

	const QString unchecked(QChar(0x2610)); // BALLOT BOX
	const QString checked(QChar(0x2611));   // BALLOT BOX WITH CHECK

	QString result;
	int last = 0;
	QRegularExpressionMatchIterator it = taskRe.globalMatch(html);
	while (it.hasNext()) {
		const QRegularExpressionMatch match = it.next();
		result += html.mid(last, int(match.capturedStart()) - last);
		result += match.captured(1) + (match.captured(2) == " " ? unchecked : checked);
		last = int(match.capturedEnd());
	}
	if (last == 0)
		return html; // no task list
	result += html.mid(last);

	// no bullets for bulleted lists made of tasks only
	struct List { int start; int length; bool bullet; bool tasksOnly; int items; };
	QVector<List> stack;
	QVector<QPair<int, int> > taskLists;

	it = tagRe.globalMatch(result);
	while (it.hasNext()) {
		const QRegularExpressionMatch tag = it.next();
		const bool closing = !tag.captured(1).isEmpty();
		const QString name = tag.captured(2).toLower();

		if (name == "li") {
			if (closing || stack.isEmpty())
				continue;
			const QRegularExpressionMatch start = itemStartRe.match(result.mid(tag.capturedEnd(), 200));
			const QString text = result.mid(tag.capturedEnd() + start.capturedLength(), 1);
			if (text != unchecked && text != checked)
				stack.last().tasksOnly = false;
			++stack.last().items;
		} else if (!closing) {
			stack.append({ int(tag.capturedStart()), int(tag.capturedLength()), name == "ul", true, 0 });
		} else if (!stack.isEmpty()) {
			const List list = stack.takeLast();
			if (list.bullet && list.tasksOnly && list.items > 0)
				taskLists.append(qMakePair(list.start, list.length));
		}
	}

	std::sort(taskLists.begin(), taskLists.end());
	for (int i = taskLists.size() - 1; i >= 0; --i)
		result.replace(taskLists[i].first, taskLists[i].second,
		               QLatin1String("<ul style=\"list-style-type:none; margin-left:0px; -qt-list-indent:1;\">"));

	return result;
}

QString RsMarkdown::toHtml(const QString& markdown, bool hardBreaks, bool classicQuotes)
{
	const QString source = linkifyBareUrls(classicQuotes ? escapeQuotes(markdown) : markdown);

#ifdef USE_CMARK
	const QByteArray utf8 = source.toUtf8();

	int options = CMARK_OPT_DEFAULT | CMARK_OPT_VALIDATE_UTF8;
	if (hardBreaks)
		options |= CMARK_OPT_HARDBREAKS;
#ifdef CMARK_OPT_SAFE
	// safe is the default since cmark 0.29, this keeps older versions safe too:
	// raw HTML and javascript:/vbscript:/file:/data: (except images) are dropped
	options |= CMARK_OPT_SAFE;
#endif

	QString body;
	char *html = cmark_markdown_to_html(utf8.constData(), static_cast<size_t>(utf8.size()), options);
	if (html) {
		body = QString::fromUtf8(html);
		// free with cmark's allocator, the library may use another C runtime (Windows DLL)
		cmark_get_default_mem_allocator()->free(html);
	}

	// compact quotes (Qt default: 40px on both sides for each level)
	body.replace(QLatin1String("<blockquote>"),
	             QString("<blockquote style=\"margin-left:%1px; margin-right:0px\">").arg(QUOTE_INDENT));

	return QLatin1String("<html><body>") + renderTaskLists(body) + QLatin1String("</body></html>");
#else
	Q_UNUSED(hardBreaks) // Qt's parser always uses soft breaks
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#if QT_CONFIG(textmarkdownreader)
	QTextDocument doc;
	doc.setMarkdown(source, QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectCommonMark
	                                                        | QTextDocument::MarkdownNoHTML));
	return renderTaskLists(doc.toHtml());
#define RS_MARKDOWN_RENDERED
#endif
#endif
#ifndef RS_MARKDOWN_RENDERED
	// no Markdown renderer available: show it as plain text
	QString escaped = markdown.toHtmlEscaped();
	escaped.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
	return QLatin1String("<html><body><p>") + escaped + QLatin1String("</p></body></html>");
#endif
#endif
}

QString RsMarkdown::renderIfMarkdown(const QString& text)
{
	return containsMarkdown(text) ? toHtml(text) : text;
}

QString RsMarkdown::toPlainText(const QString& text)
{
	if (!isHtml(text) && !containsMarkdown(text))
		return text;

	QTextDocument doc;
	doc.setHtml(isHtml(text) ? text : toHtml(text));

	QString plain = doc.toPlainText();
	plain.remove(QChar::ObjectReplacementCharacter); // images
	return plain;
}

QString RsMarkdown::fromHtml(const QString& html)
{
	if (!isHtml(html))
		return html;

	QTextDocument doc;
	doc.setHtml(html);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#if QT_CONFIG(textmarkdownwriter)
	QString markdown = doc.toMarkdown(QTextDocument::MarkdownDialectCommonMark);
	while (markdown.endsWith(QLatin1Char('\n')))
		markdown.chop(1);
	return markdown;
#define RS_MARKDOWN_WRITTEN
#endif
#endif
#ifndef RS_MARKDOWN_WRITTEN
	return doc.toPlainText();
#endif
}

QString RsMarkdown::escape(const QString& text)
{
	static const QString special = QStringLiteral("\\`*_[]<>!#|");

	QString result;
	result.reserve(text.size() + 8);
	for (const QChar& c : text) {
		if (special.contains(c))
			result += QLatin1Char('\\');
		result += c;
	}
	return result;
}

/* <url> destinations allow any character except <, > and line breaks */
static QString pointyDestination(QString url)
{
	url.replace(QLatin1Char('<'), QLatin1String("%3C"));
	url.replace(QLatin1Char('>'), QLatin1String("%3E"));
	url.remove(QLatin1Char('\r'));
	url.remove(QLatin1Char('\n'));
	return QLatin1Char('<') + url + QLatin1Char('>');
}

QString RsMarkdown::makeLink(const QString& text, const QString& url)
{
	const QString label = text.trimmed().isEmpty() ? url : text.trimmed();
	return QLatin1Char('[') + escape(label) + QLatin1String("](") + pointyDestination(url) + QLatin1Char(')');
}

QString RsMarkdown::makeImage(const QString& source, const QString& alt)
{
	return QLatin1String("![") + escape(alt) + QLatin1String("](") + pointyDestination(source) + QLatin1Char(')');
}
