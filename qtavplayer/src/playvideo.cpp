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
    int lent = mDecoder->getRenderListSize();
    if(lent >= 2){
        static  qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;
        qint64 currentTime  = mDecoder->requestRenderNextFrame();

        qint64 nextTime     = mDecoder->getNextFrameTime();
        int len = mDecoder->getRenderListSize(); //有一帧是处于显示的  不可用
        int space2 = nextTime - currentTime;

        if(len >= _cacheFrame){
            space = 5;
        }else if(len >= _cacheFrame - 1){
            space = space2 * 2 / 3 - space;
        }else{
            space = space2 - space + 1;
        }
        if(nextTime ==0){
            space= 1000 / _fps;
        }

        /*
        if(frameStep != 0){ // 帧步长知道
            int space2 = nextTime - currentTime;
            if(frameStep * 2 / 3 < space2 && space2 > frameStep * 3 / 2){  //根据时间搓播放
                if(len >= _cacheFrame - 1){
                    space = space2 * 2 / 3 - space;
                }else/* if(len >= 2) * /{
                    space = space2 - space + 1;
                }
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
        }else if(space > 50){
             space = 50;
        }
        if(len > _cacheFrame){
            space = 1;
        }




        */
        if(space <= 0){
            space = 1;
        }else if(space > 50){
             space = 50;
        }
//        if(len > _cacheFrame){
//            space = 1;
//        }

         qDebug() << "-------------------------------requestRender" << _cacheFrame << lent << len  <<  currentTime /*<< nextTime << nextTime - currentTime*/ << space;
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

        _cacheFrame = _cache / frameStep + 1;
        if(_cacheFrame < 4)
            _cacheFrame = 4;
    }
}
