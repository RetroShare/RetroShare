/*******************************************************************************
 * plugins/VOIP/gui/QVideoDevice.h                                             *
 *                                                                             *
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

#include "SpeexProcessor.h"

#include <util/rsmemory.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <math.h>
#include <cstdlib>
#include <QDateTime>
#include <limits.h>

#include "interface/rsVOIP.h"

//#include "gui/settings/rsharesettings.h"

#define iroundf(x) ( static_cast<int>(x) )

using namespace QtSpeex;

SpeexInputProcessor::SpeexInputProcessor(QObject *parent) : QIODevice(parent),
    iMaxBitRate(16800),
    lastEchoFrame(NULL),
    enc_state(0),
    enc_bits(),
    send_timestamp(0),
    bResetProcessor(true),
    preprocessor(0),
    echo_state(0),
    inputBuffer()
{
        enc_bits = new SpeexBits;
        speex_bits_init(enc_bits);
        speex_bits_reset(enc_bits);
        enc_state = speex_encoder_init(&speex_wb_mode);

        int iArg = 0;
        speex_encoder_ctl(enc_state,SPEEX_SET_VAD, &iArg);
        speex_encoder_ctl(enc_state,SPEEX_SET_DTX, &iArg);

        float fArg=9.0;
        speex_encoder_ctl(enc_state,SPEEX_SET_VBR_QUALITY, &fArg);

        iArg = iMaxBitRate;
        speex_encoder_ctl(enc_state, SPEEX_SET_VBR_MAX_BITRATE, &iArg);

        iArg = 10;
        speex_encoder_ctl(enc_state,SPEEX_SET_COMPLEXITY, &iArg);

        iArg = 9;
        speex_encoder_ctl(enc_state,SPEEX_SET_QUALITY, &iArg);

        echo_state = NULL;

        //iEchoFreq = iMicFreq = iSampleRate;

        iSilentFrames = 0;
        iHoldFrames = 0;

        bResetProcessor = true;

        //bEchoMulti = false;

        preprocessor = NULL;
        echo_state = NULL;
        //srsMic = srsEcho = NULL;
        //iJitterSeq = 0;
        //iMinBuffered = 1000;

        //psMic = new short[iFrameSize];
        psClean = new short[SAMPLING_RATE];

        //psSpeaker = NULL;

        //iEchoChannels = iMicChannels = 0;
        //iEchoFilled = iMicFilled = 0;
        //eMicFormat = eEchoFormat = SampleFloat;
        //iMicSampleSize = iEchoSampleSize = 0;

        bPreviousVoice = false;

        //pfMicInput = pfEchoInput = pfOutput = NULL;

        iRealTimeBitrate = 0;
        dPeakSignal = dPeakSpeaker = dMaxMic = dPeakMic = dPeakCleanMic = dVoiceAcivityLevel = 0.0;
        dMaxMic = 0.0;

        //if (g.uiSession) {
        //TODO : get the maxbitrate from a rs service or a dynamic code
        //iMaxBitRate = 10000;
        //}

        //bRunning = true;
    }

SpeexInputProcessor::~SpeexInputProcessor() {
    if (preprocessor) {
        speex_preprocess_state_destroy(preprocessor);
    }
        if (echo_state) {
           speex_echo_state_destroy(echo_state);
        }

        speex_encoder_destroy(enc_state);


        speex_bits_destroy(enc_bits);
        delete enc_bits;
        delete[] psClean;
}

QByteArray SpeexInputProcessor::getNetworkPacket() {
        return outputNetworkBuffer.takeFirst();
}

bool SpeexInputProcessor::hasPendingPackets() {
        return !outputNetworkBuffer.empty();
}

qint64 SpeexInputProcessor::writeData(const char *data, qint64 maxSize) {
        int iArg;
        int i;
        float sum;
        short max;

        inputBuffer += QByteArray(data, maxSize);

        while((size_t)inputBuffer.size() > FRAME_SIZE * sizeof(qint16)) {

                QByteArray source_frame = inputBuffer.left(FRAME_SIZE * sizeof(qint16));
                short* psMic = (short *)source_frame.data();

                //let's do volume detection
                sum=1.0f;
                for (i=0;i<FRAME_SIZE;i++) {
                        sum += static_cast<float>(psMic[i] * psMic[i]);
                }
                dPeakMic = qMax(20.0f*log10f(sqrtf(sum / static_cast<float>(FRAME_SIZE)) / 32768.0f), -96.0f);

                max = 1;
                for (i=0;i<FRAME_SIZE;i++)
                        max = static_cast<short>(std::abs(psMic[i]) > max ? std::abs(psMic[i]) : max);
                dMaxMic = max;

                dPeakSpeaker = 0.0;

                QMutexLocker l(&qmSpeex);

                if (bResetProcessor) {
                        if (preprocessor)
                                speex_preprocess_state_destroy(preprocessor);

                        preprocessor = speex_preprocess_state_init(FRAME_SIZE, SAMPLING_RATE);

                        iArg = 1;
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_VAD, &iArg);
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC, &iArg);
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_DENOISE, &iArg);
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_DEREVERB, &iArg);

                        iArg = 30000;
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC_TARGET, &iArg);

                        iArg = -60;
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC_DECREMENT, &iArg);

                        iArg = rsVOIP->getVoipiNoiseSuppress();
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &iArg);

                        if (echo_state) {
                                iArg = SAMPLING_RATE;
                                speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &iArg);
                                speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_ECHO_STATE, echo_state);
                        }

                        bResetProcessor = false;
                }

                float v = 30000.0f / static_cast<float>(rsVOIP->getVoipiMinLoudness());

                iArg = iroundf(floorf(20.0f * log10f(v)));
                speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN, &iArg);

                speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_GET_AGC_GAIN, &iArg);
                float gainValue = static_cast<float>(iArg);
                iArg = rsVOIP->getVoipiNoiseSuppress() - iArg;
                speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &iArg);

                short * psSource = psMic;
                if (echo_state && rsVOIP->getVoipEchoCancel()) {
                    speex_echo_playback(echo_state, (short*)lastEchoFrame->data());
                    speex_echo_capture(echo_state,psMic,psClean);
                    psSource = psClean;
                }
                speex_preprocess_run(preprocessor, psSource);

                //we will now analize the processed signal
                sum=1.0f;
                for (i=0;i<FRAME_SIZE;i++)
                        sum += static_cast<float>(psSource[i] * psSource[i]);
                float micLevel = sqrtf(sum / static_cast<float>(FRAME_SIZE));
                dPeakSignal = qMax(20.0f*log10f(micLevel / 32768.0f), -96.0f);

                spx_int32_t prob = 0;
                speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_GET_PROB, &prob);//speech probability
                fSpeechProb = static_cast<float>(prob) / 100.0f;

                // clean microphone level: peak of filtered signal attenuated by AGC gain
                dPeakCleanMic = qMax(dPeakSignal - gainValue, -96.0f);
                dVoiceAcivityLevel = 0.4f * fSpeechProb + 0.6f * (1.0f + dPeakCleanMic / 96.0f);//ponderation for speech detection and audio amplitude

                bool bIsSpeech = false;

                if (dVoiceAcivityLevel > (static_cast<float>(rsVOIP->getVoipfVADmax()) / 32767))
                        bIsSpeech = true;
                else if (dVoiceAcivityLevel > (static_cast<float>(rsVOIP->getVoipfVADmin()) / 32767) && bPreviousVoice)
                        bIsSpeech = true;

                if (! bIsSpeech) {
                        iHoldFrames++;
                        if (iHoldFrames < rsVOIP->getVoipVoiceHold())
                                bIsSpeech = true;
                } else {
                        iHoldFrames = 0;
                }


                if (rsVOIP->getVoipATransmit() == RsVOIP::AudioTransmitContinous) {
                        bIsSpeech = true;
                }
                else if (rsVOIP->getVoipATransmit() == RsVOIP::AudioTransmitPushToTalk)
                        bIsSpeech = false;//g.s.uiDoublePush && ((g.uiDoublePush < g.s.uiDoublePush) || (g.tDoublePush.elapsed() < g.s.uiDoublePush));

                //bIsSpeech = bIsSpeech || (g.iPushToTalk > 0);

                /*if (g.s.bMute || ((g.s.lmLoopMode != RsVOIP::Local) && p && (p->bMute || p->bSuppress)) || g.bPushToMute || (g.iTarget < 0)) {
                        bIsSpeech = false;
                }*/

                if (bIsSpeech) {
                        iSilentFrames = 0;
                } else {
                        iSilentFrames++;
                }

                /*if (p) {
                        if (! bIsSpeech)
                                p->setTalking(RsVOIP::Passive);
                        else if (g.iTarget == 0)
                                p->setTalking(RsVOIP::Talking);
                        else
                                p->setTalking(RsVOIP::Shouting);
                }*/


                if (! bIsSpeech && ! bPreviousVoice) {
                        iRealTimeBitrate = 0;
                        /*if (g.s.iIdleTime && ! g.s.bDeaf && ((tIdle.elapsed() / 1000000ULL) > g.s.iIdleTime)) {
                                emit doDeaf();
                                tIdle.restart();
                        }*/
                        spx_int32_t increment = 0;
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &increment);
                } else {
                        spx_int32_t increment = 12;
                        speex_preprocess_ctl(preprocessor, SPEEX_PREPROCESS_SET_AGC_INCREMENT, &increment);
                }


                int vbr_on=0;
                //just use fixed bitrate for now
                //encryption of VBR-encoded speech may not ensure complete privacy, as phrases can still be identified, at least in a controlled setting with a small dictionary of phrases, by analysing the pattern of variation of the bit rate.
                if (rsVOIP->getVoipATransmit() == RsVOIP::AudioTransmitVAD) {//maybe we can do fixer bitrate when voice detection is active
                    vbr_on = 1;//test it on for all modes
                } else {//maybe we can do vbr for ppt and continuous
                    vbr_on = 1;
                }
                speex_encoder_ctl(enc_state,SPEEX_SET_VBR, &vbr_on);

                int br = 0;
                speex_encoder_ctl(enc_state, SPEEX_GET_VBR_MAX_BITRATE, &br);
                if (br != iMaxBitRate) {
                        br = iMaxBitRate;
                        speex_encoder_ctl(enc_state, SPEEX_SET_VBR_MAX_BITRATE, &br);
                }
                speex_encoder_ctl(enc_state, SPEEX_GET_BITRATE, &br);
                if (br != iMaxBitRate) {
                        br = iMaxBitRate;
                        speex_encoder_ctl(enc_state, SPEEX_SET_BITRATE, &br);
                }

                if (! bPreviousVoice)
                        speex_encoder_ctl(enc_state, SPEEX_RESET_STATE, NULL);

                if (bIsSpeech) {
                    speex_bits_reset(enc_bits);
                    speex_encode_int(enc_state, psSource, enc_bits);
                    QByteArray networkFrame;
                    networkFrame.resize(speex_bits_nbytes(enc_bits)+4);//add 4 for the frame timestamp for the jitter buffer
                    int packetSize = speex_bits_write(enc_bits, networkFrame.data()+4, networkFrame.size()-4);
                    ((int*)networkFrame.data())[0] = send_timestamp;

                    outputNetworkBuffer.append(networkFrame);
                    emit networkPacketReady();

                    iRealTimeBitrate = packetSize * SAMPLING_RATE / FRAME_SIZE * 8;
                } else {
                    iRealTimeBitrate = 0;
                }
                bPreviousVoice = bIsSpeech;

                //std::cerr << "iRealTimeBitrate : " << iRealTimeBitrate << std::endl;

                send_timestamp += FRAME_SIZE;
                if (send_timestamp >= INT_MAX)
                    send_timestamp = 0;

                inputBuffer = inputBuffer.right(inputBuffer.size() - FRAME_SIZE * sizeof(qint16));
	}

	return maxSize;
}


SpeexOutputProcessor::SpeexOutputProcessor(QObject *parent) : QIODevice(parent),
    outputBuffer()
{
}

SpeexOutputProcessor::~SpeexOutputProcessor() {
    QHashIterator<QString, SpeexJitter*> i(userJitterHash);
    while (i.hasNext()) {
        i.next();
        speex_jitter_destroy(*(i.value()));
        free (i.value());
    }
}

void SpeexOutputProcessor::putNetworkPacket(QString name, QByteArray packet) {
    //buffer:
    //  timestamp | encodedBuf
    // —————–———–——————–———–——————–———–——————–
    //    4       | totalSize – 4
    //the size part (first 4 byets) is not actually used in the logic
    if (packet.size() > 4)
    {
        SpeexJitter* userJitter;
        if (userJitterHash.contains(name)) {
            userJitter = userJitterHash.value(name);
        } else {
            userJitter = (SpeexJitter*)rs_malloc(sizeof(SpeexJitter));
            
            if(!userJitter)
                return ;
            
            speex_jitter_init(userJitter, speex_decoder_init(&speex_wb_mode), SAMPLING_RATE);
            int on = 1;
            speex_decoder_ctl(userJitter->dec, SPEEX_SET_ENH, &on);
            userJitterHash.insert(name, userJitter);
        }

        int recv_timestamp = ((int*)packet.data())[0];
        userJitter->mostUpdatedTSatPut = recv_timestamp;
        if (userJitter->firsttimecalling_get)
            return;

        speex_jitter_put(*userJitter, (char *)packet.data()+4, packet.size()-4, recv_timestamp);
    }
}

bool SpeexInputProcessor::isSequential() const {
        return true;
}

void SpeexInputProcessor::addEchoFrame(QByteArray* echo_frame) {
    if (rsVOIP->getVoipEchoCancel() && echo_frame) {
        QMutexLocker l(&qmSpeex);
        lastEchoFrame = echo_frame;
        if (!echo_state) {//init echo_state
            echo_state = speex_echo_state_init(FRAME_SIZE, ECHOTAILSIZE*FRAME_SIZE);
            int tmp = SAMPLING_RATE;
            speex_echo_ctl(echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);
            bResetProcessor = true;
        }
        lastEchoFrame = echo_frame;
    }
}


qint64 SpeexOutputProcessor::readData(char *data, qint64 maxSize) {

    int ts = 0; //time stamp for the jitter call
    while(outputBuffer.size() < maxSize) {
        QByteArray* result_frame = new QByteArray();
        result_frame->resize(FRAME_SIZE * sizeof(qint16));
        result_frame->fill(0,FRAME_SIZE * sizeof(qint16));
        QHashIterator<QString, SpeexJitter*> i(userJitterHash);
        while (i.hasNext()) {
            i.next();
            SpeexJitter* jitter = i.value();
            QByteArray intermediate_frame;
            intermediate_frame.resize(FRAME_SIZE * sizeof(qint16));
            if (jitter->firsttimecalling_get)
            {
                //int ts = jitter->mostUpdatedTSatPut;
                jitter->firsttimecalling_get = false;
            }
            speex_jitter_get(*jitter, (spx_int16_t*)intermediate_frame.data(), &ts);
            for (int j = 0; j< FRAME_SIZE; j++) {
                short sample1 = ((short*)result_frame->data())[j];
                short sample2 = ((short*)intermediate_frame.data())[j];
                float samplef1 = sample1 / 32768.0f;
                float samplef2 = sample2 / 32768.0f;
                float mixed = samplef1 + 0.8f * samplef2;
                // hard clipping
                if (mixed > 1.0f) mixed = 1.0f;
                if (mixed < -1.0f) mixed = -1.0f;
                ((spx_int16_t*)result_frame->data())[j] = (short)(mixed * 32768.0f);
            }
        }
        outputBuffer += *result_frame;
        emit playingFrame(result_frame);
    }

    QByteArray resultBuffer = outputBuffer.left(maxSize);
    memcpy(data, resultBuffer.data(), resultBuffer.size());

    outputBuffer = outputBuffer.right(outputBuffer.size() - resultBuffer.size());

    return resultBuffer.size();
}

bool SpeexOutputProcessor::isSequential() const {
        return true;
}

void SpeexOutputProcessor::speex_jitter_init(SpeexJitter *jit, void *decoder, int /*sampling_rate*/)
{
   jit->dec = decoder;
   speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &jit->frame_size);

   jit->packets = jitter_buffer_init(jit->frame_size);
    jit->current_packet = new SpeexBits;
   speex_bits_init(jit->current_packet);
   jit->valid_bits = 0;
   jit->firsttimecalling_get = true;
   jit->mostUpdatedTSatPut = 0;
}

void SpeexOutputProcessor::speex_jitter_destroy(SpeexJitter jitter)
{
    if (jitter.dec) {
        speex_decoder_destroy(jitter.dec);
    }
   jitter_buffer_destroy(jitter.packets);
   speex_bits_destroy(jitter.current_packet);
}

void SpeexOutputProcessor::speex_jitter_put(SpeexJitter jitter, char *packet, int len, int timestamp)
{
   JitterBufferPacket p;
   p.data = packet;
   p.len = len;
   p.timestamp = timestamp;
   p.span = jitter.frame_size;
   jitter_buffer_put(jitter.packets, &p);
}

void SpeexOutputProcessor::speex_jitter_get(SpeexJitter jitter, spx_int16_t *out, int *current_timestamp)
{
   int i;
   int ret;
   spx_int32_t activity;
   //int bufferCount = 0;
   JitterBufferPacket packet;
   char data[FRAME_SIZE * ECHOTAILSIZE * 10];
   packet.data = data;
   packet.len = FRAME_SIZE * ECHOTAILSIZE * 10;

   if (jitter.valid_bits)
   {
      /* Try decoding last received packet */
      ret = speex_decode_int(jitter.dec, jitter.current_packet, out);
      if (ret == 0)
      {
         jitter_buffer_tick(jitter.packets);
         return;
      } else {
         jitter.valid_bits = 0;
      }
   }

   if (current_timestamp)
    ret = jitter_buffer_get(jitter.packets, &packet, jitter.frame_size, current_timestamp);
   else
    ret = jitter_buffer_get(jitter.packets, &packet, jitter.frame_size, NULL);

   if (ret != JITTER_BUFFER_OK)
   {
      /* No packet found */
      speex_decode_int(jitter.dec, NULL, out);
   } else {
      speex_bits_read_from(jitter.current_packet, packet.data, packet.len);
      /* Decode packet */
      ret = speex_decode_int(jitter.dec, jitter.current_packet, out);
      if (ret == 0)
      {
         jitter.valid_bits = 1;
      } else {
         /* Error while decoding */
         for (i=0;i<jitter.frame_size;i++)
            out[i]=0;
      }
   }
   speex_decoder_ctl(jitter.dec, SPEEX_GET_ACTIVITY, &activity);
   if (activity < 30)
   {
      jitter_buffer_update_delay(jitter.packets, &packet, NULL);
   }
   jitter_buffer_tick(jitter.packets);
   //ret = jitter_buffer_ctl(jitter.packets, JITTER_BUFFER_GET_AVALIABLE_COUNT, &bufferCount);
   //sprintf(msg, “   get %d bufferCount=%d\n”, speex_jitter_get_pointer_timestamp(jitter), bufferCount);
   //debugPrint(msg);
}

int SpeexOutputProcessor::speex_jitter_get_pointer_timestamp(SpeexJitter jitter)
{
   return jitter_buffer_get_pointer_timestamp(jitter.packets);
}
