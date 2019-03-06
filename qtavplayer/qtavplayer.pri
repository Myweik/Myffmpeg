QT += multimedia

SOURCES += \
    $$PWD/src/avdecoder.cpp \
    $$PWD/src/AVThread.cpp \
    $$PWD/src/playvideo.cpp \
    $$PWD/src/AVOutput.cpp \
    $$PWD/src/RtspPlayer.cpp \
    $$PWD/src/VideoBuffer.cpp
HEADERS += \
    $$PWD/src/avdecoder.h \
    $$PWD/src/AVThread.h \
    $$PWD/src/playvideo.h \
    $$PWD/src/AVDefine.h \
    $$PWD/src/AVOutput.h  \
    $$PWD/src/AVMediaCallback.h \
    $$PWD/src/RtspPlayer.h \
    $$PWD/src/VideoBuffer.h

RESOURCES += \
    $$PWD/ffmpegqml.qrc \
    $$PWD/qtavplayer.qrc

win32{
    LIBS += -L$$PWD/libs/lib/win32/ -lavcodec -lavfilter -lavformat -lavutil -lswresample -lswscale
}

win32:!win32-g++{
        PRE_TARGETDEPS += \
            $$PWD/libs/lib/win32/avcodec.lib \
$$PWD/libs/lib/win32/avfilter.lib \
$$PWD/libs/lib/win32/avformat.lib \
$$PWD/libs/lib/win32/avutil.lib \
$$PWD/libs/lib/win32/swresample.lib \
$$PWD/libs/lib/win32/swscale.lib \

    }
else:win32-g++{
        PRE_TARGETDEPS += \
            $$PWD/libs/lib/win32/avcodec.a \
$$PWD/libs/lib/win32/avfilter.a \
$$PWD/libs/lib/win32/avformat.a \
$$PWD/libs/lib/win32/avutil.a \
$$PWD/libs/lib/win32/swresample.a \
$$PWD/libs/lib/win32/swscale.a \

    }

INCLUDEPATH += $$PWD/libs/include
DEPENDPATH += $$PWD/libs/include
