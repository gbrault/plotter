#include "source.h"

/*
 * #include <QDateTime>
 *    srand( QDateTime::currentMSecsSinceEpoch() );
 *    qreal randNumber = rand() / static_cast<qreal>( RAND_MAX );
 *    x = (x + (randNumber-0.5))/1.5;
*/

/**
 * @brief simulatedSource::simulatedSource
 * @param signalFreq    in Hz
 * @param sampleFreq    in Hz
 * @param duration      in ms
 */
Source::Source(int signalFreq, int duration, int amplitude, QAudioFormat &format)
{
    displayCounter=0;
    audioCounter=0;
    this->signalFreq = signalFreq;
    // I want a periodic signal
    // signal period = 1/signalFreq
    // Number of period duration/period
    qreal period = 1.0 / qreal(signalFreq);
    qreal Nreal = qreal(duration)/period;
    int N = (int)Nreal;
    int correctedduration = (N*period*1000);
    this->duration = correctedduration;
    this->amplitude = amplitude;
    this->format = new QAudioFormat(format);
    SineGenerator();
    displayTime=0;
    AudioTime=0;
}

void Source::SineGenerator(){
    // this code is for mono
    int sampleIndex,channelBytes ;
    channelBytes = format->sampleSize()/8;
    Q_ASSERT(format->channelCount() == 1);
    int sampleFreq = format->sampleRate();
    int samples = (duration * sampleFreq)/1000;
    signal = new QVector<int>(samples);
    signal->clear();
    for(sampleIndex = 0; sampleIndex<samples; sampleIndex++){
        qreal sin = qSin((2*M_PI*signalFreq*sampleIndex)/qreal(sampleFreq));
        int value =(int)(amplitude * sin);
        signal->append(value);
    }

    signalAudio = new QByteArray();
    qint64 length = samples * format->sampleSize()/8;

    Q_ASSERT((length % (format->sampleSize()/8)) == 0);

    signalAudio->resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(signalAudio->data());
    sampleIndex = 0;

    while (length) {
        int x = signal->at(sampleIndex);
        for (int i=0; i<format->channelCount(); ++i) {
            if (format->sampleSize() == 8 && format->sampleType() == QAudioFormat::UnSignedInt) {
                const quint8 value = static_cast<quint8>(amplitude + x);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format->sampleSize() == 8 && format->sampleType() == QAudioFormat::SignedInt) {
                const qint8 value = static_cast<qint8>(x);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format->sampleSize() == 16 && format->sampleType() == QAudioFormat::UnSignedInt) {
                quint16 value = static_cast<quint16>(amplitude + x);
                if (format->byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<quint16>(value, ptr);
                else
                    qToBigEndian<quint16>(value, ptr);
            } else if (format->sampleSize() == 16 && format->sampleType() == QAudioFormat::SignedInt) {
                qint16 value = static_cast<qint16>(x);
                if (format->byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<qint16>(value, ptr);
                else
                    qToBigEndian<qint16>(value, ptr);
            } else if (format->sampleSize() == 24 && format->sampleType() == QAudioFormat::UnSignedInt) {
                quint32 value = static_cast<quint32>(amplitude + x);
                if (format->byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<quint32>(value, ptr);
                else
                    qToBigEndian<quint32>(value, ptr);
            } else if (format->sampleSize() == 24 && format->sampleType() == QAudioFormat::SignedInt) {
                qint32 value = static_cast<qint32>(x);
                if (format->byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<qint32>(value, ptr);
                else
                    qToBigEndian<qint32>(value, ptr);
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
        ++sampleIndex;
    }
}

int Source::readDisplay(QVector<int> *signal, int duration){
    int sampleFreq = format->sampleRate();
    int samples = (duration * sampleFreq)/1000;
    signal->resize(samples);
    signal->clear();

    for(int i=0; i<samples; i++){
        signal->append(this->signal->at(displayCounter));
        displayCounter++;
        displayCounter = (displayCounter % this->signal->count());
    }
    displayTime += duration;
    return samples;
}


qint64 Source::readAudio(QByteArray *data, int duration )
{
    int sampleFreq = format->sampleRate();
    int samples =  ((duration * sampleFreq)/1000) * format->sampleSize()/8;
    data->resize(samples);
    data->clear();

    for(int i=0; i<samples; i++){
        data->append(this->signalAudio->at(audioCounter));
        audioCounter++;
        audioCounter = (audioCounter % this->signalAudio->count());
    }
    AudioTime +=duration;
    return samples;
}

/**
 * @brief simulatedSource::Value Decode the encoded audio format
 * @param ptr
 * @return
 */
int Source::Value(char*ptr){
    int max;
    int value;
    int endian = format->byteOrder();
    int charInSample = (format->sampleSize() * format->channelCount()) / 8;
    bool sign = (format->sampleType()==QAudioFormat::SampleType::SignedInt);
    switch(charInSample){
    case 1:
        if (sign){
            max=127;
            value = (qint8) *ptr;
        } else {
            max =255;
            value = (quint8) *ptr;
        }
        break;
    case 2:
        if (sign){
            max=32767;
            if(endian==QAudioFormat::BigEndian){
                value= qFromBigEndian<qint16>(*(quint16*)ptr);
            }else{
                value= qFromLittleEndian<qint16>(*(quint16*)ptr);
            }
        } else {
            max =65535;
            if(endian==QAudioFormat::BigEndian){
                value= qFromBigEndian<quint16>(*(quint16 *)ptr);
            }else{
                value= qFromLittleEndian<quint16>(*(quint16 *)ptr);
            }
        }
        break;
    case 3:
        if (sign){
            max=8388607;
            if(endian==QAudioFormat::BigEndian){
                value= (uint)(qFromBigEndian<quint32>(*(quint32 *)ptr) &0xffffff);
            }else{
                value= (uint)(qFromLittleEndian<quint32>(*(quint32 *)ptr) &0xffffff);
            }
            if((value & 0x800000)==0x800000){
                value = ((uint)value & 0x7fffff);
                value = -8388608+(uint)value;
            }
        } else {
            max =16777215;
            if(endian==QAudioFormat::BigEndian){
                value= qFromBigEndian<quint32>(*(quint32 *)ptr) &0xffffff;
            }else{
                value= qFromLittleEndian<quint32>(*(quint32 *)ptr) &0xffffff;
            }
            }
        break;
    }
return value;
}


