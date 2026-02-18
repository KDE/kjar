// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Qt.labs.platform
import QtCore

Kirigami.ApplicationWindow {
    id: root
    width: 460
    height: 340
    minimumWidth: 380
    minimumHeight: 280
    visible: true
    title: i18n("KJar")

    Component.onCompleted: {
        if (typeof initialError !== "undefined" && initialError !== "") {
            statusMessage.type = Kirigami.MessageType.Error
            statusMessage.text = initialError
            statusMessage.visible = true
        }
    }

    // Tools dialog
    Kirigami.Dialog {
        id: toolsDialog
        title: i18n("Available JDK Tools")
        preferredWidth: 420
        preferredHeight: 360

        Controls.ScrollView {
            clip: true

            Flow {
                width: toolsDialog.preferredWidth - Kirigami.Units.largeSpacing * 4
                spacing: Kirigami.Units.smallSpacing
                padding: Kirigami.Units.largeSpacing

                Repeater {
                    model: backend ? backend.availableTools : []
                    delegate: Controls.Label {
                        text: modelData
                        font.family: "monospace"
                        font.pixelSize: 11
                        padding: 4
                        background: Rectangle {
                            color: Kirigami.Theme.alternateBackgroundColor
                            radius: 3
                        }
                    }
                }
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        title: i18n("KJar – Java Archive Runner")

        actions: [
            Kirigami.Action {
                icon.name: "application-x-java-archive"
                text: i18n("Run JAR")
                onTriggered: fileDialog.open()
                displayHint: Kirigami.DisplayHint.KeepVisible
            },
            Kirigami.Action {
                icon.name: "configure"
                text: i18n("Advanced")
                children: [
                    Kirigami.Action {
                        text: i18n("Show Available Tools")
                        icon.name: "utilities-terminal"
                        onTriggered: toolsDialog.open()
                    },
                    Kirigami.Action {
                        text: i18n("Generate Wrappers")
                        icon.name: "archive-insert"
                        enabled: backend ? !backend.busy : false
                        onTriggered: backend.generateWrappers()
                    }
                ]
            }
        ]

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.InlineMessage {
                id: statusMessage
                Layout.fillWidth: true
                visible: false
                showCloseButton: true
            }

            Controls.BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: backend ? backend.busy : false
                visible: backend ? backend.busy : false
            }

            Item { Layout.fillHeight: true }

            Kirigami.Icon {
                source: "application-x-java-archive"
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: Kirigami.Units.iconSizes.enormous
                implicitHeight: Kirigami.Units.iconSizes.enormous
            }

            Kirigami.Heading {
                text: i18n("Run a JAR File")
                level: 2
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
            }

            Controls.Label {
                text: i18n("Select a Java Archive (.jar) file to execute it using the bundled OpenJDK runtime.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.7
            }

            Controls.Button {
                text: i18n("Select JAR File…")
                icon.name: "document-open"
                Layout.fillWidth: true
                onClicked: fileDialog.open()
            }

            Item { Layout.fillHeight: true }
        }
    }

    FileDialog {
        id: fileDialog
        title: i18n("Select JAR File")
        nameFilters: ["JAR files (*.jar)"]
        folder: StandardPaths.writableLocation(StandardPaths.DownloadLocation)

        onAccepted: {
            let filePath = file.toString().replace("file://", "")
            backend.runJarFile(filePath)
        }
    }

    Connections {
        target: backend
        function onErrorOccurred(error) {
            statusMessage.type = Kirigami.MessageType.Error
            statusMessage.text = error
            statusMessage.visible = true
        }
        function onOperationCompleted(message) {
            statusMessage.type = Kirigami.MessageType.Positive
            statusMessage.text = message
            statusMessage.visible = true
        }
    }
}
