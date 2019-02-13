#include "playvideo.h"
#include <QQmlContext>
#include <QDateTime>
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

PlayVideo::PlayVideo(QObject *parent) : QObject(parent), mDecoder(new AVDecoder), mShowImage(new ShowImage)
{
    connect(mDecoder,&AVDecoder::send_img, mShowImage, &ShowImage::sendimage);
    wakeupPlayer();
}

PlayVideo::~PlayVideo()
{
    delete mDecoder;
    delete mShowImage;
}

void PlayVideo::setUrl(QString url)
{
    if(mDecoder)
        mDecoder->setFilename(url);
}

void PlayVideo::setQmlApplicationEngine(QQmlEngine &engine)
{
    engine.rootContext()->setContextProperty("videoImage",getShowImage());
    engine.rootContext()->setContextProperty("videoPlay",this);
    engine.addImageProvider(QLatin1String("videoImg"),getShowImage()->m_pImgProvider);

}

void PlayVideo::requestRender()
{
    /* 1-60 2->3 45  >3 30 */
    if(mDecoder->getRenderListSize() >= 2){
        static  qint64 lastTime = QDateTime::currentMSecsSinceEpoch();
        int space = QDateTime::currentMSecsSinceEpoch() - lastTime;

        qint64 currentTime  = mDecoder->requestRenderNextFrame();
//        qint64 nextTime     = mDecoder->getNextFrameTime();

        int len = mDecoder->getRenderListSize();
        if(len > 3){
//            if(nextTime > currentTime && currentTime != 0)
//                space = (nextTime - currentTime) / 100 - space ;
//            else
                space = 30 - space;

        }else if(len > 1){
            space = 45 - space;
        }else{
            space = 60 - space;
        }

        if(space <= 0){
            qDebug() << "----------------------space < 0" << len << space << mDecoder->requestRenderNextFrame() - currentTime;
            space = 1;
        }else{
            qDebug() << "================>>>>.space > 0" << len << space << mDecoder->requestRenderNextFrame() - currentTime;
        }

        mMutex.lock();
        mCondition.wait(&mMutex, space);
        mMutex.unlock();
        lastTime = QDateTime::currentMSecsSinceEpoch();
    }

//        qint64 nextTime = mDecoder->nextTime();
//        mMutex.lock();
//        mCondition.wait(&mMutex,nextTime - currentTime > 0 && nextTime - currentTime < 40 ? (nextTime - currentTime) : 20);
//        mMutex.unlock();
    wakeupPlayer();
}

void PlayVideo::wakeupPlayer()
{
    if(mDecoder){
        mThread.addTask(new AVPlayerTask(this,AVPlayerTask::AVPlayerTaskCommand_Render));
    }
}
