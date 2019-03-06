#ifndef RTSPDECODERPOOL_H
#define RTSPDECODERPOOL_H

#include <QObject>
#include <QMap>
#include "RtspDecoder.h"
#include "RtspPlayer.h"
#include "InputSignal.h"

class RtspDecoderPool : public QObject
{
    Q_OBJECT

public:
    explicit RtspDecoderPool(QObject *parent = nullptr);
    ~RtspDecoderPool();

    void addDecoder(int id, const QUrl &url);
    void initDecoder(const InputSignalList &list);

    Q_INVOKABLE inline RtspDecoder *getDecoder(int inputSignalId) const;

signals:

public slots:

private:
    QMap<int, RtspDecoder*> m_decoders;
};

RtspDecoder *RtspDecoderPool::getDecoder(int inputSignalId) const
{
    return m_decoders[inputSignalId];
}

#endif // RTSPDECODERPOOL_H
