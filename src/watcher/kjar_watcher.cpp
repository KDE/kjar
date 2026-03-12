// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QFile>
#include <QDebug>
#include <KLocalizedString>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("org.kde.kjar"));
    app.setDesktopFileName(QStringLiteral("org.kde.kjar"));

    if (argc < 2)
        return 1;

    QString jarPath = QString::fromLocal8Bit(argv[1]);

    if (jarPath.startsWith(QStringLiteral("file://")))
        jarPath.remove(0, 7);

    if (!QFile::exists(jarPath))
        return 1;

    const QString javaPath = QStringLiteral("/app/jdk/bin/java");

    QProcess java;
    QByteArray stderrBuffer;

    QObject::connect(&java, &QProcess::readyReadStandardError, [&]() {
        stderrBuffer += java.readAllStandardError();
    });

    java.start(javaPath, { QStringLiteral("-jar"), jarPath });

    if (!java.waitForStarted())
        return 1;

    java.waitForFinished(-1);

    int exitCode = java.exitCode();
    QString errorText = QString::fromLocal8Bit(stderrBuffer).trimmed();

    if (exitCode == 0 && errorText.isEmpty())
        return 0;

    if (errorText.isEmpty())
        errorText = i18n("Java exited with an error (exit code %1).", QString::number(exitCode));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("errorMessage"), errorText);

    KLocalizedString::setApplicationDomain("org.kde.kjar");
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/watcher/ErrorDialog.qml")));

    if (engine.rootObjects().isEmpty())
        return 1;

    return app.exec();
}
