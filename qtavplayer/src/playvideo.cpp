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
    if(mDecoder->getRenderListSize() >= 3){
        static  qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;
        qint64 currentTime  = mDecoder->requestRenderNextFrame();
        qint64 nextTime     = mDecoder->getNextFrameTime();
        int len = mDecoder->getRenderListSize(); //有一帧是处于显示的  不可用

        qDebug() << "-------------------------------requestRender" << len  <<  currentTime << nextTime << nextTime - currentTime;

        if(frameStep != 0){ // 帧步长知道
            int space2 = nextTime - currentTime;
            if(frameStep * 2 / 3 < space2 && space2 > frameStep * 3 / 2){  //根据时间搓播放
                if(len > 6){
                    space = space2 * 2 / 3 - space + 2;
                }else/* if(len >= 2)*/{
                    space = space2 - space;
                }/*else{
//                    space = space2 * 3 / 2  - space;
                }*/
            }else{                                                          //根据fps推算播放
                if(len > 5){
                    space = frameStep * 2 / 3 - space;
                }else if(len >= 2){
                    space = frameStep - space + 2;
                }else{
                    space = frameStep * 3 / 2  - space;
                }
            }
        }else{  //fps无效时                                                //根据缓存长度播放
            if(len > 7){
                space = 10 - space;
            }else if(len > 2){
                space = 19 - space;
            }else if(len > 1){
                space = 35 - space;
            }else{
                space = 50 - space;
            }
        }

        if(space <= 0){
            space = 1;
        }
//        if(len <= 1 || len >4)
//            qDebug() << "================>>>>.space > 0" << len << space;

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

void PlayVideo::mediaUpdateFps(uchar fps)
{
    qDebug() << "---------------------------mediaUpdateFps fps"  << fps;
    _fps = fps;
    if(_fps != 0 &&_fps <= 80){
        frameStep = 1000 / _fps;
    }
}
