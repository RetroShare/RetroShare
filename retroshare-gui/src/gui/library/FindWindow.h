 #ifndef FINDWINDOW_H
 #define FINDWINDOW_H

 #include <QDialog>

 class QComboBox;
 class QDir;
 class QLabel;
 class QPushButton;
 class QTableWidget;

 class Window : public QDialog
 {
     Q_OBJECT

 public:
     Window(QWidget *parent = 0);

 private slots:
     void browse();
     void find();

 private:
     QStringList findFiles( QDir &directory,  QStringList &files,
                            QString &text);
     void showFiles(const QDir &directory, const QStringList &files);
     void showFolders(const QDir &directory, const QStringList &folders);
     QPushButton *createButton(const QString &text, const char *member);
     QComboBox *createComboBox(const QString &text = QString());
     void createFilesTable();

     QComboBox *fileComboBox;
     QComboBox *textComboBox;
     QComboBox *directoryComboBox;
     QLabel *fileLabel;
     QLabel *textLabel;
     QLabel *directoryLabel;
     QLabel *filesFoundLabel;
     QPushButton *browseButton;
     QPushButton *findButton;
     QTableWidget *filesTable;
 };
#endif
