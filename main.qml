import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
Window {
    visible: true
    width: 960
    height: 540
    title: qsTr("Hello World")

    Loader {
        id: play
        anchors.fill:   parent
        asynchronous: true
        visible:      true
        source:      "qrc:/ffmpeg/ffmpegView.qml"
    }
        RowLayout {
              Button {
                  text: "Ok"
                  onClicked: play.visible = true
              }
              Button {
                  text: "Cancel"
                  onClicked: play.visible = false
              }

              Button {
                  text: "save TS"
                  onClicked: play.item.record = true
              }
              Button {
                  text: "Cancel Save"
                  onClicked: play.item.record = false
              }
              Button {
                  text: "Save Image"
                  onClicked: play.item.saveImage()
              }

          }

}
