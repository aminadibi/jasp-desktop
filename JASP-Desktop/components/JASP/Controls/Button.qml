//
// Copyright (C) 2013-2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public
// License along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import JASP.Theme 1.0


Button {
    id: button

    implicitWidth: height
    width: 50
    height: 30

    background: Rectangle {
        id: rectangle
        border.width: 1
        border.color: Theme.borderColor
        radius: Theme.borderRadius
        color: Theme.controlBackgroundColor
        
        Behavior on color {        
            ColorAnimation {
                duration: 500
            }
        }
        
    }
    
    states: [
        State {
            name: "disabled"
            PropertyChanges { target: rectangle; color: Theme.disableControlBackgroundColor }
        }
    ]
}
