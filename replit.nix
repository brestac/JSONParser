{ pkgs }: {
	deps = [
   pkgs.clang_18
   pkgs.cmake
    pkgs.nano
		pkgs.llvmPackages.llvm
		pkgs.gcc10
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}