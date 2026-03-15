// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#ifndef KJARAPP_H
#define KJARAPP_H

#include <QObject>
#include <QStringList>
#include <KLocalizedString>

class KjarApp : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableTools READ availableTools NOTIFY toolsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit KjarApp(QObject *parent = nullptr);

    Q_INVOKABLE bool runJarFile(const QString &file);
    Q_INVOKABLE void generateWrappers();
    Q_INVOKABLE void openModulesFolder();

    QStringList availableTools() const;
    bool busy() const { return m_busy; }

    static QString defaultModulesDir();

Q_SIGNALS:
    void toolsChanged();
    void busyChanged();
    void operationCompleted(const QString &message);
    void errorOccurred(const QString &error);

private:
    QStringList findAvailableTools() const;
    void setBusy(bool busy);
    QStringList buildJvmArgs(const QString &jarPath) const;

    QString m_targetDir;
    bool m_busy = false;
};

#endif
