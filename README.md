# bdinfo

Get bluray info and extract tracks, because ffmpeg is currently unable to
extract metadata and chapters from blurays.


## Dependencies

* libbluray
* libavformat (ffmpeg)
* ffmpeg (for stream extraction)


## Compilation

Optional macros `NO_CLIP_NAMES` and `NO_LIBAVFORMAT`.

* `NO_CLIP_NAMES` disable linux specific clip name mapping via `/proc/self/fd/`
* `NO_LIBAVFORMAT` disable dependency on libavformat at the cost of accurate
  ffmpeg stream mapping (currently does not even build)


## Functionality

```
$ ./bdinfo --help
Usage: ./bdinfo [OPTION]... FILE
Get BluRay info and extract tracks with ffmpeg.

  -l, --list[=DURATION]      list all tracks at least DURATION seconds
                             long (default)
  -p, --playlist=PLAYLIST    use playlist PLAYLIST
  -a, --angle=ANGLE          use angle ANGLE
  -c, --chapters             print chapter xml and exit (-p required)
  -f, --ffmpeg=LANGUAGES     create a ffmpeg call to extract all given languages
                             (-p required)
  -x, --remux=LANGUAGES      extract all given language's streams and chatpers
                             with ffmpeg (-p required)
  -h, --help     display this help and exit
  -v, --version  output version information and exit
```


## Known Issues

* absolutely no copy protection circumvention
