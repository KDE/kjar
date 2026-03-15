// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include <QCoreApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QFile>
#include <KLocalizedString>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 1;
    }

    QString jarPath = QString::fromLocal8Bit(argv[1]);
    if (jarPath.startsWith(QStringLiteral("file://"))) {
        jarPath.remove(0, 7);
    }

    if (!QFile::exists(jarPath)) {
        return 1;
    }

    QStringList javaArgs;
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            javaArgs << QString::fromLocal8Bit(argv[i]);
        }
    } else {
        javaArgs << QStringLiteral("-jar") << jarPath;
    }

    int exitCode;
    QString errorText;

    {
        QCoreApplication coreApp(argc, argv);
        coreApp.setApplicationName(QStringLiteral("org.kde.kjar"));

        QProcess java;
        QByteArray stderrBuffer;

        java.setProcessChannelMode(QProcess::ForwardedOutputChannel);

        QObject::connect(&java, &QProcess::readyReadStandardError, [&]() {
            stderrBuffer += java.readAllStandardError();
        });

        java.start(QStringLiteral("/app/jdk/bin/java"), javaArgs);

        if (!java.waitForStarted()) {
            return 1;
        }

        java.waitForFinished(-1);

        exitCode = java.exitCode();
        errorText = QString::fromLocal8Bit(stderrBuffer).trimmed();
    }

    if (exitCode == 0 && errorText.isEmpty()) {
        return 0;
    }

    if (errorText.isEmpty()) {
        errorText = i18n("Java exited with an error (exit code %1).",
                         QString::number(exitCode));
    }

    QApplication guiApp(argc, argv);
    guiApp.setApplicationName(QStringLiteral("org.kde.kjar"));
    guiApp.setDesktopFileName(QStringLiteral("org.kde.kjar"));

    KLocalizedString::setApplicationDomain("org.kde.kjar");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("errorMessage"), errorText);
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:/watcher/ErrorDialog.qml")));

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return guiApp.exec();
}
