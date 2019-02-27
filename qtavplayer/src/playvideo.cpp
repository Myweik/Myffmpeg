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
    if(lent >= 3){
        static  qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;
        qint64 currentTime  = mDecoder->requestRenderNextFrame();

        qint64 nextTime     = mDecoder->getNextFrameTime();
        int len = mDecoder->getRenderListSize(); //有一帧是处于显示的  不可用
        int space2 = nextTime - currentTime;

        if(space2 < 0)
            qDebug() << "---------===========<<<<<<<<<2" << space2 << len;
        if(space > 45)
            qDebug() << "---------===========<<<<<<<<<2" << space;

        if(len >= _cacheFrame){
            space = 5;
        }else if(len >= _cacheFrame - 1){
            space = space2 * 2 / 3 - space;
//        }else if(len >=  2){
//            space = space2 * 3 / 2 - space;
        }else{
            space = space2 - space + 1;
        }
        if(nextTime ==0){
            space= 1000 / _fps;
        }

        if(space <= 0){
            space = 1;
        }else if(space > 50){
             space = 50;
        }

        if(_fps < 29 && space + 3 <= 45 && len <= 3){
            space +=5;
        }

                if(len > _cacheFrame)
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
        if(_cacheFrame < 5)
            _cacheFrame = 5;
    }
}
