#!/bin/sh

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

# --- Dependencies ---

# Build dependencies
BUILD_DEPS="base-devel libxft libxinerama freetype2 fontconfig xorg-server xorg-xinit"

# Runtime dependencies
RUNTIME_DEPS="ttf-iosevka-nerd feh fcitx5 polkit-gnome libpulse xorg-xbacklight maim xclip xsel xdotool thunar power-profiles-daemon dunst"

echo "==> Installing pacman dependencies"
sudo pacman -S --needed $BUILD_DEPS $RUNTIME_DEPS

# AUR packages (requires an AUR helper like yay or paru)
AUR_DEPS="brave-bin betterlockscreen"

if command -v yay >/dev/null 2>&1; then
    echo "==> Installing AUR packages with yay"
    yay -S --needed $AUR_DEPS
elif command -v paru >/dev/null 2>&1; then
    echo "==> Installing AUR packages with paru"
    paru -S --needed $AUR_DEPS
else
    echo "==> No AUR helper found. Install manually: $AUR_DEPS"
fi

# --- Build and install suckless tools ---

echo "==> Building and installing dwm"
cd "$REPO_DIR/dwm" && sudo make clean install

echo "==> Building and installing st"
cd "$REPO_DIR/st" && sudo make clean install

echo "==> Building and installing dmenu"
cd "$REPO_DIR/dmenu" && sudo make clean install

echo "==> Building and installing slstatus"
cd "$REPO_DIR/slstatus" && sudo make clean install

# --- Build and install C utilities ---

echo "==> Building C utilities"
make -C "$REPO_DIR/utils" clean all

echo "==> Installing C utilities"
make -C "$REPO_DIR/utils" install

# --- Dotfiles ---

# Note: dmenu-clip, dmenu-cpupower, dmenu-session are now C utilities
# installed by 'make -C utils install' above. Shell scripts in scripts/
# are kept as reference only.

echo "==> Installing dwm-start"
cp "$REPO_DIR/dwm-start" "$HOME/.local/bin/dwm-start"
chmod +x "$HOME/.local/bin/dwm-start"

echo "==> Installing dunst config"
mkdir -p "$HOME/.config/dunst"
cp "$REPO_DIR/dunst/dunstrc" "$HOME/.config/dunst/dunstrc"

echo "==> Installing .xprofile"
cp "$REPO_DIR/.xprofile" "$HOME/.xprofile"

echo "==> Done!"
