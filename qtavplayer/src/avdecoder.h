#ifndef AVDECODER_H
#define AVDECODER_H
#include <QObject>
#include <QReadWriteLock>
#include "AVThread.h"
#include <QVector>
#include <QImage>
#include "AVDefine.h"
#include "AVMediaCallback.h"
extern "C"
{

    #ifndef INT64_C
    #define INT64_C
    #define UINT64_C
    #endif

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

public :
    qint64 requestRenderNextFrame();
    void load(); //初始化
    void setFilename(const QString &source);
    qint64 getNextFrameTime();
    int  getRenderListSize();

protected:
    void init();
    // +加一个获取队列  数据的
    void getPacket();
    void decodec();
    void setFilenameImpl(const QString &source);

private:
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

private:
    bool mIsInit = false;
    bool mIsOpenVideoCodec = false;
    AVFormatContext *mFormatCtx = nullptr;
    QString mFilename; // = "udp://@227.70.80.90:2000";
    int  mVideoIndex;
    AVStream *mVideo = NULL;
    AVCodecContext *mVideoCodecCtx = nullptr; //mCodecCtx
    AVCodec *mVideoCodec = nullptr;           //mCodec
    AVPacket *mPacket = nullptr;
    struct SwsContext *mVideoSwsCtx = nullptr; //视频参数转换上下文

    PacketQueue* videoq = nullptr;      //为解码的视频原始包
    QVector<RenderItem *> mRenderList;  //渲染队列
    QReadWriteLock mRenderListMutex;    //队列操作锁
    int maxRenderListSize = 15;         //渲染队列最大数量

    //  ----- HW
    QList<AVCodecHWConfig *> mHWConfigList;
    bool                     mUseHw = false;
    /** 硬解上下文 */
    AVBufferRef *mHWDeviceCtx;
    /** 硬解格式 */
    enum AVPixelFormat mHWPixFormat;

private:
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
        frame = av_frame_alloc();
    }
    void clear(){
        if(frame != nullptr){
            av_frame_unref(frame);
        }
        pts = AV_NOPTS_VALUE;
        valid = false;
        isShowing  = false;
    }

    ~RenderItem(){
        if(frame != nullptr){
            release();
        }
    }

    AVFrame*    frame;      //帧
    qint64      pts;        //播放时间
    bool        valid;      //有效的
    bool        isShowing;  //显示中
    QMutex      mutex;      //互斥锁

private:
    void release(){
        if(frame != NULL){
            av_frame_unref(frame);
            av_frame_free(&frame);
            frame = nullptr;
        }
        pts = AV_NOPTS_VALUE;
        valid = false;
        isShowing  = false;
    }
};

class PacketQueue{
public :
    PacketQueue(){init();}

    QList<AVPacket *> packets;
    AVRational time_base;

private :
    QMutex mutex;
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
        mutex.lock();
        packets.push_back(pkt);
        mutex.unlock();
    }

    AVPacket *get(){
        AVPacket *pkt = NULL;
        mutex.lock();
        if(packets.size() > 0){
            pkt = packets.front();
            packets.pop_front();
        }
        mutex.unlock();
        return pkt;
    }

    void removeToTime(int time){
        mutex.lock();
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
        mutex.lock();
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
        mutex.lock();
        if(packets.size() > 0){
            time = av_q2d(time_base) * packets.front()->pts * 1000;
        }
        mutex.unlock();
        return time;
    }

    int size(){
        mutex.lock();
        int len = packets.size();
        mutex.unlock();
        return len;
    }

    void release(){
        mutex.lock();
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
