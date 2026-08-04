// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include "kjar-version.h"
#include "kjar.h"
#include "kjarapp.h"

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <vector>

using namespace Qt::Literals::StringLiterals;

namespace
{

constexpr qsizetype MaxStderrTail = 64 * 1024;

void setupApplicationMetadata()
{
    KAboutData about(QString::fromLatin1(Kjar::AppId),
                     i18n("Java Archive Runner"),
                     QString::fromLatin1(KJAR_VERSION_STRING),
                     i18n("Run Java archives with a bundled OpenJDK"),
                     KAboutLicense::GPL_V2,
                     i18n("Copyright 2026 Hadi Chokr"));
    about.addLicense(KAboutLicense::GPL_V3);
    about.addAuthor(u"Hadi Chokr"_s, QString(), u"hadichokr@icloud.com"_s);
    about.setHomepage(u"https://invent.kde.org/system/kjar"_s);
    about.setBugAddress("https://invent.kde.org/system/kjar/-/issues");
    about.setDesktopFileName(QString::fromLatin1(Kjar::AppId));

    KAboutData::setApplicationData(about);
    QCoreApplication::setOrganizationDomain(u"kde.org"_s);
}

void setupQuickStyle()
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }
}

void printUsage(FILE *out)
{
    fprintf(out,
            "Usage: kjarapp [option] [file.jar [args...] | tool [args...]]\n"
            "\n"
            "  -h, --help               Show this help and exit\n"
            "  -v, --version            Show the version and exit\n"
            "  -g, --generate-wrappers  Write JDK wrapper scripts to ~/.local/bin\n"
            "  -r, --remove-wrappers    Delete the wrapper scripts kjar wrote\n"
            "\n"
            "With no arguments the graphical interface is shown. A first argument\n"
            "that is neither an option nor a .jar file runs that tool from the\n"
            "bundled JDK, for example: kjarapp java -version\n");
}

/** Options that consume the argument after them. */
bool takesSeparateValue(const QString &a)
{
    static const QStringList opts{
        u"-cp"_s,
        u"-classpath"_s,
        u"--class-path"_s,
        u"-p"_s,
        u"--module-path"_s,
        u"--upgrade-module-path"_s,
        u"--add-modules"_s,
        u"--limit-modules"_s,
        u"--add-exports"_s,
        u"--add-opens"_s,
        u"--add-reads"_s,
        u"--patch-module"_s,
        u"--source"_s,
    };
    return opts.contains(a);
}

struct OptionRegion {
    int end = 0; // options live in args[0, end)
    bool modularMain = false;
};

/**
 * For java, everything after -jar, -m or the main class belongs to the program
 * being run, so we must not treat it as JVM options. Other tools take options
 * anywhere on the line.
 */
OptionRegion optionRegion(const QString &tool, const QStringList &args)
{
    if (tool != u"java"_s) {
        return {int(args.size()), false};
    }

    for (int i = 0; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == u"-jar"_s) {
            return {i, false};
        }
        if (a == u"-m"_s || a == u"--module"_s || a.startsWith(u"--module="_s)) {
            return {i, true};
        }
        if (!a.startsWith(u'-')) {
            return {i, false};
        }
        if (takesSeparateValue(a)) {
            ++i;
        }
    }
    return {int(args.size()), false};
}

/**
 * Index of the argument that carries a module path, or -1.
 */
int findModulePathArg(const QStringList &args, int end, bool shortFormIsModulePath)
{
    for (int i = 0; i < end; ++i) {
        const QString &a = args.at(i);
        if (a == u"--module-path"_s || a.startsWith(u"--module-path="_s) || (shortFormIsModulePath && a == u"-p"_s)) {
            return i;
        }
    }
    return -1;
}

/**
 * Replace this process with a tool from the bundled JDK.
 */
int execJdkTool(const QString &tool, const QStringList &rest)
{
    // reject anything that could escape /app/jdk/bin.
    if (tool.contains(u'/') || tool.startsWith(u'.')) {
        fprintf(stderr, "kjar: invalid tool name\n");
        return 2;
    }

    const QByteArray exe = QFile::encodeName(Kjar::jdkTool(tool));
    if (::access(exe.constData(), X_OK) != 0) {
        fprintf(stderr, "kjar: '%s' is not an available JDK tool.\n", qPrintable(tool));
        return 127;
    }

    QStringList argList = rest;
    if (Kjar::isModulePathTool(tool)) {
        Kjar::ensureUserModulesDir();

        const OptionRegion region = optionRegion(tool, argList);
        const auto optionsBegin = argList.cbegin();
        const auto optionsEnd = argList.cbegin() + region.end;

        // Compiling a module descriptor, or an explicit --add-modules, means the
        // caller controls the module graph; do not widen it behind their back.
        const bool hasModuleInfo = std::any_of(optionsBegin, optionsEnd, [](const QString &a) {
            return QFileInfo(a).fileName() == "module-info.java"_L1;
        });
        const bool hasAddModules = std::any_of(optionsBegin, optionsEnd, [](const QString &a) {
            return a == u"--add-modules"_s || a.startsWith(u"--add-modules="_s);
        });

        // Merge with the caller's entries first so their modules win on a name clash.
        const int idx = findModulePathArg(argList, region.end, tool != u"jdeps"_s);
        if (idx < 0) {
            argList.prepend(Kjar::modulePath());
            argList.prepend(u"--module-path"_s);
        } else if (argList.at(idx).startsWith(u"--module-path="_s)) {
            argList[idx] += u':' + Kjar::modulePath();
        } else if (idx + 1 < region.end) {
            argList[idx + 1] += u':' + Kjar::modulePath();
        }
        // A trailing --module-path with no value is left alone: the tool itself
        // gives a better diagnostic than we could.

        // ALL-MODULE-PATH is not valid as an initial module, so -m rules it out.
        if (Kjar::takesAllModulePath(tool) && !region.modularMain && !hasModuleInfo && !hasAddModules) {
            argList.prepend(u"ALL-MODULE-PATH"_s);
            argList.prepend(u"--add-modules"_s);
        }
    }

    QList<QByteArray> storage;
    storage.reserve(argList.size());
    std::vector<char *> argv;
    argv.reserve(argList.size() + 2);
    argv.push_back(const_cast<char *>(exe.constData()));
    for (const QString &arg : std::as_const(argList)) {
        storage.append(arg.toLocal8Bit());
        argv.push_back(storage.last().data());
    }
    argv.push_back(nullptr);

    ::execv(exe.constData(), argv.data());

    fprintf(stderr, "kjar: failed to execute %s\n", exe.constData());
    return 126;
}

int wrapperCli(int &argc, char **argv, bool remove)
{
    QCoreApplication app(argc, argv);
    setupApplicationMetadata();

    KjarApp kjar;
    const QVariantMap result = remove ? kjar.removeWrappers() : kjar.generateWrappers();
    const QString message = result.value(u"message"_s).toString();

    if (result.value(u"ok"_s).toBool()) {
        fprintf(stdout, "%s\n", qUtf8Printable(message));
        return 0;
    }
    fprintf(stderr, "%s\n", qUtf8Printable(message));
    return 1;
}

int showErrorDialog(QQmlApplicationEngine &engine, const QString &errorText)
{
    KLocalization::setupLocalizedContext(&engine);
    engine.setInitialProperties({{u"errorMessage"_s, errorText}});
    engine.loadFromModule("org.kde.kjar", "ErrorDialog");
    if (engine.rootObjects().isEmpty()) {
        fprintf(stderr, "kjar: failed to load the error dialog\n%s\n", qUtf8Printable(errorText));
        return 1;
    }
    return QGuiApplication::exec();
}

/**
 * Runs the JVM, tees stderr, and shows a dialog if the JVM failed.
 *
 * Failure is a non-zero exit, an abnormal exit, or anything at all on stderr.
 * The exit code alone is not enough: a JAR that dies on the AWT event thread
 * takes the thread down with it and the JVM still exits reporting success.
 */
int runWatcher(int &argc, char **argv, const QStringList &rest)
{
    if (rest.isEmpty()) {
        fprintf(stderr, "kjar: --watch requires a JAR path\n");
        return 2;
    }
    const QString jarPath = rest.first();
    if (!QFile::exists(jarPath)) {
        fprintf(stderr, "kjar: %s does not exist\n", qUtf8Printable(jarPath));
        return 1;
    }

    QGuiApplication app(argc, argv);
    setupApplicationMetadata();
    setupQuickStyle();

    QStringList javaArgs = Kjar::moduleArgs();
    javaArgs << u"-jar"_s << jarPath << rest.mid(1);

    QProcess java;
    java.setProcessChannelMode(QProcess::ForwardedOutputChannel);
    java.setInputChannelMode(QProcess::ForwardedInputChannel);

    QByteArray stderrTail;
    bool truncated = false;

    auto drainStderr = [&] {
        const QByteArray chunk = java.readAllStandardError();
        if (chunk.isEmpty()) {
            return;
        }
        // Still visible to `... 2>&1 | grep` while being buffered for the dialog.
        fwrite(chunk.constData(), 1, size_t(chunk.size()), stderr);
        stderrTail += chunk;
        if (stderrTail.size() > MaxStderrTail) {
            stderrTail = stderrTail.right(MaxStderrTail);
            truncated = true;
        }
    };

    QObject::connect(&java, &QProcess::readyReadStandardError, &java, drainStderr);

    java.start(Kjar::jdkTool(u"java"), javaArgs);
    if (!java.waitForStarted()) {
        QQmlApplicationEngine engine;
        return showErrorDialog(engine, i18n("Could not start the bundled Java runtime."));
    }
    java.waitForFinished(-1);
    drainStderr(); // whatever was still buffered when the process ended

    const bool crashed = java.exitStatus() != QProcess::NormalExit;
    if (!crashed && java.exitCode() == 0 && stderrTail.isEmpty()) {
        return 0;
    }

    QString errorText = QString::fromLocal8Bit(stderrTail).trimmed();
    if (errorText.isEmpty()) {
        errorText = crashed ? i18n("Java terminated abnormally.") : i18n("Java exited with an error (exit code %1).", java.exitCode());
    } else if (truncated) {
        errorText.prepend(i18n("(output truncated, showing the last 64 KiB)") + u"\n\n"_s);
    }

    QQmlApplicationEngine engine;
    return showErrorDialog(engine, errorText);
}

enum class StartupError {
    None,
    FileNotFound,
    LaunchFailed,
};

int showMainWindow(int &argc, char **argv, StartupError error, const QString &path = QString())
{
    QGuiApplication app(argc, argv);
    setupApplicationMetadata();
    setupQuickStyle();

    QString initialError;
    switch (error) {
    case StartupError::None:
        break;
    case StartupError::FileNotFound:
        initialError = i18n("File does not exist or cannot be accessed.\n"
                            "KJar can only see your Home directory by default.\n"
                            "If the file exists elsewhere, move it to your Home directory "
                            "or expand KJar's permissions in System Settings.");
        break;
    case StartupError::LaunchFailed:
        initialError = i18n("Failed to launch %1.", path);
        break;
    }

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.setInitialProperties({{u"initialError"_s, initialError}});
    engine.loadFromModule("org.kde.kjar", "Main");
    if (engine.rootObjects().isEmpty()) {
        fprintf(stderr, "kjar: failed to load the user interface\n");
        return 1;
    }
    return app.exec();
}

} // namespace

int main(int argc, char *argv[])
{
    KLocalizedString::setApplicationDomain("org.kde.kjar");

    // Copy the arguments before any QCoreApplication touches argc/argv, and
    // construct exactly one application object further down.
    QStringList args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    const QString first = args.value(1);
    const QStringList rest = args.mid(2);

    if (first == u"--help"_s || first == u"-h"_s) {
        printUsage(stdout);
        return 0;
    }

    if (first == u"--version"_s || first == u"-v"_s) {
        fprintf(stdout, "kjar %s\n", KJAR_VERSION_STRING);
        return 0;
    }

    if (first == u"--generate-wrappers"_s || first == u"-g"_s) {
        return wrapperCli(argc, argv, false);
    }

    if (first == u"--remove-wrappers"_s || first == u"-r"_s) {
        return wrapperCli(argc, argv, true);
    }

    if (first == u"--watch"_s) {
        return runWatcher(argc, argv, rest);
    }

    // Anything that is neither an option nor a JAR is a JDK tool invocation.
    if (!first.isEmpty() && !first.startsWith(u'-') && !first.endsWith(u".jar"_s, Qt::CaseInsensitive)) {
        return execJdkTool(first, rest);
    }

    if (first.endsWith(u".jar"_s, Qt::CaseInsensitive)) {
        const QString path = first.startsWith(u"file://"_s) ? QUrl(first).toLocalFile() : first;
        if (!QFile::exists(path)) {
            return showMainWindow(argc, argv, StartupError::FileNotFound);
        }

        QStringList watcherArgs{u"--watch"_s, path};
        watcherArgs += rest;
        if (QProcess::startDetached(Kjar::selfPath(), watcherArgs)) {
            return 0;
        }
        return showMainWindow(argc, argv, StartupError::LaunchFailed, path);
    }

    if (!first.isEmpty()) {
        fprintf(stderr, "kjar: unknown option '%s'\n", qUtf8Printable(first));
        printUsage(stderr);
        return 2;
    }

    return showMainWindow(argc, argv, StartupError::None);
}
