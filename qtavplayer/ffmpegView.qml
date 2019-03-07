import QtQuick 2.5
import QtQuick.Window 2.2
import qtavplayer 1.0
import QtMultimedia 5.5

Rectangle {
    anchors.fill: parent
    color:        Qt.rgba(0,0,0,0.75)
    property string videoSource:   /*"rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov" //*/ "udp://@227.70.80.90:2000"



    property bool record: false  //保存视频流用接口

    onRecordChanged: {

        console.log("================================", record)

        avplayer.saveTs(false);
    }

//        property alias displayedPictures: vidEncoder.displayedPictures  //已显示帧
//        property alias lostPictures: vidEncoder.lostPictures  //丢失帧

//        function screenshot(){
//            vidEncoder.screenshot()
//        }


    onVisibleChanged: {
        if(!visible)
        {
            avplayer.setUrl("")
        }
        else{
            avplayer.setUrl(videoSource)
        }
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
