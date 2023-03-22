# Use `nix-shell` to quickly create a dev shell.
{ pkgs ? import <nixpkgs> {} }:
with pkgs; mkShell {
  buildInputs = [ flex bison llvmPackages_10.llvm llvmPackages_10.clang ];
  shellHook = ''
    _rootdir="$(realpath .)"
    _builddir="$_rootdir/build"
    export PATH="$_builddir:$PATH"
    rebuild(){
      (cd $_builddir; make -j)
    }
  '';
}
