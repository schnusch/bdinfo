# bdinfo

Get bluray info and extract tracks, because ffmpeg is currently unable to
extract metadata and chapters from blurays.


## Dependencies

* libbluray (optional, one may rely on the modified static libbluray
  completely)
* libxml2 (if you decide to build with modified static libbluray which is
  needed for clip names extraction)
* ffmpeg (for stream extraction)


## Installation

* run `./bootstrap` to create autoconf files
* be sure to also check out git submodules if you wish use the modified static
  libbluray
* check `./configure --help` for options
* run `./configure ...`, `make`, `make install`


## Functionality

```
$ ./bdinfo --help
Usage: ./bdinfo [OPTION]... INPUT [OUTPUT]
Get Blu-ray info and extract tracks with ffmpeg.

  -t, --time=DURATION        select all titles at least DURATION seconds long
  -p, --playlist=PLAYLIST[:ANGLE]
                             select playlist PLAYLIST and optionally only angle
                             ANGLE
  -a, --all                  do not omit duplicate titles
  -i, --info                 print more detailed information
  -c, --chapters             print chapter xml
  -f, --ffmpeg[=LANGUAGES]   print ffmpeg call to extract all or only streams of
                             given or undefined languages
  -x, --remux[=LANGUAGES]    extract all or only streams of given or undefined
                             languages with ffmpeg
  -h, --help                 display this help and exit
  -v, --version              output version information and exit
```
Where `INPUT` is the root directory of the Blu-ray or, if your distribution's
libbluray supports it, a Blu-ray image.


## Known Issues

* absolutely no copy protection circumvention
