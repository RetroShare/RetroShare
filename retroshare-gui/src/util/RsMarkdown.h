/*******************************************************************************
 * util/RsMarkdown.h                                                           *
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

#ifndef RS_MARKDOWN_H
#define RS_MARKDOWN_H

#include <QString>

/**
 * @brief CommonMark (Markdown) helpers based on libcmark.
 *
 * Used by the Markdown composer (MarkdownTextEdit) and by every place that
 * displays channel posts, forum messages and board posts.
 *
 * Messages are stored as raw Markdown text. Old messages are stored as HTML
 * (written by the former RichTextEdit / MimeTextEdit composers); isHtml()
 * tells both apart so legacy content keeps rendering as before.
 *
 * Rendering uses libcmark when RetroShare is built with CONFIG+=rs_gui_cmark
 * (USE_CMARK). Without it, Qt's own Markdown parser is used (Qt >= 5.14),
 * otherwise the text is shown as plain text.
 *
 * Links and images:
 *  - [text](url), [text](<url>), <url> and bare http(s):// / retroshare://
 *    URLs all become <a href> anchors; RsHtml::formatText() then adds titles
 *    and RetroShare link handling as for HTML messages.
 *  - ![alt](data:image/...;base64,...) embedded images are kept. Any other
 *    image source is replaced by RsHtml's "image blocked" icon.
 *  - Raw HTML and javascript:/vbscript:/file: URLs are dropped (cmark safe mode).
 */
class RsMarkdown
{
public:
	/** Left indent of each quote level (px). Qt indents <blockquote> by 40px
	 *  left and right, the rendered Markdown quotes use a compact indent,
	 *  RSTextBrowser draws the quote bar inside it. */
	static constexpr int QUOTE_INDENT = 16;

	/** True when the GUI was compiled against libcmark */
	static bool hasCMark();

	/** True for legacy rich text (HTML) messages */
	static bool isHtml(const QString& text);

	/** True when text is not HTML and contains Markdown syntax */
	static bool containsMarkdown(const QString& text);

	/**
	 * Render Markdown to a complete HTML document (<html><body>...</body></html>).
	 * With hardBreaks line breaks inside paragraphs are kept, like in the old
	 * rich text composer (posts). Without, standard Markdown rules apply: a
	 * single line break is a space (wiki pages).
	 */
	static QString toHtml(const QString& markdown, bool hardBreaks = true, bool classicQuotes = false);

	/**
	 * Classic quotes: escape the "> " quote markers (outside code blocks) so
	 * quoted lines are shown as text, "> line", ">> line"..., colored by
	 * RsSyntaxHighlighter like before Markdown, instead of indented quotes.
	 */
	static QString escapeQuotes(const QString& markdown);

	/**
	 * Task lists (GitHub): "- [ ] todo" / "- [x] done" list items are shown
	 * with check boxes, without bullets. Applied by toHtml() to rendered HTML.
	 */
	static QString renderTaskLists(const QString& html);

	/**
	 * Return HTML for text that is Markdown, or text unchanged otherwise.
	 * Use it where message bodies are loaded with QTextDocument::setHtml()
	 * (summaries, thumbnails, tooltips...).
	 */
	static QString renderIfMarkdown(const QString& text);

	/** Plain text (no Markdown markup) for summaries and one-line previews */
	static QString toPlainText(const QString& text);

	/**
	 * Convert legacy HTML messages to Markdown, used when an old message is
	 * edited in the Markdown composer. Plain/Markdown text is returned as is.
	 */
	static QString fromHtml(const QString& html);

	/**
	 * Turn bare http(s):// and retroshare:// URLs into CommonMark autolinks
	 * (<url>), skipping code blocks, code spans and existing links, so that
	 * Markdown emphasis characters inside URLs can't break them.
	 */
	static QString linkifyBareUrls(const QString& markdown);

	/** Build a Markdown link. Uses <url> destination so any URL is safe */
	static QString makeLink(const QString& text, const QString& url);

	/** Build a Markdown image */
	static QString makeImage(const QString& source, const QString& alt = QString());

	/** Escape Markdown special characters in text (used for link labels) */
	static QString escape(const QString& text);
};

#endif // RS_MARKDOWN_H
