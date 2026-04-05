#!/bin/sh

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

# --- Dependencies ---

# Build dependencies
BUILD_DEPS="base-devel libxft libxinerama freetype2 fontconfig xorg-server xorg-xinit"

# Runtime dependencies
RUNTIME_DEPS="ttf-iosevka-nerd feh fcitx5 clipmenu polkit-gnome libpulse xorg-xbacklight maim xclip xsel xdotool thunar"

echo "==> Installing pacman dependencies"
sudo pacman -S --needed $BUILD_DEPS $RUNTIME_DEPS

# AUR packages (requires an AUR helper like yay or paru)
AUR_DEPS="brave-bin"

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

# --- Dotfiles ---

echo "==> Installing dwm-start"
mkdir -p "$HOME/.local/bin"
cp "$REPO_DIR/dwm-start" "$HOME/.local/bin/dwm-start"
chmod +x "$HOME/.local/bin/dwm-start"

echo "==> Installing .xprofile"
cp "$REPO_DIR/.xprofile" "$HOME/.xprofile"

echo "==> Done!"
