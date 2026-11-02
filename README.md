# Stellar Install

Aplicación de escritorio ligera para Linux, escrita en **C++17 con Qt (5 o 6)**, que
permite consultar los metadatos y las dependencias de un paquete `.deb` local y
después instalarlo de forma segura con `apt`, resolviendo automáticamente sus
dependencias y solicitando permisos de administrador mediante **PolicyKit**
(`pkexec`).

## Características

- Lectura de metadatos sin privilegios de root con `dpkg-deb -I`.
- Panel con nombre, versión, arquitectura, tamaño instalado, mantenedor,
  descripción y otros campos del bloque de control.
- Tabla de dependencias del campo `Depends`, incluidas alternativas (`a | b`) y
  restricciones de versión (`>= 1.0`).
- Comprobación automática del estado del paquete en el sistema: no instalado,
  ya instalado con la misma versión, actualización disponible o versión
  instalada más reciente que la del archivo.
- El botón de acción cambia entre **Instalar**, **Actualizar** y **Reinstalar**
  según la versión detectada. Si el sistema ya tiene una versión más reciente,
  la instalación se bloquea para evitar degradar el paquete.
- Instalación con `pkexec apt install -y /ruta/absoluta/paquete.deb`.
- Autenticación gráfica nativa de PolicyKit (no pide la contraseña en la app).
- Ejecución **totalmente asíncrona** con `QProcess`: la interfaz no se bloquea.
- Registro de instalación en tiempo real y barra de progreso.
- Manejo específico de la cancelación de la ventana de autenticación.
- Selección mediante diálogo, arrastrar y soltar, o argumento de línea de
  comandos.
- Interfaz adaptable con `QSplitter`, diseño responsivo y soporte de temas
  claros y oscuros del escritorio.
- Botón de configuración (solo icono) para elegir la apariencia (sistema, clara
  u oscura) y abrir el diálogo **Acerca de** con el resumen y la versión de la
  aplicación.
- La apariencia elegida se guarda y se restaura al volver a abrir la aplicación.

## Requisitos

### Ejecución

| Componente | Paquete habitual en Debian/Ubuntu |
| --- | --- |
| Lectura de paquetes | `dpkg` (`dpkg-deb`) |
| Gestión e instalación | `apt` |
| Elevación gráfica | `pkexec` (`policykit-1` / `polkitd`), con un agente polkit activo en la sesión |
| Bibliotecas Qt | `libqt6widgets6` o `libqt5widgets5` |

### Compilación

En Ubuntu/Debian con Qt 6:

```bash
sudo apt install build-essential cmake qt6-base-dev
```

Con Qt 5 (alternativa):

```bash
sudo apt install build-essential cmake qtbase5-dev
```

## Compilación

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

El ejecutable queda en `build/stellar-install`.

Si están instaladas las bibliotecas de desarrollo de Qt 5 y Qt 6 a la vez, CMake
elige Qt 6. Para forzar Qt 5:

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build build-qt5 -j"$(nproc)"
```

### Integración en el escritorio

El archivo `packaging/stellar-install.desktop` está preparado para integrarse en
GNOME, KDE, XFCE y otros escritorios compatibles con el estándar Desktop Entry:

- Nombre, descripción y palabras clave en español e inglés.
- `TryExec` y `StartupWMClass` para que el lanzador y la ventana se asocien
  correctamente.
- Icono `stellar-install` instalado en el tema `hicolor` en varios tamaños
  (16, 24, 32, 48, 64, 128, 256 y 512 px).
- Categorías `System` y `PackageManager`.
- Asociación de archivos `.deb` mediante
  `application/vnd.debian.binary-package` y sus alias `application/x-deb` y
  `application/x-debian-package`.

La forma más sencilla de instalar y registrar todo es usar el script incluido:

```bash
# Instalación para el usuario actual (~/.local)
./packaging/install.sh

# Instalación para todo el sistema (/usr/local)
sudo ./packaging/install.sh --system

# No cambiar la aplicación predeterminada para los .deb
./packaging/install.sh --no-mime
```

El script compila el proyecto si el binario no existe o está desactualizado,
instala el ejecutable, el archivo `.desktop` y el icono, actualiza las cachés del
escritorio y registra Stellar Install como aplicación predeterminada para abrir
archivos `.deb`.

Para comprobar la asociación:

```bash
xdg-mime query default application/vnd.debian.binary-package
# Debe devolver: stellar-install.desktop
```

Para retirar la integración:

```bash
./packaging/uninstall.sh
sudo ./packaging/uninstall.sh --system
```

También se puede instalar únicamente con CMake. En ese caso, el archivo
`.desktop` y el icono no se registran como asociación predeterminada de `.deb`:

```bash
sudo cmake --install build --prefix /usr/local
sudo update-desktop-database /usr/local/share/applications 2>/dev/null || true
sudo gtk-update-icon-cache -f -t /usr/local/share/icons/hicolor 2>/dev/null || true
```

### Distribución portable

El script `packaging/build-portable.sh` genera una carpeta autocontenida con el
ejecutable, las bibliotecas de Qt, sus dependencias y los plugins necesarios:

```bash
./packaging/build-portable.sh
```

El resultado se crea en:

```text
dist/Stellar-Install-1.0.0-linux-x86_64-portable/
dist/Stellar-Install-1.0.0-linux-x86_64-portable.tar.gz
```

La carpeta contiene:

```text
stellar-install          Ejecutable con RPATH portable ($ORIGIN/lib)
lib/                     Bibliotecas de Qt y dependencias
plugins/                 Plugins de Qt (X11, Wayland, imágenes, etc.)
qt.conf                  Rutas relativas de bibliotecas y plugins
stellar-install.png      Icono de la aplicación
LEEME.txt                Instrucciones y requisitos
```

Se puede ejecutar directamente desde la carpeta, incluso desde otro directorio
de trabajo:

```bash
./dist/Stellar-Install-1.0.0-linux-x86_64-portable/stellar-install
./dist/Stellar-Install-1.0.0-linux-x86_64-portable/stellar-install paquete.deb
```

La distribución incluye Qt y no necesita que Qt esté instalado en el sistema.
Se excluyen deliberadamente `glibc`, el cargador dinámico y la pila gráfica
(OpenGL/EGL/Vulkan) para usar los del equipo y mantener la compatibilidad con
sus controladores. Para instalar paquetes siguen siendo necesarios `dpkg-deb`,
`apt` y `pkexec`, que forman parte del sistema operativo.

La compatibilidad binaria depende de la arquitectura (`x86_64`) y de la versión
de `glibc` del equipo donde se generó la distribución. Para equipos con una
`glibc` más antigua, genera la carpeta portable en una versión de
Debian/Ubuntu igual o más antigua que la del equipo de destino.

Opciones del script:

```bash
./packaging/build-portable.sh --output /ruta/Stellar-Install-Portable
./packaging/build-portable.sh --no-archive
./packaging/build-portable.sh --no-strip
```

## Uso

```bash
./build/stellar-install
```

También se puede abrir un paquete directamente:

```bash
./build/stellar-install /ruta/al/paquete.deb
```

1. Pulsa **Abrir archivo…** (o arrastra un `.deb` a la ventana).
2. Revisa los metadatos, el estado del paquete en el sistema y las dependencias.
   El botón indica la acción adecuada: **Instalar paquete**, **Actualizar
   paquete** o **Reinstalar paquete**.
3. Pulsa el botón de acción y confirma la operación.
4. Introduce la contraseña de administrador en la ventana de PolicyKit.
5. Sigue el progreso en **Mostrar detalles**. Al terminar se muestra un aviso de
   éxito o de error y se vuelve a comprobar la versión instalada.

El botón de engranaje de la esquina superior derecha abre el menú
**Configuración**:

- **Apariencia → Seguir el tema del sistema / Claro / Oscuro**: cambia los
  colores de toda la interfaz al instante.
- **Acerca de Stellar Install**: muestra un resumen de la aplicación, su versión
  y la versión de Qt con la que se compiló.

### Probar con un paquete descargado

```bash
mkdir -p /tmp/deb-prueba && cd /tmp/deb-prueba
apt-get download sl        # descarga el .deb sin instalarlo
./build/stellar-install /tmp/deb-prueba/sl_*.deb
```

## Estructura del proyecto

```text
CMakeLists.txt
src/
  main.cpp            Punto de entrada, argumentos y registro de tipos.
  MainWindow.h/.cpp   Ventana principal, paneles, estilos y flujo de la UI.
  DebPackageInfo.h    Estructuras con los metadatos y dependencias.
  DebParser.h/.cpp    Ejecuta dpkg-deb -I y parsea el bloque de control.
  InstalledPackageChecker.h/.cpp  Consulta la versión instalada y la compara.
  PackageManager.h/.cpp  Ejecuta pkexec apt install y transmite la salida.
resources/
  resources.qrc       Recursos embebidos.
  icons/…             Icono de la aplicación y del botón de configuración (PNG).
  icons/hicolor/…     Tamaños estándar del icono para el tema del escritorio.
packaging/
  stellar-install.desktop  Entrada de menú, icono y asociación de .deb.
  install.sh               Integración para el usuario o para el sistema.
  uninstall.sh             Retira la integración del escritorio.
  build-portable.sh        Genera la distribución portable con Qt incluido.
```

## Cómo funciona

### 1. Lector de metadatos (`DebParser`)

`DebParser` ejecuta, sin privilegios:

```bash
dpkg-deb -I /ruta/absoluta/paquete.deb
```

La salida se procesa de forma tolerante:

- Se localiza la línea `Package:` para aislar el bloque de control (las líneas
  informativas iniciales de `dpkg-deb` se ignoran).
- Se interpreta el formato RFC 822 del bloque, incluyendo líneas de continuación
  con sangría. Así la descripción larga se conserva y los campos `Depends`
  partidos en varias líneas se reconstruyen.
- Los campos se normalizan a minúsculas para no depender de mayúsculas.
- `Depends` se divide por comas **solo al nivel superior**, respetando los
  paréntesis de las versiones y separando alternativas con `|`.
- El punto (`.`) que Debian usa para representar líneas en blanco dentro de
  `Description` se convierte en una línea vacía.

Todo el proceso usa señales de `QProcess`, por lo que no hay esperas bloqueantes
ni `waitForFinished()`.

### 2. Comprobador de versión instalada (`InstalledPackageChecker`)

Tras leer el paquete, la aplicación consulta el sistema sin privilegios:

```bash
dpkg-query -W -f='${Status}\t${Version}\t${Architecture}\n' nombre-del-paquete
```

Si el paquete está instalado, compara las versiones con la semántica oficial de
Debian:

```bash
dpkg --compare-versions <versión-instalada> lt <versión-del-archivo>
dpkg --compare-versions <versión-instalada> eq <versión-del-archivo>
```

El resultado se muestra en un aviso y en el campo **Estado** de los metadatos:

| Situación | Acción del botón |
| --- | --- |
| No instalado | **Instalar paquete** |
| Instalado, misma versión | **Reinstalar paquete** |
| Instalado, versión anterior a la del archivo | **Actualizar paquete** |
| Instalado, versión más reciente que la del archivo | Deshabilitado: **Versión más reciente instalada** |
| No se pudo consultar | **Instalar paquete** (con aviso) |

La consulta también es asíncrona, con un tiempo máximo de espera, y se cancela
si el usuario abre otro paquete. Se tienen en cuenta las distintas
arquitecturas instaladas para comparar la que corresponde al archivo `.deb`.

### 3. Gestor de instalación (`PackageManager`)

La instalación se ejecuta como:

```bash
pkexec apt install -y \
  -o Dpkg::Options::=--force-confdef \
  -o Dpkg::Options::=--force-confold \
  /ruta/absoluta/paquete.deb
```

Las opciones de `dpkg` conservan los ficheros de configuración existentes y
evitan preguntas interactivas que no se podrían mostrar desde la ventana. La
entrada estándar del proceso se conecta a `/dev/null` para que, si `apt` o
`dpkg` intentaran pedir datos, reciban EOF en vez de dejar la instalación
bloqueada. `QProcess` recibe el programa y los argumentos por separado; nunca se
usa un intérprete de shell, de modo que una ruta con espacios o caracteres
especiales no puede inyectar comandos. La salida estándar y la de error se leen
mediante `readyReadStandardOutput()` y `readyReadStandardError()`, se limpian de
secuencias ANSI y se envían a la interfaz.

Al finalizar se interpretan los códigos más relevantes:

| Código | Significado |
| --- | --- |
| `0` | Instalación correcta |
| `100` | Error de `apt` (dependencias, bloqueo de dpkg, etc.) |
| `125` | Error de PolicyKit |
| `126` | Ventana de autenticación cancelada |
| `127` | Autorización denegada |
| `-1` | El proceso no llegó a iniciarse o terminó de forma anómala |

## Seguridad

- La aplicación **nunca** se ejecuta como root ni ejecuta `sudo` desde la GUI.
- La elevación se delega en `pkexec`, que usa el agente de PolicyKit del
  escritorio y las reglas del sistema.
- No se construyen comandos concatenando cadenas ni se usa `system()`/shell.
- Los procesos se lanzan con rutas absolutas y argumentos separados.
- El texto de los metadatos se escapa antes de insertarse como HTML, evitando
  interpretar contenido del paquete como marcado.
- Se evita cerrar la ventana mientras hay una instalación en curso para no
  interrumpir `dpkg` a mitad de una transacción.

## Verificación manual sugerida

- [ ] Compila con CMake en Ubuntu/Debian.
- [ ] Abrir un `.deb` válido muestra todos los metadatos y sus dependencias.
- [ ] Un paquete no instalado muestra **Instalar paquete**.
- [ ] Un paquete con la misma versión instalada muestra **Reinstalar paquete**.
- [ ] Un `.deb` con versión más nueva que la instalada muestra **Actualizar
      paquete**.
- [ ] Un `.deb` con versión más antigua que la instalada deshabilita el botón y
      avisa de que ya hay una versión más reciente.
- [ ] Abrir un archivo que no es `.deb` (o corrupto) muestra un error claro.
- [ ] La interfaz sigue respondiendo mientras se lee el paquete y mientras se
      instala (se puede mover la ventana y desplazar el registro).
- [ ] Cancelar la ventana de `pkexec` produce un aviso de cancelación y no un
      error genérico.
- [ ] Instalar un paquete con dependencias pendientes las descarga e instala.
- [ ] El registro muestra la salida de `apt` en tiempo real.
- [ ] El menú de configuración cambia entre apariencia del sistema, clara y
      oscura, y la elección se conserva al reiniciar.
- [ ] **Acerca de Stellar Install** muestra el resumen y la versión correcta.
- [ ] `./packaging/install.sh` crea la entrada de menú, el icono y abre los
      archivos `.deb` con Stellar Install.
- [ ] `desktop-file-validate packaging/stellar-install.desktop` no muestra
      errores.
- [ ] `./packaging/build-portable.sh` crea la carpeta y el `.tar.gz` con Qt y
      los plugins incluidos.
- [ ] El ejecutable portable arranca desde otra carpeta sin Qt en el sistema.
- [ ] La ventana se adapta a resoluciones de 1024×768 y superiores.

## Solución de problemas

**`pkexec` no muestra ninguna ventana**

Debe haber un agente de PolicyKit activo en la sesión gráfica (GNOME, KDE, XFCE,
etc.). En sesiones sin escritorio o por SSH, `pkexec` no puede pedir credenciales
gráficamente:

```bash
pkexec --version
```

**`apt` indica que el paquete no se encuentra**

Comprueba que la ruta sea absoluta y que el archivo exista. Stellar Install usa
`QFileInfo::absoluteFilePath()` antes de llamar a `apt`.

**Error de bloqueo de dpkg**

Suele ocurrir si otra actualización está en curso. Espera a que termine o
revisa:

```bash
sudo dpkg --configure -a
sudo apt --fix-broken install
```

**La entrada de menú no aparece**

El archivo `.desktop` usa `TryExec=stellar-install`, por lo que el ejecutable
debe estar en `PATH`. En una instalación para el usuario, comprueba que
`~/.local/bin` esté incluido:

```bash
echo "$PATH" | tr ':' '\n' | grep -qx "$HOME/.local/bin" \
  && echo "PATH correcto" \
  || echo 'Añade: export PATH="$HOME/.local/bin:$PATH"'
```

**El icono no aparece en el menú**

Actualiza las cachés del escritorio después de instalar:

```bash
sudo update-desktop-database /usr/local/share/applications
sudo gtk-update-icon-cache /usr/local/share/icons/hicolor
```

## Licencia

Este proyecto se distribuye con fines educativos. Puedes adaptarlo a los
términos que prefieras.
