/*******************************************************************************
 * plugins/VOIP/gui/QVideoDevice.h                                             *
 *                                                                             *
 * Copyright (C) 2010 Peter Zotov                                              *
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

#include <iostream>

#include <QIODevice>
#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QHash>
#include <QMap>
#include <QStack>

#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#include <speex/speex_jitter.h>

#define SAMPLING_RATE 16000 //must be the same as the speex setted mode (speex_wb_mode)
#define FRAME_SIZE 320 //must be the same as the speex setted mode (speex_wb_mode)
#define ECHOTAILSIZE  10

class SpeexBits;

/** Speex jitter-buffer state. Never use it directly! */
typedef struct SpeexJitter {
   SpeexBits *current_packet;         /**< Current Speex packet */
   int valid_bits;                   /**< True if Speex bits are valid */
   JitterBuffer *packets;            /**< Generic jitter buffer state */
   void *dec;                        /**< Pointer to Speex decoder */
   spx_int32_t frame_size;           /**< Frame size of Speex decoder */
   int mostUpdatedTSatPut;           /**< timestamp of the last packet put */
   bool firsttimecalling_get;
} SpeexJitter;

namespace QtSpeex {
        class SpeexInputProcessor : public QIODevice {
		Q_OBJECT

	public:
                float dPeakSpeaker, dPeakSignal, dMaxMic, dPeakMic, dPeakCleanMic, dVoiceAcivityLevel;
                float fSpeechProb;
                SpeexInputProcessor(QObject* parent = 0);
                virtual ~SpeexInputProcessor();

		bool hasPendingPackets();
                QByteArray getNetworkPacket();

                int iMaxBitRate;
                int iRealTimeBitrate;
                bool bPreviousVoice;

        public slots:
                void addEchoFrame(QByteArray*);

	signals:
		void networkPacketReady();

	protected:
                virtual qint64 readData(char * /*data*/, qint64 /*maxSize*/) {return false;} //not used for input processor
		virtual qint64 writeData(const char *data, qint64 maxSize);
                virtual bool isSequential() const;

        private:
                QByteArray* lastEchoFrame;
                int iSilentFrames;
                int iHoldFrames;

                QMutex qmSpeex;
                void* enc_state;
                SpeexBits* enc_bits;
                int send_timestamp; //set at the encode time for the jitter buffer of the reciever

                bool bResetProcessor;

                SpeexPreprocessState* preprocessor;
                SpeexEchoState       *echo_state;
                short * psClean; //temp buffer for audio sampling after echo cleaning (if enabled)

                QByteArray inputBuffer;
                QList<QByteArray> outputNetworkBuffer;
        };


        class SpeexOutputProcessor : public QIODevice {
                Q_OBJECT

        public:
                SpeexOutputProcessor(QObject* parent = 0);
                virtual ~SpeexOutputProcessor();

                void putNetworkPacket(QString name, QByteArray packet);

        protected:
                virtual qint64 readData(char *data, qint64 maxSize);
                virtual qint64 writeData(const char * /*data*/, qint64 /*maxSize*/) {return 0;} //not used for output processor
                virtual bool isSequential() const;

        signals:
                void playingFrame(QByteArray*);
        private:
                QByteArray outputBuffer;
                QList<QByteArray> inputNetworkBuffer;

                QHash<QString, SpeexJitter*> userJitterHash;

                //SpeexJitter jitter;

                void speex_jitter_init(SpeexJitter *jit, void *decoder, int sampling_rate);
                void speex_jitter_destroy(SpeexJitter jitter);
                void speex_jitter_put(SpeexJitter jitter, char *packet, int len, int timestamp);
                void speex_jitter_get(SpeexJitter jitter, spx_int16_t *out, int *current_timestamp);
                int speex_jitter_get_pointer_timestamp(SpeexJitter jitter);
        };
    }
