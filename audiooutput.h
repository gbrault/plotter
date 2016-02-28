#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QtCore>
#include <QtMultimedia>
#include "common.h"

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = 0);
    int ok;
    void createDevice();
    void configure(int channelCount, int sampleRate, int sampleSize, QAudioFormat::Endian byteOrder, QAudioFormat::SampleType sampleType , int buffersize);
    void close();

signals:

public slots:
    void writeData(QByteArray data);

private:
    QAudioOutput *audio;
    QIODevice *device;
    QAudioFormat format;
    int buffersize;

};

#endif // AUDIOOUTPUT_H
