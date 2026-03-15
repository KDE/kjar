// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <KLocalizedString>
#include <KLocalizedContext>
#include <QFile>
#include <QDir>
#include <QProcess>
#include "kjarapp.h"

static int showGui(KjarApp &kjarApp, const QString &initialError = QString()) {
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
  engine.rootContext()->setContextProperty(QStringLiteral("backend"), &kjarApp);
  if (!initialError.isEmpty()) {
    engine.rootContext()->setContextProperty(QStringLiteral("initialError"),
                                             initialError);
  }
  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  return qApp->exec();
}

static QString buildModulePath()
{
  const QDir shareDir(QStringLiteral("/app/share"));
  QStringList paths;
  for (const QFileInfo &fi : shareDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    paths << fi.absoluteFilePath();
  }
  const QString modulesDir = KjarApp::defaultModulesDir();
  QDir().mkpath(modulesDir);
  paths << modulesDir;
  return paths.join(QLatin1Char(':'));
}

static QStringList buildModuleArgs(bool addAllModules)
{
  QStringList args = { QStringLiteral("--module-path"), buildModulePath() };
  if (addAllModules) {
    args << QStringLiteral("--add-modules") << QStringLiteral("ALL-MODULE-PATH");
  }
  return args;
}

static const QStringList moduleAwareTools = {
  QStringLiteral("java"),
  QStringLiteral("javac"),
  QStringLiteral("javadoc"),
  QStringLiteral("jdeps"),
  QStringLiteral("jshell"),
  QStringLiteral("jlink"),
  QStringLiteral("jnativescan"),
  QStringLiteral("jpackage")
};

int main(int argc, char *argv[])
{
  if (argc > 1) {
    const QString arg = QString::fromLocal8Bit(argv[1]);

    if (!arg.startsWith(QLatin1Char('-')) &&
      !arg.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive)) {
      const QStringList allowedPaths = {
        QStringLiteral("/app/bin"),
        QStringLiteral("/app/jdk/bin")
      };
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
    if (moduleAwareTools.contains(arg)) {
      bool hasModuleInfo = false;
      for (int i = 2; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]).endsWith(QStringLiteral("module-info.java"))) {
          hasModuleInfo = true;
          break;
        }
      }
      procArgs = buildModuleArgs(!hasModuleInfo);
    }
    for (int i = 2; i < argc; ++i) {
      procArgs << QString::fromLocal8Bit(argv[i]);
    }

    QProcess proc;
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(toolPath, procArgs);
    proc.waitForFinished(-1);
    return proc.exitCode();
      }

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
        });
        kjarApp.generateWrappers();
        loop.exec();
        return 0;
      }

      if (arg.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive)) {
        QString filePath = arg;
        if (filePath.startsWith(QLatin1String("file://"))) {
          filePath.remove(0, 7);
        }

        QGuiApplication app(argc, argv);
        app.setApplicationName(QStringLiteral("org.kde.kjar"));
        app.setDesktopFileName(QStringLiteral("org.kde.kjar"));
        KLocalizedString::setApplicationDomain("org.kde.kjar");

        KjarApp kjarApp;

        if (!QFile::exists(filePath)) {
          const QString error = i18n(
            "File does not exist or cannot be accessed.\n"
            "KJar can only see your Home directory by default.\n"
            "If the file exists elsewhere, move it to your Home directory "
            "or expand KJar's permissions in System Settings.");
          return showGui(kjarApp, error);
        }

        QString errorMessage;
        QObject::connect(&kjarApp, &KjarApp::errorOccurred,
                         [&](const QString &err) { errorMessage = err; });

        if (kjarApp.runJarFile(filePath)) {
          return 0;
        }

        return showGui(kjarApp, errorMessage);
      }
  }

  QGuiApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("org.kde.kjar"));
  app.setDesktopFileName(QStringLiteral("org.kde.kjar"));
  KLocalizedString::setApplicationDomain("org.kde.kjar");

  KjarApp kjarApp;
  return showGui(kjarApp);
}
