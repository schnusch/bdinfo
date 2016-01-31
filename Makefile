bdinfo: bdinfo.c bd.c mempool.c
	gcc -std=c99 -O0 -Wall -Wpedantic -l{avformat,avutil,bluray} -o $@ $^

clean:
	-rm -f bdinfo
