#ifndef RSSYNTAXHIGHLIGHTER_H
#define RSSYNTAXHIGHLIGHTER_H

#include <QObject>
#include <QSyntaxHighlighter>
#include <QTextEdit>

class RsSyntaxHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	RsSyntaxHighlighter(QTextEdit *parent = 0);

protected:
	void highlightBlock(const QString &text);

private:
	QTextCharFormat quotationFormat;

signals:

public slots:
};

#endif // RSSYNTAXHIGHLIGHTER_H
