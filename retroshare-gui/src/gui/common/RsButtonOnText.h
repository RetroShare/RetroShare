#ifndef RSBUTTONONTEXT_H
#define RSBUTTONONTEXT_H

#include <QHelpEvent>
#include <QPushButton>
#include <QTextEdit>

class RSButtonOnText : public QPushButton
{
	Q_OBJECT

public:
	explicit RSButtonOnText(QWidget *parent = 0);
	explicit RSButtonOnText(const QString &text, QWidget *parent=0);
	RSButtonOnText(const QIcon& icon, const QString &text, QWidget *parent=0);
	RSButtonOnText(QTextEdit *textEdit, QWidget *parent = 0);
	RSButtonOnText(const QString &text, QTextEdit *textEdit, QWidget *parent = 0);
	RSButtonOnText(const QIcon& icon, const QString &text, QTextEdit *textEdit, QWidget *parent = 0);
	~RSButtonOnText();

	QString uuid();
	QString htmlText();
	void appendToText(QTextEdit *textEdit);
	void clear();
	void updateImage();

signals:
	void mouseEnter();
	void mouseLeave();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	bool isEventForThis(QObject *obj, QEvent *event, QPoint &point);

	QString _uuid;
	int _lenght;//Because cursor end position move durring editing
	QTextEdit* _textEdit;
	QWidget* _textEditViewPort;
	QTextCursor* _textCursor;
	bool _mouseOver;
	bool _pressed;

};

#endif // RSBUTTONONTEXT_H
