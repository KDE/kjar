// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include "kjar.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <initializer_list>

using namespace Qt::Literals::StringLiterals;

namespace
{

bool matchesAny(std::initializer_list<QLatin1StringView> names, QStringView name)
{
    return std::ranges::any_of(names, [name](QLatin1StringView candidate) {
        return name == candidate;
    });
}

} // namespace

QString Kjar::jdkTool(QStringView name)
{
    QString path = QString::fromLatin1(JdkBinDir);
    path += u'/';
    path += name;
    return path;
}

QString Kjar::selfPath()
{
    const QString exe = QFileInfo(u"/proc/self/exe"_s).canonicalFilePath();
    return exe.isEmpty() ? QCoreApplication::applicationFilePath() : exe;
}

QString Kjar::userModulesDir()
{
    return QDir::homePath() + u"/.local/share/kjar/modules"_s;
}

bool Kjar::ensureUserModulesDir()
{
    return QDir().mkpath(userModulesDir());
}

QString Kjar::wrapperDir()
{
    return QDir::homePath() + u"/.local/bin"_s;
}

QStringList Kjar::jdkTools()
{
    QStringList tools;
    const QDir dir(QString::fromLatin1(JdkBinDir));
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Executable, QDir::Name);
    tools.reserve(entries.size());
    for (const QFileInfo &fi : entries) {
        tools.append(fi.fileName());
    }
    return tools;
}

bool Kjar::isModulePathTool(QStringView name)
{
    return matchesAny({"java"_L1, "javac"_L1, "javadoc"_L1, "jdeps"_L1, "jshell"_L1, "jnativescan"_L1}, name);
}

bool Kjar::takesAllModulePath(QStringView name)
{
    return matchesAny({"java"_L1, "javac"_L1, "javadoc"_L1, "jshell"_L1}, name);
}

QString Kjar::modulePath()
{
    return QString::fromLatin1(JavaFxDir) + u':' + userModulesDir();
}

QStringList Kjar::moduleArgs()
{
    ensureUserModulesDir();
    return {u"--module-path"_s, modulePath(), u"--add-modules"_s, u"ALL-MODULE-PATH"_s};
}
