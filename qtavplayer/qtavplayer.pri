QT += multimedia quick qml core
CONFIG += c++11

SOURCES += \
    $$PWD/src/avdecoder.cpp \
    $$PWD/src/AVThread.cpp \
    $$PWD/src/QmlVideoObject.cpp \
    $$PWD/src/QmlVideoPlayer.cpp \
    $$PWD/src/painter/GlPainter.cpp \
    $$PWD/src/painter/GlslPainter.cpp \
    $$PWD/src/Enums.cpp

HEADERS += \
    $$PWD/src/avdecoder.h \
    $$PWD/src/AVThread.h \
    $$PWD/src/AVDefine.h \
    $$PWD/src/AVMediaCallback.h \
    $$PWD/src/QmlVideoObject.h \
    $$PWD/src/QmlVideoPlayer.h \
    $$PWD/src/painter/GlPainter.h \
    $$PWD/src/painter/GlslPainter.h \
    $$PWD/src/Enums.h



RESOURCES += \
    $$PWD/ffmpegqml.qrc \

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
