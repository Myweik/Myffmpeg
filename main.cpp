#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QImage>
#include <QDebug>
#include <QDateTime>
//extern "C"{
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libavutil/pixfmt.h"
//#include "libswscale/swscale.h"
//}

#include <stdio.h>
//#include "imageprovider.h"
#include <QApplication>

//#include "controlmanager.h"

#include  "qtavplayer/src/playvideo.h"

int main(int argc, char *argv[])
{

#if defined(Q_OS_WIN)
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    PlayVideo* play = new PlayVideo();
    play->setQmlApplicationEngine(engine);
    engine.load(QUrl(QStringLiteral("qrc:/ffmpeg/ffmpegView.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();



}



