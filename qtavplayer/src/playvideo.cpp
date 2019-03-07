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

PlayVideo::PlayVideo(QObject *parent) : QObject(parent), mDecoder(new AVDecoder), _rtspPlayer(new RtspPlayer)
{
    mDecoder->setMediaCallback(this);
    wakeupPlayer();

    _rtspPlayer->setDecoder(mDecoder);
}

PlayVideo::~PlayVideo()
{
    delete mDecoder;
    delete _rtspPlayer;
}

void PlayVideo::setUrl(QString url)
{
    if(mDecoder)
        mDecoder->setFilename(url);
}

void PlayVideo::saveTs(bool ok)
{
    if(mDecoder)
        mDecoder->saveTs(ok);
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
        qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        qint64 currentTime  = mDecoder->requestRenderNextFrame();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;
        qint64 nextTime     = mDecoder->getNextFrameTime();
        int len = mDecoder->getRenderListSize(); //有一帧是处于显示的  不可用
        static int space2 = 0;
        space2  = nextTime > currentTime ? nextTime - currentTime : space2;

        if(len >= _cacheFrame){
            space = _frameStep / 10 + 1;
        }else if(len >= _cacheFrame - 2){
            space = space2 - space - _frameStep / 10 -1;;
        }else{
            space = space2 - space + _frameStep / 10 +1;
        }

        if(space <= 0){
            space = 1;
        }else if(space > 50){
            space = 50;
        }

        if(_fps < 29 && space + 3 <= 45 && len <= 2){
            space += _frameStep / 10;
        }
//        if(len > _cacheFrame)
//            qDebug() << "-------------------------------requestRender" << _cacheFrame << _frameStep << lent << len  <<  currentTime /*<< nextTime << nextTime - currentTime*/ << space;
        mMutex.lock();
        mCondition.wait(&mMutex, space);
        mMutex.unlock();

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
//    qDebug() << "---------------------------mediaUpdateFps fps"  << fps;
    _fps = fps;
    if(_fps != 0 &&_fps <= 80){
        _frameStep = 1000 / _fps;

        _cacheFrame = _cache / _frameStep + 1;
        if(_cacheFrame < 5)
            _cacheFrame = 5;
    }
}

uchar PlayVideo::mediaGetFps()
{
    return _fps;
}
