#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    audio=NULL;
}

void AudioOutput::createDevice()
{
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        ok=0;
        return;
    }
    audio = new QAudioOutput(format, this);
    audio->setBufferSize(buffersize);

    device = audio->start();
    ok = 1;
}

void AudioOutput::configure(int channelCount,
                            int sampleRate,
                            int sampleSize,
                            QAudioFormat::Endian byteOrder,
                            QAudioFormat::SampleType sampleType,
                            int buffersize)
{
    format.setCodec("audio/pcm");
    format.setChannelCount(channelCount);
    format.setSampleRate(sampleRate);
    format.setSampleSize(sampleSize);
    format.setByteOrder(byteOrder);
    format.setSampleType(sampleType);
    this->buffersize=buffersize;
}


void AudioOutput::writeData(QByteArray data)
{
    int readlen = audio->periodSize();
    int chunks = audio->bytesFree() / readlen;

    while (chunks)
    {
        QByteArray middle(data.mid(0, readlen));
        int len = middle.size();
        data.remove(0, len);

        if (len)
            device->write(middle);

        chunks--;
    }
}

void AudioOutput::close(){
    if (audio!=NULL){
        audio->stop();
        audio->deleteLater();
    }
}
