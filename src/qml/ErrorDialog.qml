// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    visible: true
    width: 640
    height: 560
    title: i18n("Java Application Error")

    pageStack.initialPage: Kirigami.Page {
        title: i18n("Java Application Error")

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: "dialog-error"
                    implicitWidth: Kirigami.Units.iconSizes.large
                    implicitHeight: Kirigami.Units.iconSizes.large
                }

                Kirigami.Heading {
                    level: 1
                    text: i18n("Java Application Error")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Kirigami.Heading {
                level: 2
                text: i18n("Error details:")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Kirigami.Theme.inherit: false
                Kirigami.Theme.colorSet: Kirigami.Theme.View

                TextArea {
                    text: errorMessage
                    readOnly: true
                    wrapMode: Text.WrapAnywhere
                    font.family: "monospace"
                    selectByMouse: true
                    background: Rectangle {
                        color: Kirigami.Theme.backgroundColor
                        border.color: Kirigami.Theme.disabledTextColor
                        border.width: 1
                        radius: 3
                    }
                }
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: true
                type: Kirigami.MessageType.Information
                text: i18n("If this application requires external modules or libraries, place them in <b>~/.local/share/kjar/modules/</b> and try again.")
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: i18n("Close")
                icon.name: "dialog-close"
                onClicked: Qt.quit()
            }
        }
    }
}
