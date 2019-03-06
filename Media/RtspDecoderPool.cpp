#include "RtspDecoderPool.h"
#include <QQmlApplicationEngine>
#include <QDebug>

RtspDecoderPool::RtspDecoderPool(QObject *parent)
    : QObject(parent)
{
    qmlRegisterType<RtspDecoder>("Media", 1, 0, "RtspDecoder");
    qmlRegisterType<RtspPlayer>("Media", 1, 0, "RtspPlayer");
}

RtspDecoderPool::~RtspDecoderPool()
{

}

void RtspDecoderPool::addDecoder(int id, const QUrl &url)
{
    RtspDecoder *decoder = new RtspDecoder(this);
    decoder->setUrl(url.toString().toUtf8());

    m_decoders.insert(id, decoder);
}

void RtspDecoderPool::initDecoder(const InputSignalList &list)
{
    foreach (const auto &model, list) {
        RtspDecoder *decoder = new RtspDecoder(this);
        decoder->setUrl(model.url().toString().toUtf8());
        m_decoders.insert(model.id(), decoder);
    }
}
