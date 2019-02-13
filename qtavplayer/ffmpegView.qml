import QtQuick 2.5
import QtQuick.Window 2.2
import qtavplayer 1.0

Rectangle {
    anchors.fill: parent
    color:        Qt.rgba(0,0,0,0.75)

    AVOutput{ //可以同时在多个窗口上播放一个视频
        anchors.fill: parent
        source: avplayer
    }
    AVPlayer{
        id : avplayer
    }
     Component.onCompleted: avplayer.setUrl("udp://@227.70.80.90:2000")
}
