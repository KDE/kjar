// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#ifndef KJARAPP_H
#define KJARAPP_H

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QVariantMap>

class QUrl;

/**
 * Backend exposed to QML as a singleton. Every operation is synchronous because I suck at async C++.
 */
class KjarApp : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList availableTools READ availableTools CONSTANT)

public:
    explicit KjarApp(QObject *parent = nullptr);

    QStringList availableTools() const;

    /** Launches the JAR in a detached watcher process. Returns an error message, or an empty string on success. */
    Q_INVOKABLE QString runJar(const QUrl &url);

    /** Returns {"ok": bool, "message": QString}. */
    Q_INVOKABLE QVariantMap generateWrappers();

    /** Deletes the wrappers we wrote, leaving everything else alone. Same return shape. */
    Q_INVOKABLE QVariantMap removeWrappers();

    Q_INVOKABLE void openModulesFolder();
};

#endif // KJARAPP_H
