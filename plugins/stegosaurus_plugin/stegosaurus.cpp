/****************************************************************************
***************************Author: Agnit Sarkar******************************
**************************CopyRight: April 2009******************************
********************* Email: agnitsarkar@yahoo.co.uk*************************
****************************************************************************/
#include "stegosaurus.h"

StegoSaurus::StegoSaurus(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);
	bitsAvailable=0;
	sizeofData=0;
	inputFileSize=0;
}

StegoSaurus::~StegoSaurus()
{
	
}


void StegoSaurus::on_btnDryImg_clicked()
{
	static QString srcImgFileName="/home/";
	srcImgFileName= QFileDialog::getOpenFileName(this, "Open Bitmap", srcImgFileName, "Bitmap Files(*.bmp)");

	if(srcImgFileName.isEmpty()){
		ui.lblDryImgDisplay->setText("<h2>Drag Image Here<br><br>OR<br><br>Select it by browsing<br><br>(BMP Images only)</h2>");
		ui.lblDryImgDisplay->setPixmap(QPixmap(":/StegoSaurus/images/stegosaurus-256x256.png"));
		bitsAvailable=0;
		ui.txtDryImg->setText("");
		ui.lblDryImgInfo->setText("No image Loaded");
		return;
	}
	setDryImage(srcImgFileName);
	
}

void StegoSaurus::on_btnFiletoHide_clicked()
{
	static QString srcFileName="/home/";
	srcFileName= QFileDialog::getOpenFileName(this, "Open file to hide", srcFileName);

	if(srcFileName.isEmpty()){
		inputFileSize=0;
		ui.txtFiletoHide->setText("");
		ui.lblFiletoHideinfo->setText("No file loaded");
		return;
	}

	//Get File information
	QFileInfo fi= QFileInfo(srcFileName);
	inputFileSize= fi.size();
	ui.txtFiletoHide->setText(srcFileName);
	ui.lblFiletoHideinfo->setText(QString("Size=%L1 bytes").arg(inputFileSize));

}

void StegoSaurus::resizeEvent(QResizeEvent * event)  
{
	//Set the label image
	if(ui.txtDryImg->text().isEmpty())
		return;
	QPixmap pixmap(ui.txtDryImg->text());
	pixmap= pixmap.scaledToHeight(ui.lblDryImgDisplay->height());
	ui.lblDryImgDisplay->setPixmap(pixmap);

	QPixmap pixmap2(ui.txtWetImg->text());
	pixmap2= pixmap2.scaledToHeight(ui.lblWetImgDisplay->height());
	ui.lblWetImgDisplay->setPixmap(pixmap2);
}

void StegoSaurus::on_btnWetImg_clicked()
{
	static QString srcImgFileName="/home/";
	srcImgFileName= QFileDialog::getOpenFileName(this, "Open Bitmap", srcImgFileName, "Bitmap Files(*.bmp)");

	if(srcImgFileName.isEmpty()){
		ui.lblWetImgDisplay->setText("<h2>Drag Image Here<br><br>OR<br><br>Select it by browsing<br><br>(BMP Images only)</h2>");
		ui.lblWetImgDisplay->setPixmap(QPixmap(":/StegoSaurus/images/stegosaurus-256x256.png"));
		ui.lblWetImgInfo->setText("No image loaded");
		ui.txtWetImg->setText("");
		sizeofData=0;
		return;
	}
	
	setWetImage(srcImgFileName);
	
}
void StegoSaurus::setDryImage(const QString &srcImgFileName)
{
	//Get file info
	dryImage= new QImage(srcImgFileName);
	unsigned int height= dryImage->height();
	unsigned int width= dryImage->width();
	int depth= dryImage->depth();
	//Check the depth, and return if it is not 24-bit
	if(depth<24){
		QMessageBox::information(this, "StegoSaurus", "Must be 24-bit bitmap.");
		ui.txtDryImg->setText("");
		ui.lblDryImgInfo->setText("No image Loaded");
		return;
	}

	//Set the label image
	QPixmap pixmap(srcImgFileName);
	pixmap= pixmap.scaledToHeight(ui.lblDryImgDisplay->height());
	ui.lblDryImgDisplay->setPixmap(pixmap);
	bitsAvailable= height*width*3;

	ui.txtDryImg->setText(srcImgFileName);

	if(bitsAvailable){
			ui.lblDryImgInfo->setText(QString("Height= %L1	Width=%L2	Free Space=%L3 bytes").arg(height)
				.arg(width).arg(bitsAvailable/8));
	}else{
		ui.lblDryImgInfo->setText("No room for data");
		qApp->beep();
	}
}
void StegoSaurus::setWetImage(const QString &srcImgFileName)
{
	//Get file info
	wetImage= new QImage(srcImgFileName);
	unsigned int height= wetImage->height();
	unsigned int width= wetImage->width();
	int depth= wetImage->depth();
	
	//Check the depth, and return if it is not 24-bit
	if(depth<24){
		QMessageBox::information(this, "StegoSaurus", "Must be 24-bit bitmap.");
		ui.txtWetImg->setText("");
		ui.lblWetImgInfo->setText("No image Loaded");
		return;
	}

	//Set the label image
	QPixmap pixmap(srcImgFileName);
	pixmap= pixmap.scaledToHeight(ui.lblWetImgDisplay->height());
	ui.lblWetImgDisplay->setPixmap(pixmap);
		
	//Read the size of the embeded data
	 QFile imgFile(srcImgFileName);
	 if (!imgFile.open(QIODevice::ReadOnly)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to open bitmap file.");
         return;
	 }
	 if(imgFile.seek(6)){
		 imgFile.read((char *)&sizeofData, 4);
	 }else{
		 QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
		 return;
	 }

	ui.txtWetImg->setText(srcImgFileName);

	if(sizeofData){
			ui.lblWetImgInfo->setText(QString("Height= %L1	Width=%L2	Probable Size of Data=%L3 bytes").arg(height)
				.arg(width).arg(sizeofData));
	}else{
		ui.lblWetImgInfo->setText("No data in image.");
		qApp->beep();
	}
}

//The drag and drop methods
void StegoSaurus::dragEnterEvent(QDragEnterEvent *event)
{
	if(ui.tabWidget->currentIndex()==2)return;

	if (event->mimeData()->hasFormat("text/uri-list")){
		QList<QUrl> urls = event->mimeData()->urls();
		if (urls.isEmpty())
			return;
		QString fileName = urls.first().toLocalFile();
		//Check the extension
		if (fileName.isEmpty()||!fileName.endsWith(".bmp", Qt::CaseInsensitive))
			return;
        event->acceptProposedAction();
	}
}

void StegoSaurus::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;
    QString fileName = urls.first().toLocalFile();
    if (fileName.isEmpty())
        return;

	switch(ui.tabWidget->currentIndex()){
		case 0:
			setDryImage(fileName);
			break;
		case 1:
			setWetImage(fileName);
			break;
		default:
			break;
	}

}

//The hide methods
void StegoSaurus::on_btnHide_clicked()
{
	//Get the filenames
	QString imgFileName= ui.txtDryImg->text();
	QString inputFileName= ui.txtFiletoHide->text();
	//Check if both image file and data file has been selected
	if(imgFileName.isEmpty()||inputFileName.isEmpty()){
		QMessageBox::information(this, "StegoSaurus", "Please select a bitmap image and a data file.");
		return;
	}
	//Now check if size of the data file is less than that of the space available
	if(inputFileSize*8>bitsAvailable){
		QMessageBox::information(this, "StegoSaurus", "Not enough space available in image to hide the data.");
		return;
	}
	static QString targetImgName="/home/";
	targetImgName= QFileDialog::getSaveFileName(this, "Save bitmap with data to", targetImgName, "Bitmap Files(*.bmp)");

	if(targetImgName.isEmpty())return;

	//If an image file with the same name exists delete it and then copy, unless source and target are same
	if(imgFileName!=inputFileName){
		QFile::remove(targetImgName);
		QFile::copy(imgFileName, targetImgName);
	}

	//Calculate the no. of padding bytes
	unsigned int width= dryImage->width();
	unsigned int padding=0;
	if((width*3)%4==0)
		padding=0;
	unsigned int quotient= (width*3)/4;
	padding= 4*(quotient+1)- width*3;

	/////////////////////////////////Hide the data////////////////////////////////////////////
	 QFile inputFile(inputFileName);
	 if (!inputFile.open(QIODevice::ReadOnly)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to open data file for reading.");
         return;
	 }

	 //Stamp the size of the data file to the reserved bytes of the image file
	 QFile imgOutputFile(targetImgName);
	 if (!imgOutputFile.open(QIODevice::ReadWrite)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to open bitmap file.");
         return;
	 }
	 if(imgOutputFile.seek(6)){
		 imgOutputFile.write((char *)&inputFileSize, 4);
	 }else{
		 QMessageBox::warning(this, "StegoSaurus", "Unable to write to bitmap file.");
		 return;
	 }
	 //Read the dataoffset  of the bitmap
	 unsigned int dataoffset=0;
	 if(imgOutputFile.seek(10)){
		 imgOutputFile.read((char *)&dataoffset, 4);
	 }else{
		 QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
		 return;
	 }

	 //Move by dataoffset bytes
	 if(!imgOutputFile.seek(dataoffset)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
		 return;
	 }
	 QProgressDialog progress("Embedding Data", "Cancel", 0, inputFileSize, this);
	 progress.show();
	 unsigned int progresscount=0;
	 unsigned int countBytes=0;
	 while (!inputFile.atEnd()){
		//Read a byte
		unsigned char dataByte=0;
		inputFile.getChar((char *)&dataByte);
		
		//Store each bit of the data byte in 1 byte of the image pixel
		//starting with the MSB
		for(int i=7; i>=0; i--){
			//Get the bit at position i in the data byte
			unsigned char checkByte= 0x01;
			checkByte= checkByte<<i;
			unsigned char dataBit=(dataByte & checkByte)>>i;
			
			unsigned char imageByte=0;
			//To ignore the padding bytes
			if(countBytes>=width*3){
				countBytes= 0;
				//Advance the read pointer
				for(int j=0; j<padding; j++){
					imgOutputFile.getChar((char *)&imageByte);
				}
			}
			//Get a pixel byte from the bitmap
			imgOutputFile.getChar((char *)&imageByte);
			//Get the LSB 
			checkByte= imageByte & 0x01;
			//Check if the databit and the LSB is different
			if(dataBit!=checkByte){
				//Alter the LSB to the databit 
				if(dataBit){
					//If 1 set the LSB
					imageByte= imageByte | 0x01;
				}else{
					//Reset the LSB
					imageByte= imageByte & 0xFE;
				}
				//Get the current pointer position
				int pos= imgOutputFile.pos();
				//Put it back by 1 byte to write the current byte
				if(!imgOutputFile.seek(pos-1)){
					QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
					return;
				}
				//Write the byte
				imgOutputFile.putChar(imageByte);
			}
			countBytes++;
		}
		progresscount++;
		progress.setValue(progresscount);
		if (progress.wasCanceled()){
			QMessageBox::warning(this, "StegoSaurus", "Operation Aborted.");
			imgOutputFile.close();
			inputFile.close();
			return;
		}
     }
	 progress.hide();
	 //Close the files 
	 imgOutputFile.close();
	 inputFile.close();

	 QMessageBox::information(this, "StegoSaurus", "Data Embedded Successfully.");

}

//The recover methods
void StegoSaurus::on_btnRecover_clicked()
{
	//Get the filename
	QString imgFileName= ui.txtWetImg->text();
	
	//Check image file has been selected
	if(imgFileName.isEmpty()){
		QMessageBox::information(this, "StegoSaurus", "Please select a bitmap image.");
		return;
	}
	
	static QString targetFileName="/home/";
	targetFileName= QFileDialog::getSaveFileName(this, "Save recovered data to", targetFileName, "All Files(*.*)");

	if(targetFileName.isEmpty())return;

	//If a file with the same name exists delete it
	QFile::remove(targetFileName);

	//Calculate the no. of padding bytes
	unsigned int width= wetImage->width();
	unsigned int padding=0;
	if((width*3)%4==0)
		padding=0;
	unsigned int quotient= (width*3)/4;
	padding= 4*(quotient+1)- width*3;

	/////////////////////////////////Recover the data////////////////////////////////////////////
	QFile targetFile(targetFileName);
	 if (!targetFile.open(QIODevice::WriteOnly)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to open data file.");
         return;
	 }

	 QFile imgInputFile(imgFileName);
	 if (!imgInputFile.open(QIODevice::ReadOnly)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to open bitmap file.");
         return;
	 }

	 //Read the dataoffset  of the bitmap
	 unsigned int dataoffset=0;
	 if(imgInputFile.seek(10)){
		 imgInputFile.read((char *)&dataoffset, 4);
	 }else{
		 QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
		 return;
	 }

	 //Move by dataoffset bytes
	 if(!imgInputFile.seek(dataoffset)){
		 QMessageBox::warning(this, "StegoSaurus", "Unable to read bitmap file.");
		 return;
	 }

	 QProgressDialog progress("Recovering Data", "Cancel", 0, sizeofData, this);
	 progress.show();
	 unsigned int progresscount=0;
	 unsigned int countBytes=0;
	 //Iterate to recover targetFileSize bytes
	 for(unsigned int byteNo= 0; byteNo< sizeofData; byteNo++){
 		 //construct each byte
		 unsigned char dataByte= 0x00;
		 for(int bitNo=7; bitNo>=0; bitNo--){
			unsigned char imageByte=0xFF;
			//To ignore the padding bytes
			if(countBytes>=width*3){
				countBytes= 0;
				//Advance the read pointer
				for(int j=0; j<padding; j++){
					imgInputFile.getChar((char *)&imageByte);
				}
			}

			//Read a byte from the bitmap file
			imgInputFile.getChar((char *)&imageByte);
			countBytes++;
			//Get the LSB 
			unsigned char dataBit= imageByte & 0x01;
			//Set the bit in data byte at bitNo position if databit is set
			if(dataBit){
				unsigned char tempByte= 0x01;
				dataByte= dataByte | (tempByte<<bitNo);
			}
		 }
		 //Write the byte
		 targetFile.putChar(dataByte);
		 
		 //Update the progressbar
		 progresscount++;
		 progress.setValue(progresscount);
		 if (progress.wasCanceled()){
			QMessageBox::warning(this, "StegoSaurus", "Operation Aborted.");
			imgInputFile.close();
			targetFile.close();
			return;
		 }
	 }

	 progress.hide();
	 //Close the files 
	 targetFile.close();
	 imgInputFile.close();

	 QMessageBox::information(this, "StegoSaurus", "Data Recovery Successful.");


}