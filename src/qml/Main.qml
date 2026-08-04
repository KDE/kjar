// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

import QtCore
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.coreaddons as KCoreAddons
import org.kde.kirigami as Kirigami
import org.kde.kjar

Kirigami.ApplicationWindow {
    id: root

    required property string initialError

    width: 520
    height: 410
    minimumWidth: 480
    minimumHeight: 380
    visible: true
    title: i18n("Java Archive Runner")

    function showStatus(ok: bool, text: string): void {
        statusMessage.type = ok ? Kirigami.MessageType.Positive : Kirigami.MessageType.Error;
        statusMessage.text = text;
        statusMessage.visible = true;
    }

    Component.onCompleted: {
        if (root.initialError !== "") {
            root.showStatus(false, root.initialError);
        }
    }

    Component {
        id: aboutPage

        Kirigami.AboutPage {
            aboutData: KCoreAddons.AboutData
        }
    }

    Kirigami.Dialog {
        id: toolsDialog

        title: i18n("Available JDK Tools")
        preferredWidth: 500
        preferredHeight: 390

        Controls.ScrollView {
            clip: true

            Flow {
                width: toolsDialog.preferredWidth - Kirigami.Units.largeSpacing * 4
                spacing: Kirigami.Units.smallSpacing
                padding: Kirigami.Units.largeSpacing

                Repeater {
                    model: KjarApp.availableTools

                    delegate: Controls.Label {
                        required property string modelData

                        text: modelData
                        font.family: "monospace"
                        padding: Kirigami.Units.smallSpacing
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
        title: i18n("Java Archive Runner")

        actions: [
            Kirigami.Action {
                icon.name: "application-x-java-archive"
                text: i18n("Run JAR")
                displayHint: Kirigami.DisplayHint.KeepVisible
                onTriggered: fileDialog.open()
            },
            Kirigami.Action {
                icon.name: "configure"
                text: i18n("Advanced")

                Kirigami.Action {
                    text: i18n("Show Available Tools")
                    icon.name: "utilities-terminal"
                    onTriggered: toolsDialog.open()
                }

                Kirigami.Action {
                    text: i18n("Generate Wrappers")
                    icon.name: "archive-insert"
                    onTriggered: {
                        const result = KjarApp.generateWrappers();
                        root.showStatus(result.ok, result.message);
                    }
                }

                Kirigami.Action {
                    text: i18n("Remove Wrappers")
                    icon.name: "edit-delete"
                    onTriggered: {
                        const result = KjarApp.removeWrappers();
                        root.showStatus(result.ok, result.message);
                    }
                }

                Kirigami.Action {
                    text: i18n("Open Modules Folder")
                    icon.name: "folder"
                    onTriggered: KjarApp.openModulesFolder()
                }
            },
            Kirigami.Action {
                icon.name: "help-about"
                text: i18n("About")
                onTriggered: root.pageStack.layers.push(aboutPage)
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

            Item {
                Layout.fillHeight: true
            }

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
                text: i18n("Select a Java archive (.jar) to run it with the bundled OpenJDK runtime.")
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

            Item {
                Layout.fillHeight: true
            }
        }
    }

    FileDialog {
        id: fileDialog

        title: i18n("Select JAR File")
        nameFilters: [i18n("Java archives (*.jar)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DownloadLocation)

        onAccepted: {
            const error = KjarApp.runJar(selectedFile);
            if (error !== "") {
                root.showStatus(false, error);
            }
        }
    }
}
