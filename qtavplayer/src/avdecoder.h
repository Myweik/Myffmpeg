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

struct VideoFormat{
    float width;
    float height;
    float rotate;
    int format;

    AVFrame *renderFrame;
    QMutex *renderFrameMutex;
};

class RenderItem{
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

class AVCodecTask : public Task{
public :
    ~AVCodecTask(){}
    enum AVCodecTaskCommand{
        AVCodecTaskCommand_Init,
        AVCodecTaskCommand_SetPlayRate,
        AVCodecTaskCommand_Seek,
        AVCodecTaskCommand_Decodec ,
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

class AVDecoder: public QObject
{
     Q_OBJECT
    friend class AVCodecTask;
public:
    explicit AVDecoder(QObject *parent = nullptr);
    void setMediaCallback(AVMediaCallback *media);

public :
    qint64 requestRenderNextFrame();
    void load();
    void setFilename(const QString &source);
    qint64 getNextFrameTime();
    int  getRenderListSize();

protected:
    void init();
    void decodec();
    void setFilenameImpl(const QString &source);

signals:
    void send_img(QImage);

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
    AVPicture  mAVPicture;
    struct SwsContext *mVideoSwsCtx = nullptr; //视频参数转换上下文

    QVector<RenderItem *> mRenderList; //渲染队列
    QReadWriteLock mRenderListMutex; //队列操作锁
    int maxRenderListSize = 15; //渲染队列最大数量

    //  ----- HW
    QList<AVCodecHWConfig *> mHWConfigList;
    bool                     mUseHw = false;
    /** 硬解上下文 */
    AVBufferRef *mHWDeviceCtx;
    /** 硬解格式 */
    enum AVPixelFormat mHWPixFormat;

private:
    VideoFormat vFormat;
    QSize mSize = QSize(0,0);
    AVThread mProcessThread;
    AVMediaCallback *mCallback = nullptr;
    AVDefine::AVMediaStatus mStatus = AVDefine::AVMediaStatus_UnknownStatus;

private:
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
};

#endif // AVDECODER_H
