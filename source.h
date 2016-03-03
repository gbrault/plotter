#ifndef SIMULATEDSOURCE_H
#define SIMULATEDSOURCE_H

#include <qmath.h>
#include <qendian.h>
#include <QVector>
#include <QAudioFormat>

class Source
{
public:
    Source(int signalFreq, int duration, int amplitude, QAudioFormat &format);
    int displayCounter,audioCounter, displayTime, AudioTime;
    QVector<int> *signal;
    int signalFreq, duration, amplitude;
    void SineGenerator();
    QByteArray *signalAudio;
    QAudioFormat *format;
    int readDisplay(QVector<int> *signal, int duration);
    qint64 readAudio(QByteArray *data, int duration);
    int Value(char*ptr);
};

#endif // SIMULATEDSOURCE_H
