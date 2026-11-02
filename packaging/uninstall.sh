#!/usr/bin/env bash
#
# Quita la integración de Stellar Install del escritorio.
#
# Uso:
#   ./packaging/uninstall.sh                 # instalación del usuario actual
#   sudo ./packaging/uninstall.sh --system   # instalación para todo el sistema
#
set -euo pipefail

APP_ID="stellar-install"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_PATH="${SCRIPT_DIR}/$(basename -- "${BASH_SOURCE[0]}")"

MODE="user"

usage() {
    cat <<EOF
Uso: $(basename -- "$0") [opciones]

Elimina el ejecutable, la entrada de menú y el icono instalados por
install.sh. No elimina la configuración ni los paquetes instalados con la
aplicación.

Opciones:
  --user        Desinstala solo para el usuario actual (predeterminado).
  --system      Desinstala de /usr/local para todos los usuarios (usa sudo).
  -h, --help    Muestra esta ayuda.
EOF
}

for argument in "$@"; do
    case "$argument" in
        --user) MODE="user" ;;
        --system) MODE="system" ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "Opción desconocida: $argument" >&2
            echo >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ "$MODE" == "system" ]]; then
    if (( EUID != 0 )); then
        echo "Se necesitan permisos de administrador; solicitando sudo…"
        exec sudo -- "$SCRIPT_PATH" --system
    fi

    PREFIX="${PREFIX:-/usr/local}"
    BIN_FILE="${PREFIX}/bin/${APP_ID}"
    APPLICATIONS_DIR="${PREFIX}/share/applications"
    ICON_ROOT="${PREFIX}/share/icons/hicolor"
else
    BIN_FILE="${HOME}/.local/bin/${APP_ID}"
    DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
    APPLICATIONS_DIR="${DATA_HOME}/applications"
    ICON_ROOT="${DATA_HOME}/icons/hicolor"
fi

rm -f "$BIN_FILE" "${APPLICATIONS_DIR}/${APP_ID}.desktop"
for size in 16 24 32 48 64 128 256 512; do
    rm -f "${ICON_ROOT}/${size}x${size}/apps/${APP_ID}.png"
done
rm -f "${ICON_ROOT}/scalable/apps/${APP_ID}.svg"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APPLICATIONS_DIR" >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$ICON_ROOT" >/dev/null 2>&1 || true
fi

echo "Se eliminaron los archivos de integración de Stellar Install."
echo "Nota: la asociación de archivos .deb puede seguir apuntando a"
echo "${APP_ID}.desktop; elige otra aplicación con «Abrir con» o con xdg-mime."
