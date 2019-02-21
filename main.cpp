#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include <QImage>
#include <QDebug>
#include <QDateTime>
#include <QFile>

#include <stdio.h>
#include <QApplication>
#include  "qtavplayer/src/playvideo.h"
#include "qtavplayer/src/AVOutput.h"

#include "ffmpeg.h"

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    mutex.lock();

    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;

    case QtWarningMsg:
        text = QString("Warning:");
        break;

    case QtCriticalMsg:
        text = QString("Critical:");
        break;

    case QtFatalMsg:
        text = QString("Fatal:");
    }

    QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(msg).arg(current_date);

    QFile file("log.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();

    mutex.unlock();
}


int main(int argc, char *argv[])
{

    #if defined(Q_OS_WIN)
    //    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif

        //注册MessageHandler
    //        qInstallMessageHandler(outputMessage);

        QApplication app(argc, argv);
        QQmlApplicationEngine engine;

    //    PlayVideo* play = new PlayVideo();
    //    play->setQmlApplicationEngine(engine);

        AVOutput::registerPlugin();

        engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

        if (engine.rootObjects().isEmpty())
            return -1;
        return app.exec();


//    av_register_all();
//    avformat_network_init();

//    string str = INPUTURL;
//    qDebug("--------INIT INPUT----w------\n");
//    int ret = InitInput(str.c_str(),&icodec,&ifmt);
//    qDebug() << "--------INIT INPUT----------\n" << ret;
//    ret = InitOutput();
//    qDebug() << "--------程序运行开始----------\n" << ret;
//    //////////////////////////////////////////////////////////////////////////
//    Es2Mux_2();
//    //////////////////////////////////////////////////////////////////////////
//    UintOutput();
//    UintInput();
//    qDebug("--------程序运行结束----------\n");
//    printf("-------请按任意键退出---------\n");
//    return getchar();
}






