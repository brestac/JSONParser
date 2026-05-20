{ pkgs }: {
	deps = [
   pkgs.nano
		pkgs.clang
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}