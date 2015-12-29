#ifndef RSSYNTAXHIGHLIGHTER_H
#define RSSYNTAXHIGHLIGHTER_H

#include <QObject>
#include <QSyntaxHighlighter>
#include <QTextEdit>

class RsSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)

public:
	RsSyntaxHighlighter(QTextEdit *parent = 0);
	QColor textColorQuote() const { return quotationFormat.foreground().color(); };

protected:
	void highlightBlock(const QString &text);

private:
	QTextCharFormat quotationFormat;

signals:

public slots:
	void setTextColorQuote(QColor textColorQuote);

};

#endif // RSSYNTAXHIGHLIGHTER_H
