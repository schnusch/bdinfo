{ lib, stdenv, ffmpeg, libbluray, pkg-config }:

let
  bdinfoSource = builtins.readFile ./src/bdinfo.c;
  versionMatch = builtins.match ".*#define[[:space:]]+BDINFO_VERSION[[:space:]]+\"([^\"]*)\".*" bdinfoSource;
in

stdenv.mkDerivation {
  pname = "bdinfo";
  version = builtins.head versionMatch;

  src = ./.;

  buildInputs = [ ffmpeg libbluray ];

  nativeBuildInputs = [ pkg-config ];

  installPhase = ''
    make PREFIX=$out install
  '';

  meta = {
    homepage = "https://github.com/schnusch/bdinfo";
    platforms = lib.platforms.linux;
  };
}
