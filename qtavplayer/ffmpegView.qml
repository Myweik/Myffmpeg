import QtQuick 2.5
import QtQuick.Window 2.2

Window {
    visible: true
    width: 960
    height: 540
    title: qsTr("Hello World")
    Image
    {
        id:video_image
        anchors.fill: parent
        fillMode:Image.PreserveAspectFit
    }
    Connections{
        target: videoImage;
        onCallQmlRefeshImg:
        {
//            console.log("playing----", "image://videoImg/"+Math.random())
            video_image.source = "image://videoImg/"+Math.random()
        }
    }
    Component.onCompleted: videoPlay.setUrl("udp://@227.70.80.90:2000")
}
