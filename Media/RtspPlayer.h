#ifndef RTSPPLAYER_H
#define RTSPPLAYER_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QList>
#include "RtspDecoder.h"

class RtspPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)
    Q_PROPERTY(RtspDecoder *decoder READ decoder WRITE setDecoder NOTIFY decoderChanged)
    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)

public:
    explicit RtspPlayer(QObject *parent = nullptr);
    ~RtspPlayer();

    inline QAbstractVideoSurface *videoSurface() { return m_surface; }
    void setVideoSurface(QAbstractVideoSurface *surface);

    inline RtspDecoder *decoder() const { return m_decoder; }
    void setDecoder(RtspDecoder *decoder);

    inline int width() const;
    inline int height() const;

signals:
    void decoderChanged(RtspDecoder *decoder);

    void widthChanged(int width);
    void heightChanged(int height);

public slots:
    void onFrameSizeChanged(int width, int height);
    void onNewVideoFrameReceived(const QVideoFrame &frame);

protected:
    void setWidth(int width);
    void setHeight(int height);
    void setSize(int width, int height);

private:
    QVideoSurfaceFormat m_format;
    QAbstractVideoSurface *m_surface;
    RtspDecoder *m_decoder;
};

int RtspPlayer::width() const
{
    return m_format.frameSize().width();
}

int RtspPlayer::height() const
{
    return m_format.frameSize().height();
}

#endif // RTSPPLAYER_H
