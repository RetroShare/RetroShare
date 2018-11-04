/*******************************************************************************
 * plugins/VOIP/gui/AudioInputConfig.h                                         *
 *                                                                             *
 * Copyright (C) 2005-2010 Thorvald Natvig <thorvald@natvig.com>               *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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
#pragma once
#pragma once

#include <QAudioInput>
#include <QWidget>

#include "retroshare-gui/configpage.h"

#include "SpeexProcessor.h"
#include "VideoProcessor.h"
#include "AudioStats.h"
#include "gui/common/RSGraphWidget.h"

class voipGraphSource ;

class voipGraph: public RSGraphWidget
{
public:
    voipGraph(QWidget *parent) ;
    
    voipGraphSource *voipSource() const { return _src ; }
    
    void setVoipSource(voipGraphSource *gs) ;
    
private:
    voipGraphSource *_src ;
};

#include "ui_AudioInputConfig.h"

class AudioInputConfig : public ConfigPage 
{
	Q_OBJECT

	private:
		Ui::AudioInput ui;
		QAudioInput* inputAudioDevice;
		QtSpeex::SpeexInputProcessor* inputAudioProcessor;
		AudioBar* abSpeech;
		//VideoDecoder *videoDecoder ;
		//VideoEncoder *videoEncoder ;
		QVideoInputDevice *videoInput ;
        	VideoProcessor *videoProcessor ;
		bool loaded;

        voipGraphSource *graph_source ;

	protected:
		QTimer *qtTick;
		/*void hideEvent(QHideEvent *event);
		  void showEvent(QShowEvent *event);*/

	public:
		/** Default Constructor */
		AudioInputConfig(QWidget * parent = 0, Qt::WindowFlags flags = 0);
		/** Default Destructor */
		~AudioInputConfig();

		/** Saves the changes on this page */
		virtual bool save(QString &errmsg);
		/** Loads the settings for this page */
		virtual void load();

		virtual QPixmap iconPixmap() const { return QPixmap(":/images/talking_on.svg") ; }
		virtual QString pageName() const { return tr("VOIP") ; }
		virtual QString helpText() const { return ""; }
        
private slots:
	void updateAvailableBW(double r);
		void loadSettings();
		void emptyBuffer();
		void togglePreview(bool) ;

		void on_qsTransmitHold_valueChanged(int v);
		void on_qsAmp_valueChanged(int v);
		void on_qsNoise_valueChanged(int v);
		void on_qcbTransmit_currentIndexChanged(int v);
		void on_Tick_timeout();
		void on_qpbAudioWizard_clicked();
		void on_qcbEchoCancel_clicked();
};
