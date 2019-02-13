#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QImage>
#include <QDebug>
#include <QDateTime>
#include <QFile>

#include <stdio.h>
#include <QApplication>
#include  "qtavplayer/src/playvideo.h"
#include "qtavplayer/src/AVOutput.h"

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");
    }

    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(msg).arg(current_date);

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();

    mutex.unlock();
}


int main(int argc, char *argv[])
{

#if defined(Q_OS_WIN)
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    //注册MessageHandler
//        qInstallMessageHandler(outputMessage);

    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

//    PlayVideo* play = new PlayVideo();
//    play->setQmlApplicationEngine(engine);

    AVOutput::registerPlugin();

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();

}



#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;
static FILE *output_file = NULL;
//static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
//{
//    int err = 0;
//    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
//                                      NULL, NULL, 0)) < 0) {
//        fprintf(stderr, "Failed to create specified HW device.\n");
//        return err;
//    }
//    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
//    return err;
//}
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }
    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}
static int decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL, *sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    uint8_t *buffer = NULL;
    int size;
    int ret = 0;
    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }
    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        if (frame->format == hw_pix_fmt) {
            /* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                fprintf(stderr, "Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
        } else
            tmp_frame = frame;
        size = av_image_get_buffer_size((AVPixelFormat)tmp_frame->format, tmp_frame->width,
                                        tmp_frame->height, 1);
        buffer = (uint8_t *)av_malloc(size);
        if (!buffer) {
            fprintf(stderr, "Can not alloc buffer\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = av_image_copy_to_buffer(buffer, size,
                                      (const uint8_t * const *)tmp_frame->data,
                                      (const int *)tmp_frame->linesize, (AVPixelFormat)tmp_frame->format,
                                      tmp_frame->width, tmp_frame->height, 1);
        if (ret < 0) {
            fprintf(stderr, "Can not copy image to buffer\n");
            goto fail;
        }
        if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
            fprintf(stderr, "Failed to dump raw data.\n");
            goto fail;
        }
    fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_freep(&buffer);
        if (ret < 0)
            return ret;
    }
}
//int main(int argc, char *argv[])
//{
//    AVFormatContext *input_ctx = NULL;
//    int video_stream, ret;
//    AVStream *video = NULL;
//    AVCodecContext *decoder_ctx = NULL;
//    AVCodec *decoder = NULL;
//    AVPacket packet;
//    enum AVHWDeviceType type;
//    int i;

//    const char *arg[4];
//    arg[1] = "dxva2";
//    arg[2] = "udp://@227.70.80.90:2000";
//    arg[3] = "E:/vedio.ts";

////    if (argc < 4) {
////        fprintf(stderr, "Usage: %s <device type> <input file> <output file>\n", argv[0]);
////        return -1;
////    }
//    type = av_hwdevice_find_type_by_name(arg[1]);
//    if (type == AV_HWDEVICE_TYPE_NONE) {
//        fprintf(stderr, "Device type %s is not supported.\n", arg[1]);
//        fprintf(stderr, "Available device types:");
//        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
//            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
//        fprintf(stderr, "\n");
//        return -1;
//    }
//    /* open the input file */
//    if (avformat_open_input(&input_ctx, arg[2], NULL, NULL) != 0) {
//        fprintf(stderr, "Cannot open input file '%s'\n", arg[2]);
//        return -1;
//    }
//    if (avformat_find_stream_info(input_ctx, NULL) < 0) {
//        fprintf(stderr, "Cannot find input stream information.\n");
//        return -1;
//    }
//    /* find the video stream information */
//    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
//    if (ret < 0) {
//        fprintf(stderr, "Cannot find a video stream in the input file\n");
//        return -1;
//    }
//    video_stream = ret;
//    for (i = 0;; i++) {
//        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
//        if (!config) {
//            fprintf(stderr, "Decoder %s does not support device type %s.\n",
//                    decoder->name, av_hwdevice_get_type_name(type));
//            return -1;
//        }
//        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
//            config->device_type == type) {
//            hw_pix_fmt = config->pix_fmt;
//            break;
//        }
//    }
//    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
//        return AVERROR(ENOMEM);
//    video = input_ctx->streams[video_stream];
//    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
//        return -1;
//    decoder_ctx->get_format  = get_hw_format;
//    if (hw_decoder_init(decoder_ctx, type) < 0)
//        return -1;
//    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
//        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
//        return -1;
//    }
//    /* open the file to dump raw data */
//    output_file = fopen(arg[3], "w+");
//    /* actual decoding and dump the raw data */
//    while (ret >= 0) {
//        if ((ret = av_read_frame(input_ctx, &packet)) < 0)
//            break;
//        if (video_stream == packet.stream_index)
//            ret = decode_write(decoder_ctx, &packet);
//        av_packet_unref(&packet);
//    }
//    /* flush the decoder */
//    packet.data = NULL;
//    packet.size = 0;
//    ret = decode_write(decoder_ctx, &packet);
//    av_packet_unref(&packet);
//    if (output_file)
//        fclose(output_file);
//    avcodec_free_context(&decoder_ctx);
//    avformat_close_input(&input_ctx);
//    av_buffer_unref(&hw_device_ctx);
//    return 0;
//}

