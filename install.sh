#!/bin/sh
#
# suckless-environment installer (Arch + Artix only, POSIX sh)
# See .planning/phases/01-install-hardening-platform-detection/ for design notes.

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

# ---------------------------------------------------------------- config
BUILD_DEPS="base-devel git libxft libxinerama freetype2 fontconfig xorg-server xorg-xinit"
RUNTIME_DEPS="ttf-iosevka-nerd feh fcitx5 lxpolkit libpulse brightnessctl maim xclip xsel xdotool thunar pamixer dunst"
AUR_DEPS="brave-bin betterlockscreen"
# PPD_PKG, INIT, AUR_HELPER set at runtime by detect_distro / ensure_aur_helper
SUDO_KEEPALIVE_PID=""

# ---------------------------------------------------------------- color + helpers (D-14)
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    C_RED='\033[31m'; C_GREEN='\033[32m'; C_YELLOW='\033[33m'
    C_BLUE='\033[34m'; C_CYAN='\033[36m'; C_RESET='\033[0m'
else
    C_RED=''; C_GREEN=''; C_YELLOW=''; C_BLUE=''; C_CYAN=''; C_RESET=''
fi

info() { printf '==> %s%s%s\n' "$C_CYAN"   "$*" "$C_RESET"; }
skip() { printf '==> %s[skip]%s %s\n'       "$C_BLUE"  "$C_RESET" "$*"; }
warn() { printf '==> %sWARN:%s %s\n'        "$C_YELLOW" "$C_RESET" "$*" >&2; }
die()  { printf '==> %sFAIL:%s %s\n'        "$C_RED"    "$C_RESET" "$*" >&2; exit 1; }
hr()   { printf -- '-----------------------------------------------------------------\n'; }

# ---------------------------------------------------------------- guards (D-04, D-01, D-02, D-03)
require_non_root() {
    [ "$(id -u)" -eq 0 ] && \
        die "do not run as root — run as your normal user; sudo will be invoked when needed"
    return 0
}

detect_distro() {
    [ -r /etc/os-release ] || die "/etc/os-release not found"
    # shellcheck disable=SC1091
    . /etc/os-release
    case "$ID" in
        arch)  PPD_PKG="power-profiles-daemon" ;;
        artix) PPD_PKG="power-profiles-daemon-openrc" ;;
        *)     die "unsupported distro: $ID (this installer supports arch and artix only)" ;;
    esac
    if [ -d /run/openrc ]; then
        INIT=openrc
    elif [ -d /run/systemd/system ]; then
        INIT=systemd
    else
        die "no live init detected (neither /run/openrc nor /run/systemd/system) — running in a chroot?"
    fi
    command -v pacman >/dev/null 2>&1 || die "pacman not found on $ID — broken system"
    info "distro: $ID / init: $INIT / ppd: $PPD_PKG"
}

# ---------------------------------------------------------------- sudo keepalive (D-05)
sudo_keepalive_start() {
    sudo -v || die "sudo credentials required"
    (
        while true; do
            sudo -n true 2>/dev/null || exit
            sleep 60
        done
    ) &
    SUDO_KEEPALIVE_PID=$!
}

sudo_keepalive_stop() {
    if [ -n "$SUDO_KEEPALIVE_PID" ]; then
        kill "$SUDO_KEEPALIVE_PID" 2>/dev/null || :
    fi
    SUDO_KEEPALIVE_PID=""
}

ensure_aur_helper() {
    if command -v paru >/dev/null 2>&1; then
        AUR_HELPER=paru
        skip "AUR helper: paru already installed"
        return 0
    fi
    if command -v yay >/dev/null 2>&1; then
        AUR_HELPER=yay
        skip "AUR helper: yay already installed"
        return 0
    fi

    printf '%s\n' "No AUR helper found. Required for brave-bin + betterlockscreen."
    printf '%s\n' "  1) install paru (recommended)"
    printf '%s\n' "  2) install yay"
    printf '%s\n' "  3) abort"
    printf 'choose [1/2/3]: '

    if [ -t 0 ]; then
        read -r choice
    else
        read -r choice < /dev/tty
    fi

    case "$choice" in
        1) bootstrap_aur_helper paru https://aur.archlinux.org/paru.git ;;
        2) bootstrap_aur_helper yay  https://aur.archlinux.org/yay.git ;;
        3) die "aborted by user" ;;
        *) die "invalid choice: $choice" ;;
    esac
}

bootstrap_aur_helper() {
    _name=$1
    _url=$2

    info "bootstrapping $_name: installing base-devel + git prerequisites"
    # shellcheck disable=SC2086
    sudo pacman -S --needed --noconfirm base-devel git \
        || die "failed to install base-devel git (needed to build $_name)"

    command -v git     >/dev/null 2>&1 || die "git not installed after pacman step"
    command -v makepkg >/dev/null 2>&1 || die "makepkg not installed after pacman step"

    info "bootstrapping $_name from AUR: $_url"
    _tmp=$(mktemp -d) || die "mktemp failed"
    # shellcheck disable=SC2064
    trap "rm -rf '$_tmp'; sudo_keepalive_stop" EXIT

    ( cd "$_tmp" && git clone --depth 1 "$_url" "$_name" ) \
        || die "git clone $_url failed"
    ( cd "$_tmp/$_name" && makepkg -si --noconfirm ) \
        || die "makepkg for $_name failed"

    trap 'sudo_keepalive_stop' EXIT
    rm -rf "$_tmp"

    AUR_HELPER=$_name
    info "$_name installed"
}

# ---------------------------------------------------------------- AUR helper bootstrap (D-08) — added in Task 2

# ---------------------------------------------------------------- install steps — added in Plans 02/03

# ---------------------------------------------------------------- main
main() {
    require_non_root
    detect_distro
    sudo_keepalive_start
    trap 'sudo_keepalive_stop' EXIT
    trap 'sudo_keepalive_stop; exit 130' INT
    trap 'sudo_keepalive_stop; exit 143' TERM

    ensure_aur_helper

    # install_pkgs / install_udev_rule / enable_service / ensure_groups — Plan 02
    # install_binaries / install_dotfiles / verify_install — Plan 03

    info "skeleton OK (plans 02/03 fill in the rest)"
}

main "$@"
