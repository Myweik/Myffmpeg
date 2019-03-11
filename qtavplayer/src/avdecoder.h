#ifndef AVDECODER_H
#define AVDECODER_H
#include <QObject>
#include <QReadWriteLock>
#include "AVThread.h"
#include <QVector>
#include <QImage>
#include "AVDefine.h"
#include "AVMediaCallback.h"
#include <QDebug>

#include <QVideoFrame>

extern "C"
{

    #ifndef INT64_C
    #define INT64_C
    #define UINT64_C
    #endif

    #include <libavutil/time.h>

    #include <libavcodec/avcodec.h>
    #include <libavcodec/avfft.h>


    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>

    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavfilter/avfilter.h>

    #include <libavutil/opt.h>

    #include <libswresample/swresample.h>

    #include <libavutil/hwcontext.h>

    #ifdef LIBAVUTIL_VERSION_MAJOR
    #if (LIBAVUTIL_VERSION_MAJOR >= 57)
    #define SUPPORT_HW
    #endif
    #endif
}
class AVDecoder;
class RenderItem;
class PacketQueue;
struct VideoFormat{
    float width;
    float height;
    float rotate;
    int format;

    AVFrame *renderFrame;
    QMutex *renderFrameMutex;
};

class AVDecoder: public QObject
{
    Q_OBJECT
    friend class AVCodecTask;
public:
    explicit AVDecoder(QObject *parent = nullptr);
    ~AVDecoder();
    void setMediaCallback(AVMediaCallback *media);
    bool getmIsInitEC() {return mIsInitEC;}

public :
    qint64 requestRenderNextFrame();
    void load(); //初始化
    void setFilename(const QString &source);
    void rePlay(); //重新加载
    void saveTs(bool status = false);
    void saveImage();

    qint64 getNextFrameTime();
    int  getRenderListSize();

protected:
    void init();
    void getPacket();
    void decodec();
    void setFilenameImpl(const QString &source);
    void _rePlay();

signals:
    void frameSizeChanged(int width, int height);
    void newVideoFrame(const QVideoFrame &frame);
    void senderEncodecStatus(bool);
private:
    qint64 lastReadPacktTime = 0;
    int timeout = 5000;
    static int interrupt_cb(void *ctx)
    {
        AVDecoder* avDecoder = (AVDecoder* )ctx;
        if(av_gettime() - avDecoder->lastReadPacktTime > avDecoder->timeout * 1000)
        {
            return -1;
        }
        return 0;
    }

    /* 推流 */
    void initEncodec();
    int  OpenOutput(string outUrl);

    void getPacketTask();
    void decodecTask();

    void release(bool isDeleted = false);
    void statusChanged(AVDefine::AVMediaStatus);
    int decode_write(AVCodecContext *avctx, AVPacket *packet);
    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);

private :
    void initRenderList();
    void clearRenderList(bool isDelete = false);
    RenderItem *getInvalidRenderItem();
    RenderItem *getMinRenderItem();
    RenderItem *getShowRenderItem();
    /* 软解需要 */
    void changeRenderItemSize(int width,int height,AVPixelFormat format);

    /* fps统计 */
    void onFpsTimeout();
    /* 保存图片 */
    void saveFrame(RenderItem *render = nullptr);

private:
    QString outUrl;
    bool mIsInitEC = false;
    bool mIsInit = false;
    bool mIsOpenVideoCodec = false;
    QString mFilename; // = "udp://@227.70.80.90:2000";
    int  mVideoIndex;
    AVStream *mVideo = NULL;
    AVFormatContext *mFormatCtx = nullptr;
    AVCodecContext *mVideoCodecCtx = nullptr; //mCodecCtx
    AVCodec *mVideoCodec = nullptr;           //mCodec
    struct SwsContext *mVideoSwsCtx = nullptr; //视频参数转换上下文
    PacketQueue* videoq = nullptr;      //为解码的视频原始包
    QVector<RenderItem *> mRenderList;  //渲染队列
    QReadWriteLock mRenderListMutex;    //队列操作锁
    int maxRenderListSize = 15;         //渲染队列最大数量

    /* 资源锁 */
    QReadWriteLock mVideoCodecCtxMutex;

    //  ----- HW
    QList<AVCodecHWConfig *> mHWConfigList;
    bool                     mUseHw = false;
    /** 硬解上下文 */
    AVBufferRef *mHWDeviceCtx;
    /** 硬解格式 */
    enum AVPixelFormat mHWPixFormat;

    /** 保存TS流 */
    FILE *tsSave = nullptr;
    /** 保存Image */
    bool _isSaveImage = false;
    AVFrame *_frameRGB = nullptr;
    uint8_t *_out_buffer = nullptr;
    struct SwsContext *mRGBSwsCtx = nullptr; //RGB转码器 -- 保存图片用

private:
    QTimer      *_fpsTimer = nullptr; //帧率统计心跳
    uint        _fpsFrameSum = 0;

    uchar       _fps = 0;
    VideoFormat vFormat;
    QSize mSize = QSize(0,0);
    enum AVPixelFormat mPixFormat;  //原始格式格式
    AVThread mProcessThread;
    AVThread mDecodeThread;
    AVMediaCallback *mCallback = nullptr;
    AVDefine::AVMediaStatus mStatus = AVDefine::AVMediaStatus_UnknownStatus;
};

class AVCodecTask : public Task{
public :
    ~AVCodecTask(){}
    enum AVCodecTaskCommand{
        AVCodecTaskCommand_Init,
        AVCodecTaskCommand_SetPlayRate,
        AVCodecTaskCommand_Seek,
        AVCodecTaskCommand_GetPacket ,
        AVCodecTaskCommand_SetFileName ,
        AVCodecTaskCommand_DecodeToRender,
        AVCodecTaskCommand_SetDecodecMode,
        AVCodecTaskCommand_ShowFrameByPosition,
        AVCodecTaskCommand_RePlay,   //重新加载
        AVCodecTaskCommand_saveTS,   //保存TS流
        AVCodecTaskCommand_saveImage,//保存图片
    };
    AVCodecTask(AVDecoder *codec,AVCodecTaskCommand command,double param = 0,QString param2 = ""):
        mCodec(codec),command(command),param(param),param2(param2){}
protected :
    /** 现程实现 */
    virtual void run();
private :
    AVDecoder *mCodec;
    AVCodecTaskCommand command;
    double param;
    QString param2;
};

class RenderItem{
   friend class AVDecoder;
public :
    RenderItem()
        : pts(AV_NOPTS_VALUE)
        , valid (false)
        , isShowing(false)
    {

    }
    void clear(){
        pts = AV_NOPTS_VALUE;
        valid = false;
        isShowing  = false;
        width = 0;
        height = 0;
        format = 0;
    }

    ~RenderItem(){
        if(videoSize){
            release();
        }
    }

     /* 帧 */
    uint8_t*            videoData[4];
    int                 videoLineSize[4];
    int                 videoSize = 0;

    int                 format = 0;
    int                 width = 0;
    int                 height = 0;
    qint64              pts;        //播放时间

    bool        valid;      //有效的
    bool        isShowing;  //显示中
    QReadWriteLock mutex;   //读写锁

//private:
    void release(){
        if(videoSize > 0){
            av_freep(&videoData[0]);
            videoSize = 0;
        }
        clear();
    }
};

class PacketQueue{
public :
    PacketQueue(){init();}

    QList<AVPacket *> packets;
    AVRational time_base;

private :
    QReadWriteLock mutex;
public :

    void setTimeBase(AVRational &timebase){
        this->time_base.den = timebase.den;
        this->time_base.num = timebase.num;
    }

    void init(){
        release();
    }

    void put(AVPacket *pkt){
        if(pkt == NULL)
            return;
        mutex.lockForWrite();
        packets.push_back(pkt);
        mutex.unlock();
    }

    AVPacket *get(){
        AVPacket *pkt = NULL;
        mutex.lockForWrite();
        if(packets.size() > 0){
            pkt = packets.front();
            packets.pop_front();
        }
        mutex.unlock();
        return pkt;
    }

    void removeToTime(int time){
        mutex.lockForRead();
        QList<AVPacket *>::iterator begin = packets.begin();
        QList<AVPacket *>::iterator end = packets.end();
        while(begin != end){
            AVPacket *pkt = *begin;
            if(pkt != NULL){
                if(av_q2d(time_base) * pkt->pts * 1000 >= time || packets.size() == 1){
                    break;
                }
                av_packet_unref(pkt);
                av_freep(pkt);
            }
            packets.pop_front();
            begin = packets.begin();
        }
        mutex.unlock();
    }

    int diffTime(){
        int time = 0;
        mutex.lockForRead();
        if(packets.size() > 1){
            int start = av_q2d(time_base) * packets.front()->pts * 1000;
            int end = av_q2d(time_base) * packets.back()->pts * 1000;
            time = end - start;
        }
        mutex.unlock();
        return time;
    }

    int startTime(){
        int time = -1;
        mutex.lockForRead();
        if(packets.size() > 0){
            time = av_q2d(time_base) * packets.front()->pts * 1000;
        }
        mutex.unlock();
        return time;
    }

    int size(){
        mutex.lockForRead();
        int len = packets.size();
        mutex.unlock();
        return len;
    }

    void release(){
        mutex.lockForWrite();
        QList<AVPacket *>::iterator begin = packets.begin();
        QList<AVPacket *>::iterator end = packets.end();
        while(begin != end){
            AVPacket *pkt = *begin;
            if(pkt != NULL){
                av_packet_unref(pkt);
                av_freep(pkt);
            }
            packets.pop_front();
            begin = packets.begin();
        }
        mutex.unlock();
    }
};

#endif // AVDECODER_H
