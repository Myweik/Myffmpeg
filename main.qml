import QtQuick 2.5
import QtQuick.Window 2.2

Window {
    visible: true
    width: 960
    height: 540
    title: qsTr("Hello World")

    Loader {
        anchors.fill:   parent
        asynchronous: true
        visible:      true
        source:      "qrc:/ffmpeg/ffmpegView.qml"
    }
}
