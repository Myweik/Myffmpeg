#ifndef PLAYVIDEO_H
#define PLAYVIDEO_H

#include <QObject>
#include <QWaitCondition>
#include "avdecoder.h"
#include "imageprovider.h"
#include <QQmlApplicationEngine>

class AVPlayerTask;

class PlayVideo : public QObject
{
    Q_OBJECT
public:
    explicit PlayVideo(QObject *parent = nullptr);
    ~PlayVideo();
    ShowImage* getShowImage() { return mShowImage; }

    Q_INVOKABLE void setUrl(QString url);

    void  setQmlApplicationEngine(QQmlEngine &engine);
public :
    void requestRender();
signals:

public slots:

private:
    void wakeupPlayer();

    QWaitCondition mCondition;
    QMutex mMutex;

    AVDecoder *mDecoder;
    ShowImage *mShowImage;
    AVThread mThread;
};

class AVPlayerTask : public Task{
public :
    ~AVPlayerTask(){}
    enum AVPlayerTaskCommand{
        AVPlayerTaskCommand_Render,
        AVPlayerTaskCommand_Seek ,
        AVPlayerTaskCommand_SetPlayRate
    };
    AVPlayerTask(PlayVideo *player,AVPlayerTaskCommand command,double param = 0):
        mPlayer(player),command(command),param(param){this->type = (int)command;}
protected :
    /** 线程实现 */
    virtual void run();
private :
    PlayVideo *mPlayer;
    AVPlayerTaskCommand command;
    double param;
};

#endif // PLAYVIDEO_H
