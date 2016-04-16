# bdinfo

Get bluray info and extract tracks, because ffmpeg is currently unable to
extract metadata and chapters from blurays.


## Dependencies

* libbluray
* ffmpeg (for stream extraction)


## Compilation

* `NO_CLIP_NAMES` do not retrieve clip names and consequently do not build with
  a static libbluray
* `NO_STRIP`


## Functionality

```
$ ./bdinfo --help
Usage: ./bdinfo [OPTION]... FILE
Get Blu-ray info and extract tracks with ffmpeg.

  -t, --time=DURATION        select all titles at least DURATION seconds long
  -p, --playlist=PLAYLIST[:ANGLE]
                             select playlist PLAYLIST and optionally only angle
                             ANGLE
  -a, --all                  do not omit duplicate titles
  -i, --info                 print more detailed information
  -c, --chapters             print chapter xml
  -f, --ffmpeg=LANGUAGES     print ffmpeg call to extract streams of given or
                             undefined languages
  -x, --remux=LANGUAGES      extract streams of given or undefined languages
                             with ffmpeg
  -h, --help     display this help and exit
  -v, --version  output version information and exit
```


## Known Issues

* absolutely no copy protection circumvention
