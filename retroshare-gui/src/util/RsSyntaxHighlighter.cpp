#include "RsSyntaxHighlighter.h"

RsSyntaxHighlighter::RsSyntaxHighlighter(QTextEdit *parent)
	: QSyntaxHighlighter(parent)
{

}

void RsSyntaxHighlighter::highlightBlock(const QString &text)
{
	QRegExp endl("[\\r\\n\\x2028]"); //Usually 0x2028 cahracter is used for newline, no idea why
	int index = 0;
	QStringList lines = text.split(endl);
	foreach (const QString &line, lines) {
		if(line.trimmed().startsWith('>')) {
			setFormat(index, line.length(), quotationFormat);
		}
		index += line.length() + 1;
	}
	//Make it work with the compact chat style
	if(lines.length() > 0){
		int i = lines[0].indexOf(": >");
		if(i != -1) {
			setFormat(i+2, lines[0].length()-i-2, quotationFormat);
		}
	}
}

void RsSyntaxHighlighter::setTextColorQuote(QColor textColorQuote)
{
	quotationFormat.setForeground(textColorQuote);
	this->rehighlight();
}

//Dumping the raw unicode string into the console in Base64 encoding
/*
		QByteArray uniline;
		const QChar* qca = line.unicode();
		for(int i=0; qca[i]!='\0' ;++i)
		{
			uniline.append(qca[i].row());
			uniline.append(qca[i].cell());
		}
		std::cout << "Line: " << uniline.toBase64().toStdString() << std::endl;
 */
