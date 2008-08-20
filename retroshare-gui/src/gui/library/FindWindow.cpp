#include <QtGui>
//#include "ui_window.h"
#include "findwindow.h"
#include "mainwindowimpl.h"

extern QString fileToFind;

 Window::Window(QWidget *parent)
     : QDialog(parent)
 {
     browseButton = createButton(tr("&Browse..."), SLOT(browse()));
     findButton = createButton(tr("&Find"), SLOT(find()));
	 
	 fileComboBox = createComboBox(fileToFind);
     textComboBox = createComboBox();
     directoryComboBox = createComboBox();

     fileLabel = new QLabel(tr("Named:"));
     textLabel = new QLabel(tr("Containing text:"));
     directoryLabel = new QLabel(tr("In directory:"));
     filesFoundLabel = new QLabel;

     createFilesTable();

     QHBoxLayout *buttonsLayout = new QHBoxLayout;
     buttonsLayout->addStretch();
     buttonsLayout->addWidget(findButton);

     QGridLayout *mainLayout = new QGridLayout;
     mainLayout->addWidget(fileLabel, 0, 0);
     mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
     mainLayout->addWidget(textLabel, 1, 0);
     mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
     mainLayout->addWidget(directoryLabel, 2, 0);
     mainLayout->addWidget(directoryComboBox, 2, 1);
     mainLayout->addWidget(browseButton, 2, 2);
     mainLayout->addWidget(filesTable, 3, 0, 1, 3);
     mainLayout->addWidget(filesFoundLabel, 4, 0);
     mainLayout->addLayout(buttonsLayout, 5, 0, 1, 3);
     setLayout(mainLayout);

     setWindowTitle(tr("Find Files"));
     resize(700, 300);
 }

 void Window::browse()
 {
     QString directory = QFileDialog::getExistingDirectory(this,
                                tr("Find Files"), QDir::currentPath());
     if (!directory.isEmpty()) {
         directoryComboBox->addItem(directory);
         directoryComboBox->setCurrentIndex(directoryComboBox->currentIndex() + 1);
     }
 }

 void Window::find()
 {
     filesTable->setRowCount(0);

     QString fileName = fileComboBox->currentText();
     QString text = textComboBox->currentText();
     QString path = directoryComboBox->currentText();
    

     QDir directory = QDir(path);
     QStringList files,folders;
     
     if (fileName.isEmpty())
         fileName = "*";
     files = directory.entryList(QStringList(fileName),
                                 QDir::Files |QDir::NoSymLinks);
     folders=directory.entryList(QStringList(fileName),
                                 QDir::Dirs | QDir::NoSymLinks);
     //QMessageBox::information(this,"path",path);

     files = findFiles(directory, files, text);
     if(text.isEmpty()){
     	showFolders(directory, folders);
    	showFiles(directory, files);
    	}
     else if(!text.isEmpty()){
    	showFiles(directory, files);
 	    }
 	    QLocale d;
 	    if (files.isEmpty())
    	{
    		if(d.toString(folders.size())!=0){
    			for(int i=folders.size();i>0;i--)
    		directory.cd(folders[i]); 
    		//QMessageBox::information(this,"dir",directory.dirName());
   			}
   		}
 }
QStringList Window::findFiles(  QDir &directory,  QStringList &files,
                                QString &text)
 {
        QStringList foundFiles;
      
          for (int i = 0; i < files.size(); ++i) {
         QFile file(directory.absoluteFilePath(files[i]));

         if (file.open(QIODevice::ReadOnly)) {
             QString line;
             QTextStream in(&file);
             while (!in.atEnd()) {
                 line = in.readLine();
                 if (line.contains(text)) {
                     foundFiles << files[i];
                     break;
                  }
              }
          }
      }
     return foundFiles;
 }

 void Window::showFiles(const QDir &directory, const QStringList &files)
 {
     for (int i = 0; i < files.size(); ++i) {
         QFile file(directory.absoluteFilePath(files[i]));
         qint64 size = QFileInfo(file).size();

         QTableWidgetItem *fileNameItem = new QTableWidgetItem(files[i]);
         fileNameItem->setFlags(Qt::ItemIsEnabled);
         QTableWidgetItem *sizeItem = new QTableWidgetItem(tr("%1 KB")
                                              .arg(int((size + 1023) / 1024)));
         sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
         sizeItem->setFlags(Qt::ItemIsEnabled);

         int row = filesTable->rowCount();
         filesTable->insertRow(row);
         filesTable->setItem(row, 0, fileNameItem);
         filesTable->setItem(row, 1, sizeItem);
     }
     filesFoundLabel->setText(tr("%1 file(s) found").arg(files.size()));
 }

void Window::showFolders(const QDir &directory, const QStringList &folders)
 {
     for (int i = 0; i < folders.size(); ++i) {
         QDir folder(directory.absoluteFilePath(folders[i]));
         
         QTableWidgetItem *fileNameItem = new QTableWidgetItem(folders[i]);
         fileNameItem->setFlags(Qt::ItemIsEnabled);
         

         int row = filesTable->rowCount();
         filesTable->insertRow(row);
         filesTable->setItem(row, 0, fileNameItem);
        
     }
    return ;
 }

 QPushButton *Window::createButton(const QString &text, const char *member)
 {
     QPushButton *button = new QPushButton(text);
     connect(button, SIGNAL(clicked()), this, member);
     return button;
 }

 QComboBox *Window::createComboBox(const QString &text)
 {
     QComboBox *comboBox = new QComboBox;
     comboBox->setEditable(true);
     comboBox->addItem(text);
     comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
     return comboBox;
 }

 void Window::createFilesTable()
 {
     filesTable = new QTableWidget(0, 2);
     QStringList labels;
     labels << tr("File Name") << tr("Size");
     filesTable->setHorizontalHeaderLabels(labels);
     filesTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
     filesTable->verticalHeader()->hide();
     filesTable->setShowGrid(false);
 }
