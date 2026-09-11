# nixos-btw — the reference machine, complete.
#
# This is the one host in this repository that describes a real disk. Every
# parity claim in CLAUDE.md is checked against it, and it is the machine the
# whole repository was written on: Intel i5-1135G7, 16 GiB, eDP-1 + DP-1.
#
# Rebuild it from a clone, with no /etc/nixos in the loop:
#
#   sudo nixos-rebuild switch --flake /home/void/suckless-environment#nixos-btw
#
# Three layers, as before -- only the outer one moved here from /etc/nixos:
#
#   ../../nix/laptop.nix     the desktop and the chipset
#   this file                the installation: boot, identity, user, language
#   ./home.nix               this user: their applications
#
# hardware-configuration.nix is a SNAPSHOT of what nixos-generate-config
# wrote on this machine. It holds the UUIDs of these disks and is true only
# for as long as they are not reformatted. Refresh it with:
#
#   nixos-generate-config --show-hardware-config \
#     | nixfmt > hosts/nixos-btw/hardware-configuration.nix
#
# Anyone reproducing this desktop on OTHER hardware wants
# `nix flake init -t .#laptop' instead, which writes an /etc/nixos with no
# disks in it. See README.
{
  config,
  lib,
  pkgs,
  ...
}:

{
  imports = [
    # The desktop plus the Intel TigerLake facts.
    ../../nix/laptop.nix
    # Generated on this machine: disks, initrd modules. Do not hand-edit.
    ./hardware-configuration.nix
  ];

  # ------------------------------------------------------------------
  # Boot: systemd-boot on the ESP this machine was installed with.
  # ------------------------------------------------------------------
  boot.loader.systemd-boot.enable = true;
  boot.loader.efi.canTouchEfiVariables = true;

  # ------------------------------------------------------------------
  # Identity.
  # ------------------------------------------------------------------
  networking.hostName = "nixos-btw";
  networking.networkmanager.enable = true;
  time.timeZone = "America/Sao_Paulo";

  # nix-ld is on because this machine runs binaries built somewhere else --
  # see nix/module.nix, which is where the library list lives. Declared
  # there as mkDefault; kept here because it was here first and because it
  # is a property of how this machine is used, not of the desktop.
  programs.nix-ld.enable = true;

  # ------------------------------------------------------------------
  # The user.
  #
  #   wheel          -> sudo, and the polkit rule that mounts internal disks
  #   networkmanager -> nmcli/nmtui without sudo
  #   video, input   -> the backlight udev rules the module installs
  #
  # No initialPassword: this account already exists with a password of its
  # own, and activation must never touch it.
  # ------------------------------------------------------------------
  users.users.void = {
    isNormalUser = true;
    extraGroups = [
      "wheel"
      "networkmanager"
      "video"
      "input"
    ];
    packages = with pkgs; [ tree ];
  };

  # ------------------------------------------------------------------
  # Interface language: English.
  #
  # nix/module.nix makes this desktop Japanese-first -- ja_JP.UTF-8 with
  # LANGUAGE=ja:en -- and declares both with mkDefault exactly so an
  # installation can decide otherwise. A plain assignment here outranks it.
  #
  # This is the *interface* language and nothing else. The keyboard is a
  # separate set of options and is untouched: br/abnt2 on X and br-abnt2 on
  # the TTYs, both still owned by the module. fcitx5 + mozc stay, so typing
  # Japanese is one hotkey away.
  #
  # en_US.UTF-8 is already in i18n.supportedLocales, so this generates no
  # new locale and rebuilds no glibc-locales: it changes /etc/locale.conf
  # and nothing else.
  # ------------------------------------------------------------------
  i18n.defaultLocale = "en_US.UTF-8";
  i18n.extraLocaleSettings.LANGUAGE = "en";

  # ...and the way back, kept symmetric. The module ships
  # specialisation.english, which against the system above is now identical
  # to its own parent -- two English entries in the boot menu, one of them
  # named for the thing that no longer distinguishes it. mkForce replaces
  # the whole attribute set, so this host owns the specialisations: the
  # parent is English, and there is exactly one alternative entry.
  #
  # What mkForce costs: a specialisation added to nix/module.nix later would
  # be dropped here silently. Taken deliberately -- the module has had
  # exactly one since it was written, and it is this one, inverted.
  #
  # mkForce on the values too, and this part is not optional: the parent now
  # declares both at normal priority, and a specialisation's configuration
  # merges as just another module, so without it the definitions collide.
  #
  # Switch without rebooting, then log out and back in (a running session
  # keeps the LANG it was started with):
  #
  #   sudo /run/current-system/specialisation/japanese/bin/switch-to-configuration switch
  #
  # For a single program none of this is needed -- `LANG=ja_JP.UTF-8 emacs'
  # works as-is, because the locale is generated either way.
  specialisation = lib.mkForce {
    japanese.configuration = {
      i18n.defaultLocale = lib.mkForce "ja_JP.UTF-8";
      i18n.extraLocaleSettings = lib.mkForce { LANGUAGE = "ja:en"; };
    };
  };

  # ------------------------------------------------------------------
  # Monitor layout -- declaratively persisted.
  #
  # The dwm launcher sources ~/.config/suckless/autostart.sh on every login
  # (see nix/module.nix). This activation script regenerates that file from
  # ./autostart.sh at every switch, so the layout is part of the
  # configuration and cannot drift from it. Manual edits to
  # ~/.config/suckless/autostart.sh are overwritten -- change the layout
  # here instead, and apply it without rebooting with
  # `sudo /nix/var/nix/profiles/system/activate'.
  # ------------------------------------------------------------------
  system.activationScripts.suckless-autostart = {
    deps = [ "users" ];
    text = ''
      install -D -m 0644 -o void -g users \
        ${./autostart.sh} /home/void/.config/suckless/autostart.sh
    '';
  };

  # ------------------------------------------------------------------
  # Unfree licences.
  #
  # This lives here rather than in home.nix for a mechanical reason: the
  # flake sets home-manager.useGlobalPkgs, so home-manager receives this
  # same finished pkgs and never evaluates nixpkgs again -- `nixpkgs.config'
  # inside home.nix would be ignored in silence. This is the only place the
  # option has any effect.
  #
  # Named one by one rather than a blanket allowUnfree: these five are the
  # only unfree packages on this system -- Brave, which looks like it would
  # be, is MPL-2.0 -- and the predicate keeps it that way. A typo or a
  # future dependency does not get in for free.
  #
  # Steam needs three names rather than one because the client is a wrapper
  # around itself: `steam-unwrapped' is the real client, `steam' the FHS
  # wrapper the module below builds and runs, and `steamcmd' the headless
  # downloader in home.nix. The predicate is asked about each derivation
  # separately, so each carries its own unfree meta. `steam-run', which
  # comes along with the module, is marked free and needs no entry.
  #
  # The applications themselves are in ./home.nix: they belong to this user,
  # not to this machine.
  # ------------------------------------------------------------------
  nixpkgs.config.allowUnfreePredicate =
    pkg:
    builtins.elem (lib.getName pkg) [
      "spotify"
      "obsidian"
      "steam"
      "steam-unwrapped"
      "steamcmd"
    ];

  # A convenience alias. It goes here and not into home-manager on purpose:
  # declaring `programs.bash' for the user would make home-manager write its
  # own ~/.bashrc, which does not load the /etc/bashrc where nix/module.nix
  # injects bash/bashrc -- history, shopts, the Tokyo Night prompt, the
  # aliases and the hyfetch greeting. One line of convenience is not worth
  # trading the whole shell for.
  programs.bash.interactiveShellInit = lib.mkAfter ''
    alias btw='echo I use nixos, btw'
  '';

  # Brave is this desktop's browser; Firefox stays as a second engine.
  programs.firefox.enable = true;

  # ------------------------------------------------------------------
  # Steam.
  #
  # A SYSTEM option, and that is the whole point: `programs.steam' does not
  # exist in home-manager, so declaring it beside the user's applications
  # fails the evaluation outright -- "The option
  # `home-manager.users.void.programs.steam' does not exist".
  #
  # The placement is not a matter of taste either. The module does three
  # things no user profile can reach:
  #
  #   hardware.graphics.enable32Bit = true  the 32-bit libraries the client
  #                                         and the games link against
  #   networking.firewall.allowed{TCP,UDP}Ports
  #                                         UDP 10400/10401 and 27015/27036,
  #                                         TCP 27015/27036/27037 -- what the
  #                                         two openFirewall below turn on
  #   udev rules + steam-run + the FHS wrapper
  #                                         gamepads, and the sandbox Steam
  #                                         actually runs inside
  #
  # It is also why the client is not a package in ./home.nix: a bare
  # pkgs.steam in the user profile would be a second client with none of
  # that around it.
  # ------------------------------------------------------------------
  programs.steam = {
    enable = true;
    remotePlay.openFirewall = true;
    dedicatedServer.openFirewall = true;
  };

  # ------------------------------------------------------------------
  # Installed as 25.05. Keeping the older value means the 26.05 upgrade does
  # not quietly turn on new defaults. Bump deliberately, after reading the
  # release notes, or never.
  # ------------------------------------------------------------------
  system.stateVersion = "25.05";
}
