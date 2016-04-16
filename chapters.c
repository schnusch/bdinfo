#include <inttypes.h>
#include <stdio.h>

#include "chapters.h"
#include "util.h"

static int print_chapters(BLURAY_TITLE_INFO *ti, const char *head,
		int (*print_chapter)(BLURAY_TITLE_CHAPTER *), const char *tail)
{
	if(fputs(head, stdout) == EOF)
		return -1;
	for(uint32_t i = 0; i < ti->chapter_count; i++)
		if(print_chapter(&ti->chapters[i]) == -1)
			return -1;
	if(tail != NULL)
		if(fputs(tail, stdout) == EOF)
			return -1;
	return 0;
}

static int print_xml_chapter(BLURAY_TITLE_CHAPTER *chap)
{
	if(printf("\t\t<ChapterAtom>\n"
			"\t\t\t<ChapterTimeStart>%s</ChapterTimeStart>\n"
			"\t\t\t<ChapterFlagHidden>0</ChapterFlagHidden>\n"
			"\t\t\t<ChapterFlagEnabled>1</ChapterFlagEnabled>\n"
			"\t\t</ChapterAtom>\n", ticks2time(chap->start)) < 0)
		return -1;
	return 0;
}

int print_xml_chapters(BLURAY_TITLE_INFO *ti)
{
	return print_chapters(ti,
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<Chapters>\n"
			"\t<EditionEntry>\n"
			"\t\t<EditionFlagHidden>0</EditionFlagHidden>\n"
			"\t\t<EditionFlagDefault>0</EditionFlagDefault>\n",
			&print_xml_chapter,
			"\t</EditionEntry>\n"
			"</Chapters>\n");
}

static int print_ff_chapter(BLURAY_TITLE_CHAPTER *chap)
{
	if(printf("[CHAPTER]\n"
			"TIMEBASE=1/90000\n"
			"START=%" PRIu64 "\n"
			"END=%" PRIu64 "\n",
			chap->start, chap->start + chap->duration) < 0)
		return -1;
	return 0;
}

int print_ff_chapters(BLURAY_TITLE_INFO *ti)
{
	return print_chapters(ti, ";FFMETADATA1\n", &print_ff_chapter, NULL);
}
