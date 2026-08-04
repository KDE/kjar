# kjar
**K**ool **J**ava **A**rchive **R**unner (pronounced *K-jarrrrgh*, like a dragon pirate >:D).

Run JAR files directly via a bundled OpenJDK Flatpak with a Kirigami GUI. And optionally generate wrapper scripts for developers who get tired of typing out `flatpak run` commands.

## Security

**KJar does not sandbox the JARs it runs.**

The Flatpak holds `--filesystem=home` and `--share=network`, and the JVM it
starts inherits both. A JAR launched through KJar can therefore read and modify
everything in your Home folder and talk to the network exactly as it could
under a system-wide Java installation.

**Only run JAR files you trust.**

## Build

Needs the `org.kde.Platform` and `org.kde.Sdk` 6.11 runtimes plus the
`org.freedesktop.Sdk.Extension.openjdk25` extension, from whichever remote you
already have them on.

```
flatpak-builder --install --user --force-clean build-dir .flatpak-manifest.json
```

## Use

### GUI

Launch the app to open JAR files interactively:
```
flatpak run org.kde.kjar
```

KJar is also registered as a handler for `.jar` files so you can open them directly from your file manager.

### Run a JAR file directly
```
flatpak run org.kde.kjar /path/to/app.jar
```

The command returns immediately; the JAR keeps running in a watcher process.
If Java reports an error (e.g. missing main manifest attribute) the watcher
opens a window and displays it.

A JAR can also fail without a non-zero exit code, for example when it dies on
the AWT event thread. The watcher therefore treats anything on stderr as a
failure. Some JARs write warnings there and work fine, so the window can turn
up after a perfectly good run; it only ever appears once the JAR has exited, so
closing it costs nothing.

### JavaFX

JavaFX is bundled. JavaFX apps will work out of the box with no extra setup.

### External modules

If a JAR requires external modules (e.g. a custom library distributed as a modular JAR), drop them into:
```
~/.local/share/kjar/modules/
```

They will be picked up automatically on the next run. You can open this folder directly via **Advanced → Open Modules Folder**.

### Generate wrappers (for Developers)

Creates wrapper scripts in `~/.local/bin` for all JDK tools (`java`, `javac`, `jar`, etc.) so they can be used like a system JDK:
```
flatpak run org.kde.kjar --generate-wrappers
```

Or via the GUI: **Advanced → Generate Wrappers**. Existing non-kjar binaries are never overwritten. `~/.local/bin` must be in `PATH`.

Note that a generated `java` in `~/.local/bin` shadows a system JDK whenever
`~/.local/bin` comes first in `PATH`.

Wrappers can only see files inside your Home folder, so `javac /tmp/Foo.java`
will not work unless you extend the permission in System Settings.

### Remove wrappers

Deletes the scripts kjar wrote and nothing else:
```
flatpak run org.kde.kjar --remove-wrappers
```

Or via the GUI: **Advanced → Remove Wrappers**.

### List available tools

Via the GUI: **Advanced → Show Available Tools**.

### Run a JDK tool directly
```
flatpak run org.kde.kjar java -version
flatpak run org.kde.kjar javac MyClass.java
```

`java`, `javac`, `javadoc`, `jdeps`, `jshell` and `jnativescan` get the bundled
JavaFX and `~/.local/share/kjar/modules/` appended to their module path. Of
those, `java`, `javac`, `javadoc` and `jshell` also get
`--add-modules ALL-MODULE-PATH`, unless you pass your own `--add-modules`,
compile a `module-info.java`, or run a module with `-m`.

`jlink` and `jpackage` are left alone, since a wider module graph would change
the image they produce.

For `java`, only the arguments before `-jar`, `-m` or the main class are
treated as JVM options; anything after that is passed to your program
untouched.

## License

GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
