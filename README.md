# bdinfo

Get bluray info and extract tracks including language metadata and chapters,
which ffmpeg is currently unable to do.


## Dependencies

* libbluray >= 1.0.0
* ffmpeg (for remuxing)


## Installation

* `make [PKGCONF=<pkgconf>]`
* `make [PREFIX=<prefix>] [DESTDIR=<destdir>] [INSTALL=<install>] install`


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
  -c, --chapters             print XML chapters
  -f, --ffmpeg[=LANGUAGES]   print ffmpeg call to extract all or only streams of
                             given or undefined languages
  -x, --remux[=LANGUAGES]    extract all or only streams of given or undefined
                             languages with ffmpeg
  -L, --lossless             transcode lossless audio tracks to flac
  -s, --skip-igs             skip interactive graphic streams on extraction
  -h, --help                 display this help and exit
  -v, --version              output version information and exit
```
Where `INPUT` is the root directory of the Blu-ray or, if your distribution's
libbluray supports it, a Blu-ray image.
