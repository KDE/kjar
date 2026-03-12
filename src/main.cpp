// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <KLocalizedString>
#include <KLocalizedContext>
#include <QFile>
#include <QProcess>
#include "kjarapp.h"

int main(int argc, char *argv[])
{
    // Handle pure CLI cases before initializing any GUI
    if (argc > 1) {
        const QString arg = QString::fromLocal8Bit(argv[1]);

        // Direct tool invocation: only /app/bin and /app/jdk/bin are searched
        if (!arg.startsWith(QLatin1Char('-')) &&
            !arg.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive))
        {
            // Find tool in sandbox paths
            const QStringList allowedPaths = { QStringLiteral("/app/bin"), QStringLiteral("/app/jdk/bin") };
            QString toolPath;
            for (const QString &path : allowedPaths) {
                const QString candidate = path + QLatin1Char('/') + arg;
                if (QFile::exists(candidate)) {
                    toolPath = candidate;
                    break;
                }
            }

            if (toolPath.isEmpty()) {
                fprintf(stderr, "kjar: '%s' is not an available JDK tool.\n", argv[1]);
                return 1;
            }

            QStringList procArgs;
            for (int i = 2; i < argc; ++i)
                procArgs << QString::fromLocal8Bit(argv[i]);

            QProcess proc;
            proc.setProcessChannelMode(QProcess::ForwardedChannels);
            proc.start(toolPath, procArgs);
            proc.waitForFinished(-1);
            return proc.exitCode();
        }

        // --generate-wrappers/-g: CLI call, assume script/TTY context
        if (arg == QLatin1String("--generate-wrappers") || arg == QLatin1String("-g")) {
            QCoreApplication coreApp(argc, argv);
            KLocalizedString::setApplicationDomain("org.kde.kjar");
            KjarApp kjarApp;
            QEventLoop loop;
            QObject::connect(&kjarApp, &KjarApp::operationCompleted, [&](const QString &msg) {
                fprintf(stdout, "%s\n", msg.toLocal8Bit().constData());
                loop.quit();
            });
            QObject::connect(&kjarApp, &KjarApp::errorOccurred, [&](const QString &err) {
                fprintf(stderr, "%s\n", err.toLocal8Bit().constData());
                // don't quit as more errors may follow per-tool, operationCompleted ends it
            });
            kjarApp.generateWrappers();
            loop.exec();
            return 0;
        }

        // If the argument is a .jar file, attempt to run it detached
        if (arg.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive) && QFile::exists(arg)) {
            QString filePath = arg;
            if (filePath.startsWith(QLatin1String("file://")))
                filePath.remove(0, 7);

            // We need a QGuiApplication because we might show the GUI on error
            QGuiApplication app(argc, argv);
            app.setApplicationName(QStringLiteral("org.kde.kjar"));
            app.setDesktopFileName(QStringLiteral("org.kde.kjar"));
            KLocalizedString::setApplicationDomain("org.kde.kjar");

            KjarApp kjarApp;
            QString errorMessage;
            QObject::connect(&kjarApp, &KjarApp::errorOccurred, [&](const QString &err) {
                errorMessage = err;
            });

            bool started = kjarApp.runJarFile(filePath);
            if (started) {
                return 0;   // JAR launched successfully
            } else {
                // Starting failed so show the GUI with the error
                QQmlApplicationEngine engine;
                engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
                engine.rootContext()->setContextProperty(QStringLiteral("backend"), &kjarApp);
                engine.rootContext()->setContextProperty(QStringLiteral("initialError"), errorMessage);
                engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
                return app.exec();
            }
        }
    }

    // GUI path, no valid JAR or tool argument
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("org.kde.kjar"));
    app.setDesktopFileName(QStringLiteral("org.kde.kjar"));
    KLocalizedString::setApplicationDomain("org.kde.kjar");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    KjarApp kjarApp;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &kjarApp);

    engine.rootContext()->setContextProperty(QStringLiteral("initialError"), QString());
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    return app.exec();
}
