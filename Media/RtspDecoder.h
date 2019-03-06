#ifndef RTSPDECODER_H
#define RTSPDECODER_H

#include <QThread>
#include <QVideoFrame>
#include <QUrl>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class RtspDecoder : public QThread
{
    Q_OBJECT

public:
    explicit RtspDecoder(QObject *parent = nullptr);
    ~RtspDecoder();

    inline int width() const;
    inline int height() const;
    inline QSize frameSize() const;

    inline void setUrl(const QByteArray &url) { m_url = url; }

public slots:
    void play();
    void stop();

signals:
    void frameSizeChanged(int width, int height);
    void newVideoFrame(const QVideoFrame &frame);

protected:
    void run();

private:
    bool open();
    void free();

    QByteArray m_url;
    bool                m_isOpen = false;
    bool                m_playing = false;
    AVFormatContext*    m_fmtCtx = nullptr;
    AVCodecContext*     m_videoCtx = nullptr;
    AVDictionary*       m_opts = nullptr;
    int                 m_streamIndex = -1;
    AVFrame*            m_frame = nullptr;
    uint8_t*            m_videoData[4] = {nullptr};
    int                 m_videoLineSize[4];
    int                 m_videoSize;
};

int RtspDecoder::width() const
{
    return m_videoCtx ? m_videoCtx->width : 0;
}

int RtspDecoder::height() const
{
    return m_videoCtx ? m_videoCtx->height : 0;
}

QSize RtspDecoder::frameSize() const
{
    return QSize(width(), height());
}

#endif // RTSPDECODER_H
