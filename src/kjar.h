// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#ifndef KJAR_H
#define KJAR_H

#include <QLatin1StringView>
#include <QString>
#include <QStringList>

namespace Kjar
{

inline constexpr QLatin1StringView AppId("org.kde.kjar");
inline constexpr QLatin1StringView JdkBinDir("/app/jdk/bin");
inline constexpr QLatin1StringView JavaFxDir("/app/share/javafx");

/** Absolute path of a tool in the bundled JDK. */
QString jdkTool(QStringView name);

/**
 * Absolute path of this executable.
 */
QString selfPath();

/**
 * ~/.local/share/kjar/modules
 *
 * Deliberately *not* QStandardPaths::AppDataLocation: under Flatpak XDG_DATA_HOME
 * is redirected to ~/.var/app/org.kde.kjar/data.
 */
QString userModulesDir();

/** Creates userModulesDir() if it does not exist yet. */
bool ensureUserModulesDir();

/** ~/.local/bin, inside --filesystem=home, writable directly from the sandbox. */
QString wrapperDir();

/** Executable names found in /app/jdk/bin */
QStringList jdkTools();

/** Tools that understand --module-path. */
bool isModulePathTool(QStringView name);

/**
 * Subset of isModulePathTool() that may also get --add-modules ALL-MODULE-PATH.
 *
 * jlink and jpackage are excluded from both: widening their module graph changes
 * the image they produce.
 */
bool takesAllModulePath(QStringView name);

/** <javafx>:<user modules>, as a single --module-path value. */
QString modulePath();

/** --module-path <javafx>:<user modules> --add-modules ALL-MODULE-PATH */
QStringList moduleArgs();

} // namespace Kjar

#endif // KJAR_H
