#!/usr/bin/env bash
#
# Integra Stellar Install en el escritorio.
#
# Uso:
#   ./packaging/install.sh                 # instalación para el usuario actual
#   sudo ./packaging/install.sh --system   # instalación para todo el sistema
#
set -euo pipefail

APP_ID="stellar-install"
APP_NAME="Stellar Install"
APP_MIME="application/vnd.debian.binary-package"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_PATH="${SCRIPT_DIR}/$(basename -- "${BASH_SOURCE[0]}")"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
BIN_FILE="${BUILD_DIR}/${APP_ID}"
DESKTOP_FILE="${SCRIPT_DIR}/${APP_ID}.desktop"
ICON_SIZES=(16 24 32 48 64 128 256 512)
ICON_SOURCE_ROOT="${PROJECT_ROOT}/resources/icons/hicolor"

MODE="user"
SET_DEFAULT_MIME=1
ALLOW_BUILD=1

usage() {
    cat <<EOF
Uso: $(basename -- "$0") [opciones]

Instala ${APP_NAME} y registra su icono, su entrada de menú y la asociación
de archivos .deb.

Opciones:
  --user        Instala solo para el usuario actual (predeterminado).
  --system      Instala en /usr/local para todos los usuarios (usa sudo).
  --no-mime     No establece ${APP_NAME} como aplicación predeterminada
                para los archivos .deb.
  --no-build    No compila; falla si el ejecutable no existe.
  -h, --help    Muestra esta ayuda.

Ejemplos:
  ./packaging/install.sh
  sudo ./packaging/install.sh --system
EOF
}

for argument in "$@"; do
    case "$argument" in
        --user) MODE="user" ;;
        --system) MODE="system" ;;
        --no-mime) SET_DEFAULT_MIME=0 ;;
        --no-build) ALLOW_BUILD=0 ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "Opción desconocida: $argument" >&2
            echo >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ ! -f "$DESKTOP_FILE" ]]; then
    echo "No se encontró el archivo de escritorio: $DESKTOP_FILE" >&2
    exit 1
fi
for size in "${ICON_SIZES[@]}"; do
    icon_source="${ICON_SOURCE_ROOT}/${size}x${size}/apps/${APP_ID}.png"
    if [[ ! -f "$icon_source" ]]; then
        echo "No se encontró el icono: $icon_source" >&2
        exit 1
    fi
done

needs_build=0
if [[ ! -x "$BIN_FILE" ]]; then
    needs_build=1
elif [[ -n "$(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/resources" \
        "$PROJECT_ROOT/CMakeLists.txt" -newer "$BIN_FILE" -print -quit 2>/dev/null)" ]]; then
    needs_build=1
fi

if (( needs_build )); then
    if (( ! ALLOW_BUILD )); then
        echo "El ejecutable no existe o está desactualizado: $BIN_FILE" >&2
        echo "Compílalo con: cmake -S . -B build && cmake --build build -j\"\$(nproc)\"" >&2
        exit 1
    fi

    echo "Compilando ${APP_NAME}…"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

if command -v desktop-file-validate >/dev/null 2>&1; then
    if ! validation_output="$(desktop-file-validate "$DESKTOP_FILE" 2>&1)"; then
        echo "$validation_output" >&2
        exit 1
    fi
fi

install_icons() {
    local icon_root="$1"

    for size in "${ICON_SIZES[@]}"; do
        local icon_dir="${icon_root}/${size}x${size}/apps"
        install -d "$icon_dir"
        install -m 0644 \
            "${ICON_SOURCE_ROOT}/${size}x${size}/apps/${APP_ID}.png" \
            "${icon_dir}/${APP_ID}.png"
    done
}

update_caches() {
    local applications_dir="$1"
    local icon_root="$2"

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$applications_dir" >/dev/null 2>&1 || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -f -t "$icon_root" >/dev/null 2>&1 || true
    fi
}

if [[ "$MODE" == "system" ]]; then
    if (( EUID != 0 )); then
        echo "Se necesitan permisos de administrador; solicitando sudo…"
        reexec_arguments=(--system)
        if (( ! SET_DEFAULT_MIME )); then
            reexec_arguments+=(--no-mime)
        fi
        if (( ! ALLOW_BUILD )); then
            reexec_arguments+=(--no-build)
        fi
        exec sudo -- "$SCRIPT_PATH" "${reexec_arguments[@]}"
    fi

    PREFIX="${PREFIX:-/usr/local}"
    BIN_DIR="${PREFIX}/bin"
    APPLICATIONS_DIR="${PREFIX}/share/applications"
    ICON_ROOT="${PREFIX}/share/icons/hicolor"

    install -d "$BIN_DIR" "$APPLICATIONS_DIR"
    install -m 0755 "$BIN_FILE" "${BIN_DIR}/${APP_ID}"
    install -m 0644 "$DESKTOP_FILE" "${APPLICATIONS_DIR}/${APP_ID}.desktop"
    install_icons "$ICON_ROOT"

    update_caches "$APPLICATIONS_DIR" "$ICON_ROOT"

    if [[ -n "${SUDO_USER:-}" && "${SUDO_USER}" != "root" ]]; then
        if (( SET_DEFAULT_MIME )) && command -v xdg-mime >/dev/null 2>&1; then
            sudo -u "$SUDO_USER" xdg-mime default "${APP_ID}.desktop" "$APP_MIME" \
                >/dev/null 2>&1 || true
        fi
    fi

    echo "Instalado para todo el sistema en ${PREFIX}."
    echo "Puedes abrir un .deb con doble clic o desde el menú de aplicaciones."
    if (( SET_DEFAULT_MIME )) && [[ -z "${SUDO_USER:-}" ]]; then
        echo "Para asociar .deb con ${APP_NAME} para tu usuario, ejecuta:"
        echo "  xdg-mime default ${APP_ID}.desktop ${APP_MIME}"
    fi
else
    BIN_DIR="${HOME}/.local/bin"
    DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
    APPLICATIONS_DIR="${DATA_HOME}/applications"
    ICON_ROOT="${DATA_HOME}/icons/hicolor"

    install -d "$BIN_DIR" "$APPLICATIONS_DIR"
    install -m 0755 "$BIN_FILE" "${BIN_DIR}/${APP_ID}"
    install -m 0644 "$DESKTOP_FILE" "${APPLICATIONS_DIR}/${APP_ID}.desktop"
    install_icons "$ICON_ROOT"

    update_caches "$APPLICATIONS_DIR" "$ICON_ROOT"

    if (( SET_DEFAULT_MIME )) && command -v xdg-mime >/dev/null 2>&1; then
        install -d "${XDG_CONFIG_HOME:-${HOME}/.config}"
        xdg-mime default "${APP_ID}.desktop" "$APP_MIME" >/dev/null 2>&1 || true
    fi

    echo "Instalado para el usuario actual:"
    echo "  ejecutable: ${BIN_DIR}/${APP_ID}"
    echo "  menú:       ${APPLICATIONS_DIR}/${APP_ID}.desktop"
    echo "  iconos:     ${ICON_ROOT}/<tamaño>/apps/${APP_ID}.png"

    case ":${PATH}:" in
        *":${BIN_DIR}:"*) ;;
        *)
            echo
            echo "Aviso: ${BIN_DIR} no está en PATH."
            echo "Añádelo a ~/.profile para que la entrada del menú pueda iniciar la aplicación:"
            echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
            ;;
    esac
fi
