#ifndef AVDECODER_H
#define AVDECODER_H
#include <QObject>
#include <QReadWriteLock>
#include "AVThread.h"
#include <QVector>
#include <QImage>
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

class RenderItem{
public :
    RenderItem()
        : pts(0)
        , valid (false)
        , isShowing(false)
    {
        frame = nullptr;
    }
    void clear(){
        if(frame != nullptr){
            delete frame;
            frame = nullptr;
        }
        pts = 0;
        valid = false;
        isShowing  = false;
    }

    ~RenderItem(){
        if(frame != nullptr){
            release();
        }
    }

    QImage* frame; //帧
    qint64 pts; //播放时间
    bool valid; //有效的
    bool isShowing; //显示中
    QMutex mutex;

private:
    void release(){
        if(frame != NULL){
            delete frame;
            frame = nullptr;
        }
        pts = 0;
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
public:
    explicit AVDecoder(QObject *parent = nullptr);

private:
    AVThread mProcessThread;

signals:
    void send_img(QImage);

private:
    bool mIsInit = false;
    bool mIsOpenVideoCodec = false;
    AVFormatContext *mFormatCtx = nullptr;
    QString mFilename; // = "udp://@227.70.80.90:2000";

    int  mVideoIndex;

    AVCodecContext *mVideoCodecCtx = nullptr; //mCodecCtx
    AVCodec *mVideoCodec = nullptr;           //mCodec
    AVPacket *mPacket = nullptr;
    AVPicture  mAVPicture;
//    VideoFrame vFrame;
    struct SwsContext *mVideoSwsCtx = nullptr; //视频参数转换上下文

    QVector<RenderItem *> mRenderList; //渲染队列
    QReadWriteLock mRenderListMutex; //队列操作锁
    int maxRenderListSize = 5; //渲染队列最大数量

    AVFrame *mFrame = nullptr;

    QSize mSize = QSize(0,0);

public :
    qint64 requestRenderNextFrame();

    void load();
    void setFilename(const QString &source);

    void init();
    void release(bool isDeleted = false);

    void decodec();
    void setFilenameImpl(const QString &source);

    int  getRenderListSize();
private :
    void initRenderList();

    void clearRenderList(bool isDelete = false);
    RenderItem *getInvalidRenderItem();
    RenderItem *getMinRenderItem();
};

#endif // AVDECODER_H
