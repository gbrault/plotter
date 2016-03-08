#ifndef SIMULATEDSOURCE_H
#define SIMULATEDSOURCE_H

#include <qmath.h>
#include <qendian.h>
#include <QVector>
#include <QAudioFormat>
#include <QQueue>
#include <QStack>

class Source
{
public:
    Source(QAudioFormat &format);
    Source(int signalFreq, int duration, int amplitude, QAudioFormat &format);
    int displayCounter,audioCounter, displayTime, AudioTime;
    QVector<int> *signal;
    int signalFreq, duration, amplitude;
    void SineGenerator();
    QByteArray *signalAudio;
    QAudioFormat *format;
    int readPeriodic(QVector<int> *signal, int duration);
    qint64 readPeriodicAudio(QByteArray *data, int duration);
    void fromAudio();
    void toAudio();
    QQueue<int> *qsignal;
    QQueue<unsigned char> *qsignalaudio;
    void EnQueue(int value, bool audio);
    int DeQueue(QVector<int> *signal, int duration);
    int DeQueueAudio(QByteArray *signal, int duration);
};

#endif // SIMULATEDSOURCE_H
