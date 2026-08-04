#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
#
# Discard native debug symbols from /app/jdk/jmods.
#

set -eu

JDK=/app/jdk
JMOD="$JDK/bin/jmod"
MODULES="${1:-java.base}"

mb() {
    awk -v kb="$1" 'BEGIN { printf "%.1f", kb / 1024 }'
}

command -v objcopy >/dev/null || { echo "strip-jmods: objcopy not found"; exit 1; }
[ -d "$JDK/jmods" ] || { echo "strip-jmods: $JDK/jmods missing"; exit 1; }
[ -x "$JMOD" ] || { echo "strip-jmods: $JMOD missing"; exit 1; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

before=$(du -sk "$JDK/jmods" | cut -f1)
touched_non_base=0

# shellcheck disable=SC2086
for name in $MODULES; do
    jm="$JDK/jmods/$name.jmod"
    if [ ! -f "$jm" ]; then
        echo "strip-jmods: ERROR: $jm not found"
        exit 1
    fi
    [ "$name" = "java.base" ] || touched_non_base=1

    kb=$(du -k "$jm" | cut -f1)
    d="$WORK/$name"

    # Per-module invariant: version, target platform, main class, requires and
    # exports must all survive the round-trip.
    "$JMOD" describe "$jm" > "$WORK/$name.before"

    "$JMOD" extract --dir "$d" "$jm"

    # --strip-debug
    find "$d" -type f \( -name '*.so' -o -name '*.so.*' \) \
        -exec objcopy --strip-debug {} \; 2>/dev/null || true
    [ -d "$d/bin" ] && find "$d/bin" -type f \
        -exec objcopy --strip-debug {} \; 2>/dev/null || true

    set -- create
    [ -d "$d/classes" ] && set -- "$@" --class-path    "$d/classes"
    [ -d "$d/bin" ]     && set -- "$@" --cmds          "$d/bin"
    [ -d "$d/lib" ]     && set -- "$@" --libs          "$d/lib"
    [ -d "$d/conf" ]    && set -- "$@" --config        "$d/conf"
    [ -d "$d/include" ] && set -- "$@" --header-files  "$d/include"
    [ -d "$d/legal" ]   && set -- "$@" --legal-notices "$d/legal"
    [ -d "$d/man" ]     && set -- "$@" --man-pages     "$d/man"

    rm -f "$jm"
    "$JMOD" "$@" "$jm"
    rm -rf "$d"

    "$JMOD" describe "$jm" > "$WORK/$name.after"
    if ! diff -q "$WORK/$name.before" "$WORK/$name.after" >/dev/null; then
        echo "strip-jmods: ERROR: descriptor changed for $name"
        diff "$WORK/$name.before" "$WORK/$name.after" || true
        exit 1
    fi

    printf 'strip-jmods:   %-22s %7s MB -> %7s MB\n' "$name" \
        "$(mb "$kb")" "$(mb "$(du -k "$jm" | cut -f1)")"
done

# Only reachable when the caller passed non-base modules.
if [ "$touched_non_base" -eq 1 ]; then
    echo "strip-jmods: recomputing recorded module hashes"
    if ! "$JMOD" hash --module-path "$JDK/jmods" --hash-modules '.*'; then
        echo "strip-jmods: ERROR: jmod hash failed; rerun with java.base only,"
        echo "strip-jmods:        which no other module records a hash of."
        exit 1
    fi
fi

after=$(du -sk "$JDK/jmods" | cut -f1)
echo "strip-jmods: total $(mb "$before") MB -> $(mb "$after") MB"

# Smoke test
echo "strip-jmods: verifying jlink against the rewritten jmods"

if ! "$JDK/bin/jlink" --module-path "$JDK/jmods" \
        --add-modules java.base,java.desktop,java.xml,jdk.compiler \
        --output "$WORK/verify"; then
    echo "strip-jmods: ERROR: jlink failed against the rewritten jmods"
    exit 1
fi

if ! "$WORK/verify/bin/java" -version; then
    echo "strip-jmods: ERROR: linked runtime does not run"
    exit 1
fi

rm -rf "$WORK/verify"
echo "strip-jmods: OK"
