# kjar

**K**ool **J**ava **A**rchive **R**unner — pronounced *K-jarrrrgh* (like a dinosaur pirate).

Run JAR files directly via a bundled OpenJDK Flatpak with a Kirigami GUI.
And optionaly generate wrapper scripts for developers who get tired of typing out `flatpak run` commands.

## Build

```
flatpak remote-add --user flathub https://flathub.org/repo/flathub.flatpakrepo
```
```
flatpak-builder --install --user --install-deps-from=flathub --force-clean build-dir .flatpak-manifest.json
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

If Java reports an error (e.g. missing main manifest attribute) the GUI opens automatically and displays it.

### Generate wrappers (for Developers)

Creates wrapper scripts in `~/.local/bin` for all JDK tools (`java`, `javac`, `jar`, etc.) so they can be used like a system JDK:

```
flatpak run org.kde.kjar --generate-wrappers
```

Or via the GUI: **Advanced → Generate Wrappers**.

Existing non-kjar binaries are never overwritten. `~/.local/bin` must be in `PATH`.

### List available tools

Via the GUI: **Advanced → Show Available Tools**.

### Run a JDK tool directly

```
flatpak run org.kde.kjar java -version
flatpak run org.kde.kjar javac MyClass.java
```

## License

GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
