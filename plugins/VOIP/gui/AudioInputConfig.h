/* Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _AUDIOINPUTCONFIG_H
#define _AUDIOINPUTCONFIG_H

#include <QAudioInput>
#include <QWidget>

#include "retroshare-gui/configpage.h"

#include "ui_AudioInputConfig.h"
#include "SpeexProcessor.h"
#include "AudioStats.h"

class AudioInputConfig : public ConfigPage 
{
	Q_OBJECT

	private:
		Ui::AudioInput ui;
		QAudioInput* inputDevice;
		QtSpeex::SpeexInputProcessor* inputProcessor;
		AudioBar* abSpeech;
		bool loaded;


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
		void loadSettings();
		void emptyBuffer();

		void on_qsTransmitHold_valueChanged(int v);
		void on_qsAmp_valueChanged(int v);
		void on_qsNoise_valueChanged(int v);
		void on_qcbTransmit_currentIndexChanged(int v);
		void on_Tick_timeout();
		void on_qpbAudioWizard_clicked();
		void on_qcbEchoCancel_clicked();
};

#endif
