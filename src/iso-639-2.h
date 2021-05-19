/*
Copyright (C) 2016, 2018 Schnusch

This file is part of bdinfo.

bdinfo is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

bdinfo is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with bdinfo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ISO_639_2_H_INCLUDED
#define ISO_639_2_H_INCLUDED

#include <string.h>

#include "util.h"

/**
 * Convert ISO-639-2 terminology codes to bibliographic codes. (Convert localized
 * codes to international codes.) If *lang* is already a bibliographic code it
 * is returned unchanged.
 */
const char *iso6392_to_bcode(const char *lang)
{
	static const struct {
		char tcode[4];
		char bcode[4];
	} tcodes[] = {
		{"bod", "tib"},
		{"ces", "cze"},
		{"cym", "wel"},
		{"deu", "ger"},
		{"ell", "gre"},
		{"eus", "baq"},
		{"fra", "fre"},
		{"hye", "arm"},
		{"isl", "ice"},
		{"kat", "geo"},
		{"mkd", "mac"},
		{"mri", "mao"},
		{"mya", "bur"},
		{"nld", "dut"},
		{"slk", "slo"}
	};
	static const size_t numtcodes = sizeof(tcodes) / sizeof(tcodes[0]);

	size_t i = bisect_left(tcodes, lang, numtcodes, sizeof(tcodes[0]), (compar_fn)strcmp);
	if(i < numtcodes && strcmp(tcodes[i].tcode, lang) == 0)
		return tcodes[i].bcode;
	else
		return lang;
}

/**
 * Test if *lang* is a known ISO 639-2 language code.
 */
int iso6392_is_known(const char *lang)
{
	static const char bcodes[][4] = {
		"aar", "abk", "ace", "ach", "ada", "ady", "afh", "afr", "ain", "akk",
		"ale", "alt", "amh", "ang", "anp", "arc", "arg", "arm", "arn", "arp",
		"arw", "asm", "ast", "ava", "ave", "awa", "bak", "bam", "ban", "baq",
		"bas", "bej", "bel", "bem", "ben", "bho", "bin", "bis", "bla", "bos",
		"bra", "bre", "bug", "bul", "bur", "byn", "cad", "car", "cat", "ceb",
		"cha", "chb", "che", "chg", "chk", "chn", "cho", "chp", "chr", "chu",
		"chv", "chy", "cop", "cor", "cos", "crh", "csb", "cze", "dak", "dan",
		"dar", "dgr", "div", "dsb", "dua", "dum", "dut", "dyu", "dzo", "efi",
		"egy", "eka", "elx", "eng", "enm", "epo", "ewe", "ewo", "fan", "fao",
		"fat", "fij", "fil", "fin", "fon", "fre", "frm", "fro", "frr", "frs",
		"fry", "fur", "gaa", "gay", "geo", "ger", "gez", "gil", "gla", "gle",
		"glg", "glv", "gmh", "goh", "gor", "got", "grc", "gre", "gsw", "guj",
		"gwi", "hat", "hau", "haw", "heb", "her", "hil", "hin", "hit", "hmo",
		"hrv", "hsb", "hun", "hup", "iba", "ibo", "ice", "ido", "iii", "ile",
		"ilo", "ina", "ind", "inh", "ita", "jav", "jbo", "jpn", "jpr", "kaa",
		"kab", "kac", "kal", "kam", "kan", "kas", "kaw", "kaz", "kbd", "kha",
		"khm", "kho", "kik", "kin", "kir", "kmb", "kor", "kos", "krc", "krl",
		"kru", "kua", "kum", "kut", "lad", "lam", "lao", "lat", "lez", "lim",
		"lin", "lit", "lol", "loz", "ltz", "lua", "lub", "lug", "lui", "lun",
		"luo", "lus", "mac", "mad", "mag", "mah", "mai", "mak", "mal", "mao",
		"mar", "mas", "mdf", "mdr", "men", "mga", "mic", "min", "mlt", "mnc",
		"mni", "moh", "mos", "mus", "mwl", "myv", "nap", "nau", "nav", "nbl",
		"nde", "ndo", "nds", "new", "nia", "niu", "nno", "nob", "nog", "non",
		"nqo", "nso", "nwc", "nya", "nym", "nyn", "nyo", "nzi", "oci", "osa",
		"oss", "ota", "pag", "pal", "pam", "pan", "pap", "pau", "peo", "phn",
		"pli", "pol", "pon", "por", "pro", "sad", "sag", "sah", "sam", "san",
		"sas", "sat", "scn", "sco", "sel", "sga", "shn", "sid", "sin", "slo",
		"slv", "sma", "sme", "smj", "smn", "smo", "sms", "sna", "snd", "snk",
		"sog", "som", "sot", "spa", "srn", "srp", "srr", "ssw", "suk", "sun",
		"sus", "sux", "swe", "syc", "tah", "tam", "tat", "tel", "tem", "ter",
		"tet", "tgk", "tgl", "tha", "tib", "tig", "tir", "tiv", "tkl", "tlh",
		"tli", "tog", "ton", "tpi", "tsi", "tsn", "tso", "tuk", "tum", "tur",
		"tvl", "twi", "tyv", "udm", "uga", "uig", "ukr", "umb", "und", "urd",
		"vai", "ven", "vie", "vol", "vot", "wal", "war", "was", "wel", "wln",
		"wol", "xal", "xho", "yao", "yap", "yor", "zbl", "zen", "zgh", "zul",
		"zun"
	};
	static const size_t numbcodes = sizeof(bcodes) / sizeof(bcodes[0]);

	lang = iso6392_to_bcode(lang);
	size_t i = bisect_left(bcodes, lang, numbcodes, sizeof(bcodes[0]), (compar_fn)strcmp);
	return i < numbcodes && strcmp(bcodes[i], lang) == 0;
}

#endif
