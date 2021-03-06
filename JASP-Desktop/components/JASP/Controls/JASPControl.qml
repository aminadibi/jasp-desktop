import QtQuick 2.10
import JASP.Theme 1.0

FocusScope {
    id: jaspControl
    property string controlType: "JASPControl"
    property string name: ""
    property bool hasTabFocus: true
    property bool isBound: true
    property bool debug: false
    property var backgroundRectangle: Rectangle {
        color: debug ? Theme.debugBackgroundColor : Theme.analysisBackgroundColor
        border.width: 0
        border.color: Theme.analysisBackgroundColor
    }
    
    readonly property string iconFolder: "qrc:/icons/"
    
    Component.onCompleted: {
        if (typeof(DEBUG_MODE) !== "undefined" && !DEBUG_MODE && debug)
            visible = false;
    }
    
    states: [
        State {
            when: jaspControl.activeFocus && jaspControl.hasTabFocus
            PropertyChanges {
                target: backgroundRectangle
                border.width: 3
                border.color: Theme.focusBorderColor
                radius: 3
            }
        }
    ]
    
    transitions: [
        Transition {
            ParallelAnimation {
                NumberAnimation {
                    target: backgroundRectangle
                    properties: "border.width"; 
                    duration: 800; 
                    easing.type: Easing.OutElastic; 
                    easing.amplitude: 1.5;
                }
                ColorAnimation {
                    target: backgroundRectangle                    
                    duration: 100
                }
            }
        }
    ]
    
    
}
