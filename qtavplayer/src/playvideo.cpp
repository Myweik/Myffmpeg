#include "playvideo.h"
#include <QQmlContext>
#include <QDateTime>
#include <QDebug>
/** 现程实现 */
void AVPlayerTask::run(){
    switch(command){
    case AVPlayerTaskCommand_Render:
        mPlayer->requestRender();
        break;
    case AVPlayerTaskCommand_Seek:
//        mPlayer->seekImpl(param);
        break;
    case AVPlayerTaskCommand_SetPlayRate:
//        mPlayer->slotSetPlayRate(param);
        break;
    }
}

PlayVideo::PlayVideo(QObject *parent) : QObject(parent), mDecoder(new AVDecoder)
{
    mDecoder->setMediaCallback(this);
    wakeupPlayer();
}

PlayVideo::~PlayVideo()
{
    delete mDecoder;
}

void PlayVideo::setUrl(QString url)
{
    if(mDecoder)
        mDecoder->setFilename(url);
}

VideoFormat *PlayVideo::getRenderData()
{
     return mRenderData;
}

AVDefine::AVPlayState PlayVideo::getPlaybackState()
{
    return mPlaybackState;
}

void PlayVideo::requestRender()
{
    /* 1-60  >2 45  >3 30  >4 20  >5 15 */
    if(mDecoder->getRenderListSize() >= 1){
        static  qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;

        qint64 currentTime  = mDecoder->requestRenderNextFrame();
//        qint64 nextTime     = mDecoder->getNextFrameTime();

        int len = mDecoder->getRenderListSize();
        if(len > 7){
            space = 10 - space;
        }else if(len > 3){
            space = 19 - space;
//        }else if(len > 2){
//            space = 25 - space;
        }else if(len > 1){
            space = 35 - space;
        }else{
            space = 50 - space;
        }

        if(space <= 0){
            qDebug() << "----------------------space < 0" << len << space;
            space = 1;
        }else{
            qDebug() << "================>>>>.space > 0" << len << space;
        }

        mMutex.lock();
        mCondition.wait(&mMutex, space);
        mMutex.unlock();
        lastTime = QDateTime::currentMSecsSinceEpoch();
    }
    wakeupPlayer();
}

void PlayVideo::wakeupPlayer()
{
    if(mDecoder){
        mThread.addTask(new AVPlayerTask(this,AVPlayerTask::AVPlayerTaskCommand_Render));
    }
}

/* --------------------------------------[ 继承函数 ]----------------------------------- */
void PlayVideo::mediaUpdateVideoFrame(void *f)
{
    mRenderData = (VideoFormat *)f;
    emit updateVideoFrame(mRenderData);
}

void PlayVideo::mediaStatusChanged(AVDefine::AVMediaStatus)
{

}

void PlayVideo::mediaHasVideoChanged()
{

}
