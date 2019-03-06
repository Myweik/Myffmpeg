#include "RtspDecoder.h"
#include "VideoBuffer.h"
#include <QDateTime>
#include <QDebug>

RtspDecoder::RtspDecoder(QObject *parent)
    : QThread(parent)
{
    avformat_network_init();
}

RtspDecoder::~RtspDecoder()
{
    stop();
    free();
}

bool RtspDecoder::open()
{
    /* open input file, and allocate format context */
    av_dict_set(&m_opts, "stimeout", "10000000", 0);
    av_dict_set(&m_opts, "rtsp_transport", "tcp", 0);
    if (avformat_open_input(&m_fmtCtx, m_url.constData(), nullptr, &m_opts) < 0) {
        qWarning("Could not open source file %s.", m_url.constData());
        return false;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0) {
        qWarning("Could not find stream information.");
        return false;
    }

    m_streamIndex = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_streamIndex < 0) {
        qWarning("Could not find stream in input file.");
        return false;
    }

    AVStream *videoStream = m_fmtCtx->streams[m_streamIndex];

    /* find decoder for the stream */
    AVCodec *dec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (!dec) {
        qWarning("Failed to find codec.");
        return false;
    }

    /* Allocate a codec context for the decoder */
    m_videoCtx = avcodec_alloc_context3(dec);
    if (!m_videoCtx) {
        qWarning("Failed to allocate the codec context.");
        return false;
    }

    /* Copy codec parameters from input stream to output codec context */
    if (avcodec_parameters_to_context(m_videoCtx, videoStream->codecpar) < 0) {
        qWarning("Failed to copy codec parameters to decoder context.");
        free();
        return false;
    }

    /* Init the decoders, with or without reference counting */
    if (avcodec_open2(m_videoCtx, dec, nullptr) < 0) {
        qWarning("Failed to open codec.");
        free();
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        qWarning("Could not allocate frame.");
        free();
        return false;
    }

    m_videoSize = av_image_alloc(m_videoData, m_videoLineSize,
                                width(), height(), m_videoCtx->pix_fmt, 1);
    if (m_videoSize < 0) {
        qWarning("Could not allocate raw video buffer.");
        free();
        return false;
    }

    emit frameSizeChanged(width(), height());
    m_isOpen = true;

    return true;
}

void RtspDecoder::play()
{
    if (isRunning())
        return ;

    m_playing = true;
    start();
}

void RtspDecoder::stop()
{
    m_playing = false;
    wait(); // wait thread exit
}

void RtspDecoder::run()
{
    if (!m_isOpen)
    {
        if (!open())
            return ;
    }

    AVPacket pkt;

    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    while (av_read_frame(m_fmtCtx, &pkt) >= 0)
    {
        if (!m_playing)
            break;

        if(pkt.stream_index != m_streamIndex)
            continue;

        if (avcodec_send_packet(m_videoCtx, &pkt) < 0) {
            qWarning("Error sending a packet for decoding.");
            continue;
        }

        if (avcodec_receive_frame(m_videoCtx, m_frame) >= 0)
        {
            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(m_videoData, m_videoLineSize,
                          const_cast<const uint8_t **>(m_frame->data), m_frame->linesize,
                          m_videoCtx->pix_fmt, width(), height());

            VideoBuffer *buffer = new VideoBuffer(m_videoData[0], m_videoSize, m_videoLineSize[0]);

            emit newVideoFrame(QVideoFrame(buffer, frameSize(), QVideoFrame::Format_YUV420P));
        }

        av_packet_unref(&pkt);
    }
}

void RtspDecoder::free()
{
    avcodec_close(m_videoCtx);
    avformat_close_input(&m_fmtCtx);

    if (m_videoCtx)
        avcodec_free_context(&m_videoCtx);

    if (m_frame)
        av_frame_free(&m_frame);

    if (m_opts)
        av_dict_free(&m_opts);
}
