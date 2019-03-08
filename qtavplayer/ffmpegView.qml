import QtQuick 2.5
import QtQuick.Window 2.2
import qtavplayer 1.0
import QtMultimedia 5.5

//import MMCSettings  1.0
Rectangle {
    anchors.fill: parent
    color:        Qt.rgba(0,0,0,0.75)
    property string videoSource:  "udp://@227.70.80.90:2000" //  /*"rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov" //*/  MMCSettings.value("video/videoUrl","udp://@227.70.80.90:2000") /*"udp://@227.70.80.90:2000"*/

    property bool record: false  //保存视频流用接口
    onRecordChanged: {
        avplayer.saveTs(record);
    }

    onVisibleChanged: {
        if(!visible)
        {
            avplayer.setUrl("")
        }
        else{
            avplayer.setUrl(videoSource)
        }
    }

    Text {
        z:1
        anchors.top: parent.top
        anchors.right: parent.right
        color: avplayer.encodecStatus ? "#0f0" : "#900"
        text: avplayer.encodecStatus ? "RTMP push OK ": "RTMP push NG"
    }

    VideoOutput {
             source: avplayer.rtspPlayer
             anchors.fill: parent
//             focus : visible // to receive focus and capture key events when visible
         }

//    AVOutput{ //可以同时在多个窗口上播放一个视频
//        anchors.fill: parent
//        source: avplayer
//    }

    AVPlayer{
        id : avplayer
    }
     Component.onCompleted: avplayer.setUrl(videoSource)
}
