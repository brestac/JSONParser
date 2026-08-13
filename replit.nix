{ pkgs }: {
	deps = [
   pkgs.cmake
    pkgs.nano
		pkgs.clang
		pkgs.gcc10
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}