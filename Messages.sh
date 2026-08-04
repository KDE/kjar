#! /usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none

$EXTRACTRC $(find . -name '*.ui' -o -name '*.rc') >> rc.cpp 2>/dev/null
$XGETTEXT $(find src -name '*.cpp' -o -name '*.h' -o -name '*.qml') \
    rc.cpp -o "$podir"/org.kde.kjar.pot
rm -f rc.cpp
