#!/usr/bin/env bash
#
# Crea una distribución portable de Stellar Install.
#
# La carpeta generada contiene el binario, las bibliotecas de Qt, sus
# dependencias y los plugins necesarios para ejecutar la aplicación sin tener
# Qt instalado en el sistema.
#
# Uso:
#   ./packaging/build-portable.sh
#   ./packaging/build-portable.sh --output /ruta/Stellar-Install-Portable
#   ./packaging/build-portable.sh --no-archive
#
set -euo pipefail

APP_ID="stellar-install"
APP_NAME="Stellar Install"
ARCH="$(uname -m)"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
PORTABLE_BUILD_DIR="${PROJECT_ROOT}/build-portable"

APP_VERSION="$(sed -n 's/^project(StellarInstall VERSION \([^ ]*\).*/\1/p' \
    "${PROJECT_ROOT}/CMakeLists.txt")"
if [[ -z "$APP_VERSION" ]]; then
    APP_VERSION="1.0.0"
fi

DEFAULT_OUTPUT="${PROJECT_ROOT}/dist/${APP_NAME// /-}-${APP_VERSION}-linux-${ARCH}-portable"
OUTPUT_DIR="$DEFAULT_OUTPUT"
DO_BUILD=1
DO_STRIP=1
MAKE_ARCHIVE=1
JOBS="$(nproc)"

usage() {
    cat <<EOF
Uso: $(basename -- "$0") [opciones]

Genera una carpeta portable con el ejecutable de ${APP_NAME}, las bibliotecas
de Qt y los plugins necesarios. También crea un archivo .tar.gz.

Opciones:
  --output DIR    Carpeta de salida (predeterminado: dist/...-portable).
  --no-build      No compila; usa build-portable/stellar-install.
  --no-strip      No elimina símbolos de depuración.
  --no-archive    No crea el archivo .tar.gz.
  --jobs N        Número de trabajos de compilación (predeterminado: $(nproc)).
  -h, --help      Muestra esta ayuda.

Ejemplos:
  ./packaging/build-portable.sh
  ./packaging/build-portable.sh --output /tmp/Stellar-Install-Portable
EOF
}

require_argument() {
    if (( $# < 2 )) || [[ -z "$2" ]]; then
        echo "Falta el valor para $1" >&2
        echo >&2
        usage >&2
        exit 2
    fi
}

while (( $# > 0 )); do
    case "$1" in
        --output)
            require_argument "$@"
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --no-build) DO_BUILD=0; shift ;;
        --no-strip) DO_STRIP=0; shift ;;
        --no-archive) MAKE_ARCHIVE=0; shift ;;
        --jobs)
            require_argument "$@"
            JOBS="$2"
            shift 2
            ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "Opción desconocida: $1" >&2
            echo >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ "$OUTPUT_DIR" != /* ]]; then
    OUTPUT_DIR="${PWD}/${OUTPUT_DIR}"
fi
case "$OUTPUT_DIR" in
    ""|"/"|"/."|"$HOME")
        echo "Ruta de salida no válida: $OUTPUT_DIR" >&2
        exit 1
        ;;
esac

BIN_FILE="${PORTABLE_BUILD_DIR}/${APP_ID}"

if (( DO_BUILD )); then
    echo "Compilando ${APP_NAME} con soporte portable…"
    cmake -S "$PROJECT_ROOT" -B "$PORTABLE_BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSTELLAR_PORTABLE_RPATH=ON
    cmake --build "$PORTABLE_BUILD_DIR" -j"$JOBS"
fi

if [[ ! -x "$BIN_FILE" ]]; then
    echo "No se encontró el ejecutable: $BIN_FILE" >&2
    echo "Compílalo con: ./packaging/build-portable.sh" >&2
    exit 1
fi

if command -v readelf >/dev/null 2>&1; then
    if ! readelf -d "$BIN_FILE" 2>/dev/null | grep -q 'RPATH.*\$ORIGIN/lib'; then
        echo "El binario no tiene el RPATH portable \$ORIGIN/lib." >&2
        echo "Recompílalo sin --no-build para poder distribuir esta carpeta." >&2
        exit 1
    fi
fi

QMAKE_BIN=""
for candidate in qmake6 qmake-qt6 qmake; do
    if command -v "$candidate" >/dev/null 2>&1; then
        QMAKE_BIN="$candidate"
        break
    fi
done
if [[ -z "$QMAKE_BIN" ]]; then
    echo "No se encontró qmake para localizar las bibliotecas y plugins de Qt." >&2
    exit 1
fi

QT_LIBS="$("$QMAKE_BIN" -query QT_INSTALL_LIBS)"
QT_PLUGINS="$("$QMAKE_BIN" -query QT_INSTALL_PLUGINS)"
QT_VERSION="$("$QMAKE_BIN" -query QT_VERSION)"
QT_MAJOR="${QT_VERSION%%.*}"

if [[ ! -d "$QT_PLUGINS" ]]; then
    for candidate in "$QT_LIBS/qt${QT_MAJOR}/plugins" "$QT_LIBS/qt/plugins"; do
        if [[ -d "$candidate" ]]; then
            QT_PLUGINS="$candidate"
            break
        fi
    done
fi
if [[ ! -d "$QT_PLUGINS" ]]; then
    echo "No se encontró el directorio de plugins de Qt: $QT_PLUGINS" >&2
    exit 1
fi

echo "Qt ${QT_VERSION}"
echo "Bibliotecas: ${QT_LIBS}"
echo "Plugins:     ${QT_PLUGINS}"
echo "Salida:      ${OUTPUT_DIR}"

PORTABLE_MARKER=".stellar-install-portable"
if [[ -e "$OUTPUT_DIR" ]]; then
    if [[ -f "${OUTPUT_DIR}/${PORTABLE_MARKER}" ]]; then
        rm -rf -- "$OUTPUT_DIR"
    elif [[ -d "$OUTPUT_DIR" && -z "$(ls -A "$OUTPUT_DIR")" ]]; then
        rmdir -- "$OUTPUT_DIR"
    else
        echo "La carpeta de salida ya existe y no parece una distribución portable" >&2
        echo "generada por este script: ${OUTPUT_DIR}" >&2
        echo "Elimínala manualmente o indica otra ruta con --output." >&2
        exit 1
    fi
fi

mkdir -p "$OUTPUT_DIR/lib" "$OUTPUT_DIR/plugins"
printf 'Generado por packaging/build-portable.sh\n' > "${OUTPUT_DIR}/${PORTABLE_MARKER}"

install -m 0755 "$BIN_FILE" "${OUTPUT_DIR}/${APP_ID}"
install -m 0644 "${PROJECT_ROOT}/resources/icons/hicolor/256x256/apps/stellar-install.png" \
    "${OUTPUT_DIR}/stellar-install.png"

cat > "${OUTPUT_DIR}/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
Libraries = lib
EOF

PLUGIN_FILES=()
PLUGINS=(
    "platforms/libqxcb.so"
    "platforms/libqwayland.so"
    "platforms/libqwayland-egl.so"
    "platforms/libqwayland-generic.so"
    "platforms/libqoffscreen.so"
    "platforms/libqminimal.so"
    "platforminputcontexts/libcomposeplatforminputcontextplugin.so"
    "wayland-shell-integration/libxdg-shell.so"
    "wayland-shell-integration/libfullscreen-shell-v1.so"
    "wayland-shell-integration/libwl-shell-plugin.so"
    "wayland-decoration-client/libbradient.so"
    "wayland-decoration-client/libadwaita.so"
    "wayland-graphics-integration-client/libqt-plugin-wayland-egl.so"
    "wayland-graphics-integration-client/libdmabuf-server.so"
    "wayland-graphics-integration-client/libshm-emulation-server.so"
    "xcbglintegrations/libqxcb-glx-integration.so"
    "xcbglintegrations/libqxcb-egl-integration.so"
    "imageformats/libqgif.so"
    "imageformats/libqico.so"
    "imageformats/libqjpeg.so"
    "imageformats/libqtga.so"
    "imageformats/libqtiff.so"
    "imageformats/libqwbmp.so"
    "imageformats/libqwebp.so"
)

echo "Copiando plugins…"
for plugin in "${PLUGINS[@]}"; do
    plugin_source="${QT_PLUGINS}/${plugin}"
    if [[ ! -f "$plugin_source" ]]; then
        echo "  omitido (no existe): ${plugin}" >&2
        continue
    fi

    plugin_dir="${OUTPUT_DIR}/plugins/$(dirname -- "$plugin")"
    mkdir -p "$plugin_dir"
    cp -L --preserve=mode,timestamps "$plugin_source" "$plugin_dir/"
    PLUGIN_FILES+=("${OUTPUT_DIR}/plugins/${plugin}")
done

if [[ ! -f "${OUTPUT_DIR}/plugins/platforms/libqxcb.so" &&
      ! -f "${OUTPUT_DIR}/plugins/platforms/libqwayland.so" ]]; then
    echo "No se copió ningún plugin de plataforma (xcb o wayland)." >&2
    exit 1
fi

should_exclude() {
    local name="$1"
    case "$name" in
        ld-linux*.so*|ld64.so*|linux-vdso.so*)
            return 0 ;;
        libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*)
            return 0 ;;
        libresolv.so.*|libutil.so.*|libnsl.so.*|libanl.so.*)
            return 0 ;;
        libBrokenLocale.so.*|libthread_db.so.*|libmemusage.so.*|libpcprofile.so.*)
            return 0 ;;
        libGL.so.*|libGLX.so.*|libGLdispatch.so.*|libOpenGL.so.*)
            return 0 ;;
        libEGL.so.*|libGLESv2.so.*|libdrm.so.*|libgbm.so.*|libvulkan.so.*)
            return 0 ;;
        libnvidia-*.so*|libcuda.so*|libnvcuvid.so*)
            return 0 ;;
    esac
    return 1
}

declare -A COPIED_LIBS=()
LIBS_COPIED=()

copy_dependency() {
    local path="$1"
    path="${path%% (*}"

    [[ -n "$path" && -f "$path" ]] || return 0

    local name
    name="$(basename -- "$path")"
    if should_exclude "$name"; then
        return 0
    fi
    if [[ -n "${COPIED_LIBS[$name]:-}" ]]; then
        return 0
    fi

    cp -L --preserve=mode,timestamps "$path" "${OUTPUT_DIR}/lib/${name}"
    COPIED_LIBS["$name"]=1
    LIBS_COPIED+=("$name")
}

collect_dependencies() {
    local elf="$1"
    local output
    output="$(LC_ALL=C ldd "$elf" 2>&1)" || {
        echo "No se pudo analizar ${elf}:" >&2
        echo "$output" >&2
        return 1
    }

    if grep -q 'not found' <<<"$output"; then
        echo "Dependencias no encontradas para ${elf}:" >&2
        grep 'not found' <<<"$output" >&2
        return 1
    fi

    local path
    while IFS= read -r path; do
        copy_dependency "$path"
    done < <(awk '/=>/ && $3 ~ /^\// { print $3 }
                   $1 ~ /^\// { print $1 }' <<<"$output" | sort -u)
}

echo "Copiando bibliotecas…"
collect_dependencies "${OUTPUT_DIR}/${APP_ID}"
for plugin in "${PLUGIN_FILES[@]}"; do
    collect_dependencies "$plugin"
done

if (( DO_STRIP )) && command -v strip >/dev/null 2>&1; then
    echo "Eliminando símbolos de depuración…"
    strip --strip-unneeded "${OUTPUT_DIR}/${APP_ID}" 2>/dev/null || true
    while IFS= read -r -d '' file; do
        strip --strip-unneeded "$file" 2>/dev/null || true
    done < <(find "${OUTPUT_DIR}/lib" "${OUTPUT_DIR}/plugins" \
        -type f \( -name '*.so' -o -name '*.so.*' \) -print0)
fi

cat > "${OUTPUT_DIR}/LEEME.txt" <<EOF
${APP_NAME} ${APP_VERSION} — distribución portable (${ARCH})

Contenido
---------
  ${APP_ID}   Ejecutable de la aplicación.
  lib/        Bibliotecas de Qt y sus dependencias.
  plugins/    Plugins de Qt (X11, Wayland, imágenes, etc.).
  qt.conf     Indica a Qt dónde están los plugins y las bibliotecas.
  stellar-install.png  Icono de la aplicación.

Uso
---
  ./${APP_ID}
  ./${APP_ID} /ruta/al/paquete.deb

También se puede ejecutar con doble clic desde el gestor de archivos.

Requisitos del sistema
----------------------
  - Linux ${ARCH} con glibc.
  - Un entorno gráfico X11 o Wayland.
  - Controladores gráficos del sistema (OpenGL/EGL/Vulkan).
  - Para instalar paquetes: dpkg-deb, apt y pkexec, que forman parte del
    sistema operativo y se usan con la autenticación habitual.

Esta carpeta incluye Qt ${QT_VERSION} y los plugins necesarios. No es necesario
tener Qt instalado en el sistema. Se excluyen deliberadamente glibc, el cargador
dinámico y la pila gráfica (OpenGL/EGL/Vulkan) para usar los del sistema y
mantener la compatibilidad con los controladores de cada equipo.

La compatibilidad binaria depende de la arquitectura (${ARCH}) y de la versión
de glibc del equipo donde se generó esta distribución. Para equipos con una
glibc más antigua, genera la carpeta portable en una versión de Debian/Ubuntu
igual o más antigua que la del equipo de destino.
EOF

if (( MAKE_ARCHIVE )); then
    ARCHIVE="${OUTPUT_DIR}.tar.gz"
    echo "Creando ${ARCHIVE}…"
    tar -C "$(dirname -- "$OUTPUT_DIR")" -czf "$ARCHIVE" "$(basename -- "$OUTPUT_DIR")"
fi

echo
echo "Distribución portable creada:"
echo "  carpeta: ${OUTPUT_DIR}"
echo "  tamaño:  $(du -sh "$OUTPUT_DIR" | cut -f1)"
echo "  libs:    ${#LIBS_COPIED[@]}"
echo "  plugins: ${#PLUGIN_FILES[@]}"
if (( MAKE_ARCHIVE )); then
    echo "  archivo: ${ARCHIVE}"
fi
