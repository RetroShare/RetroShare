/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "rshare.h"
#include "StatisticDialog.h"
#include <control/bandwidthevent.h>

#include <retroshare/rsconfig.h>

#include <QTime>
#include <QHeaderView>

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "yyyy MM dd hh:mm:ss"

QTime UpTime;
int UpDays;
bool dayChange;
static int Timer=0;

/** Constructor */
StatisticDialog::StatisticDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* Bind events to actions */
  createActions();

   UpDays=0;
   dayChange=false;
   UpTime.start();

  /* Initialize Sent/Receive data counters */
  reset();

  /* Hide Bandwidth Graph Settings frame */
  showSettingsFrame(false);

   /* Set header resize modes and initial section sizes */
   QHeaderView * _stheader = ui.treeWidget-> header();
   _stheader->resizeSection ( 0, 210 );

  QAbstractItemModel * model =ui.treeWidget->model();
  QModelIndex ind2;
  // set Times --> Session --> Since
  ind2=model->index(4,0).child(0,0).child(1,1);
  model->setData(ind2,QDateTime::currentDateTime()
			    .toString(DATETIME_FMT));

  /* Turn off opacity group on unsupported platforms */
#if defined(Q_WS_WIN)
  if(!(QSysInfo::WV_2000 <= QSysInfo::WindowsVersion <= QSysInfo::WV_2003)) {
    ui.frmOpacity->setVisible(false);
  }
#endif

#if defined(Q_WS_X11)
  ui.frmOpacity->setVisible(false);
#endif

  Timer=startTimer(REFRESH_RATE);

}

/**
 Custom event handler. Checks if the event is a bandwidth update event. If it
 is, it will add the data point to the history and updates the graph.*/

void
StatisticDialog::customEvent(QEvent *event)
{
  if (event->type() == CustomEventType::BandwidthEvent) {
    BandwidthEvent *bw = (BandwidthEvent *)event;
    updateGraph(bw->bytesRead(), bw->bytesWritten());
  }
}

void StatisticDialog::timerEvent( QTimerEvent * )
{
  QAbstractItemModel * model =ui.treeWidget->model();
  QModelIndex ind1;
  // set download --> session bytes
  //ind1=model->index(1,0).child(0,0).child(0,1);
  //model->setData(ind1,totalToString(TotBytesDownloaded/1024.0));
  // set upload --> session bytes
  //ind1=model->index(2,0).child(0,0).child(0,1);
  //model->setData(ind1,totalToString(TotBytesUploaded/1024.0));

  // set Times --> Session --> Uptime
  ind1=model->index(4,0).child(0,0).child(0,1);
  if(! dayChange && (UpTime.elapsed()/1000 > 60*60*23)) dayChange=true;
  if( dayChange && (UpTime.elapsed() <10000)) {dayChange=false;UpDays++;}

  QTime elapsed;
  elapsed= QTime(0,0,0).addMSecs(UpTime.elapsed());
  if( UpDays>0)
      model->setData(ind1,QString(tr("%1 days")).arg(UpDays)+" "+elapsed.toString("hh:mm:ss"));
  else
      model->setData(ind1,elapsed.toString("hh:mm:ss"));

 	/* set users/friends/network */
	float downKb = 0;
	float upKb = 0;
	rsConfig->GetCurrentDataRates(downKb, upKb);

        updateGraph(downKb,upKb);
  //

}

/**
 Binds events to actions
*/
void
StatisticDialog::createActions()
{
  connect(ui.btnToggleSettings, SIGNAL(toggled(bool)),
      this, SLOT(showSettingsFrame(bool)));

  connect(ui.btnReset, SIGNAL(clicked()),
      this, SLOT(reset()));

  connect(ui.btnSaveSettings, SIGNAL(clicked()),
      this, SLOT(saveChanges()));

  connect(ui.btnCancelSettings, SIGNAL(clicked()),
      this, SLOT(cancelChanges()));

  connect(ui.sldrOpacity, SIGNAL(valueChanged(int)),
      this, SLOT(setOpacity(int)));
}

/**
 Adds new data to the graph
*/
void
StatisticDialog::updateGraph(quint64 bytesRead, quint64 bytesWritten)
{
  /* Graph only cares about kilobytes */
  ui.frmGraph->addPoints(bytesRead, bytesWritten);
}


/**
 Loads the saved Bandwidth Graph settings
*/
void
StatisticDialog::loadSettings()
{
  /* Set window opacity slider widget */
  ui.sldrOpacity->setValue(_settings->getBWGraphOpacity());



  /* Set the line filter checkboxes accordingly */
  uint filter = Settings->getBWGraphFilter();
  ui.chkReceiveRate->setChecked(filter & BWGRAPH_REC);
  ui.chkSendRate->setChecked(filter & BWGRAPH_SEND);

  /* Set graph frame settings */
  ui.frmGraph->setShowCounters(ui.chkReceiveRate->isChecked(),
                               ui.chkSendRate->isChecked());
}

/**
 Resets the log start time
*/
void
StatisticDialog::reset()
{
  /* Set to current time */
//  ui.statusbar->showMessage(tr("Since:") + " " +
//			    QDateTime::currentDateTime()
//			    .toString(DATETIME_FMT));
  /* Reset the graph */
  ui.frmGraph->resetGraph();
}

/**
 Saves the Bandwidth Graph settings and adjusts the graph if necessary
*/
void
StatisticDialog::saveChanges()
{
  /* Hide the settings frame and reset toggle button */
  showSettingsFrame(false);

  /* Save the opacity */
  Settings->setBWGraphOpacity(ui.sldrOpacity->value());



  /* Save the line filter values */
  Settings->setBWGraphFilter(BWGRAPH_REC, ui.chkReceiveRate->isChecked());
  Settings->setBWGraphFilter(BWGRAPH_SEND, ui.chkSendRate->isChecked());

  /* Update the graph frame settings */
  ui.frmGraph->setShowCounters(ui.chkReceiveRate->isChecked(),
                               ui.chkSendRate->isChecked());

  /* A change in window flags causes the window to disappear, so make sure
   * it's still visible. */
  showNormal();
}

/**
 Simply restores the previously saved settings
*/
void
StatisticDialog::cancelChanges()
{
  /* Hide the settings frame and reset toggle button */
  showSettingsFrame(false);

  /* Reload the settings */
  loadSettings();
}

/**
 Toggles the Settings pane on and off, changes toggle button text
*/
void
StatisticDialog::showSettingsFrame(bool show)
{
  if (show) {
    ui.frmSettings->setVisible(true);
    ui.btnToggleSettings->setChecked(true);
    ui.btnToggleSettings->setText(tr("Hide Settings"));
  } else {
    ui.frmSettings->setVisible(false);
    ui.btnToggleSettings->setChecked(false);
    ui.btnToggleSettings->setText(tr("Show Settings"));
  }
}

/**
 Sets the opacity of the Bandwidth Graph window
*/
void
StatisticDialog::setOpacity(int value)
{
  qreal newValue = value / 100.0;

  /* Opacity only supported by Mac and Win32 */
#if defined(Q_WS_MAC)
  this->setWindowOpacity(newValue);
#elif defined(Q_WS_WIN)
  if(QSysInfo::WV_2000 <= QSysInfo::WindowsVersion <= QSysInfo::WV_2003) {
    this->setWindowOpacity(newValue);
  }
#else
  Q_UNUSED(newValue);
#endif
}



