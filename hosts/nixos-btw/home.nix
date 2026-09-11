# home-manager for `void' on nixos-btw: what belongs to the USER rather
# than to the machine.
#
# home-manager enters as a NixOS module, not as a standalone profile, with
# useGlobalPkgs -- so it uses the SAME pkgs as the system, and the
# nixpkgs.config.allowUnfreePredicate declared in ./default.nix applies here
# too. With useUserPackages the packages land in
# /etc/profiles/per-user/void and are part of the system closure: one
# `nixos-rebuild switch' applies everything, and there is no separate
# `home-manager switch' to forget.
{ pkgs, ... }:

{
  home.username = "void";
  home.homeDirectory = "/home/void";

  # Plain packages: the ones with no home-manager module worth using.
  #
  # spotify and pavucontrol have none (what exists is
  # `programs.spotify-player', a TUI client -- a different program). The two
  # below that do have modules use them, for the same reason Thunar does in
  # nix/module.nix: the module is what builds the wrapper and writes the
  # configuration.
  home.packages = with pkgs; [
    spotify
    dosfstools
    ntfs3g

    # The graphical mixer: per-application volume and routing -- sending
    # Spotify to one sink while OBS captures another. The desktop already
    # ships pulsemixer (TUI) and pamixer (what slstatus/config.h:76 calls),
    # which cover the terminal; this is the case where pointing at it is
    # easier, which is the rule written in nix/module.nix itself.
    pavucontrol

    # Toolchains. Deliberately absent from the system module (decision 4 --
    # `nix shell nixpkgs#rust-analyzer' in the project that needs one), and
    # present here, because a language used daily is the user's call rather
    # than the machine's.
    rustc
    cargo
    rust-analyzer
    zig
    zls

    # The same argument, for the two runtimes. `npm' is not a package of
    # its own: it ships inside nodejs, and `pkgs.npm' does not exist at all
    # -- asking for it is an evaluation error, not a missing binary.
    nodejs
    bun

    libreoffice

    # The Steam CLIENT is not here. It is `programs.steam' in ./default.nix,
    # the SYSTEM module -- see the comment there for why it cannot be a user
    # package. These two are different programs and stay packages: steamcmd
    # is the headless content downloader, steam-tui a terminal front end
    # that drives it.
    steamcmd
    steam-tui
  ];

  # OBS through its module: it is what builds the wrapper that can find
  # plugins, which a bare package in the list above cannot. Plugins go here:
  #
  #   programs.obs-studio.plugins = with pkgs.obs-studio-plugins; [ obs-vkcapture ];
  #
  # The virtual camera is not from here: it is a kernel module
  # (v4l2loopback), and on NixOS it sits behind the SYSTEM obs-studio
  # module's `mkIf cfg.enable' -- turning on
  # programs.obs-studio.enableVirtualCamera would mean enabling the system
  # obs-studio too, installing a second OBS beside this one. If it is ever
  # needed, the clean path is declaring the kernel module in default.nix
  # directly, which is exactly what that module does.
  programs.obs-studio.enable = true;

  # Obsidian through its module for one concrete reason: it writes
  # `updateDisabled = true' into ~/.config/obsidian/obsidian.json. Obsidian's
  # built-in updater cannot work on NixOS -- the binary is in the read-only
  # store -- and without this it keeps trying and failing. Activation merges
  # with jq into the existing file, so vaults created through the interface
  # survive a rebuild.
  #
  # If the vaults should ever be declarative too (plugins, themes, hotkeys),
  # that is `programs.obsidian.vaults.<name>' -- empty on purpose today,
  # because nothing here asked for it.
  programs.obsidian.enable = true;

  # Deliberately NOT declaring `programs.bash' here. It would write its own
  # ~/.bashrc, which does not load the /etc/bashrc where nix/module.nix
  # injects history, shopts, the Tokyo Night prompt, the aliases and the
  # hyfetch greeting. The `btw' alias stays in default.nix for the same
  # reason.

  # Same meaning as system.stateVersion: the defaults this was written
  # against. Changing it is not an upgrade, it is a change of behaviour.
  home.stateVersion = "25.05";
}
