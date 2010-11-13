#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <xine.h>
#include <xine/xineutils.h>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QTreeWidgetItem>
#include <QTime>
#include <QPixmap>
#include <QIcon>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>


class filefunctions{

private:
xine_t	*xine;
xine_stream_t	*stream;
xine_video_port_t	*vo_port;
xine_audio_port_t	*ao_port;
xine_event_queue_t	*event_queue;
char  *vo_driver;
char  *ao_driver;

QString tempdir;

static QString codec;
static int length; // length of the stream
static int fwidth;
static int fheight;
void saveRGBImage(uchar *,int,int,int);
QString getXineError(int);
QProgressDialog *progress;
void savebuffer(QByteArray &, QImage *);
bool saveimage(const QByteArray & , const QString &);

public:
filefunctions();
~filefunctions();
static QString cfilename;
static QTreeWidget *treeObj;
static QStringList tempimages;
QImage* currImage;

static QString getCodec();
static QString getImageWidth();
static QString getImageHeight();
static int getLength();

void open();
void getFrames();
void getFramesTime();

void yuy2Toyv12 (uint8_t *, uint8_t *, uint8_t *, uint8_t *, int , int );
uchar * yv12ToRgb (uint8_t *, uint8_t *, uint8_t *, int , int );
void SaveFrame(int);
QTime postime(const int);

};
#endif
