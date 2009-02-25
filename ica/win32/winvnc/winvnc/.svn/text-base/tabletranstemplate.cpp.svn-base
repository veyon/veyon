/*
 * tabletranstemplate.c - template for translation using lookup tables.
 *
 * This file shouldn't be compiled.  It is included multiple times by
 * translate.c, each time with different definitions of the macros IN and OUT.
 *
 * For each pair of values IN and OUT, this file defines two functions for
 * translating a given rectangle of pixel data.  One uses a single lookup
 * table, and the other uses three separate lookup tables for the red, green
 * and blue values.
 *
 * I know this code isn't nice to read because of all the macros, but
 * efficiency is important here.
 */

#if !defined(INVNC) || !defined(OUTVNC)
#error "This file shouldn't be compiled."
#error "It is included as part of translate.c"
#endif

#define IN_T CONCAT2E(CARD,INVNC)
#define OUT_T CONCAT2E(CARD,OUTVNC)
#define rfbTranslateWithSingleTableINVNCtoOUTVNC \
				CONCAT4E(rfbTranslateWithSingleTable,INVNC,to,OUTVNC)
#define rfbTranslateWithRGBTablesINVNCtoOUTVNC \
				CONCAT4E(rfbTranslateWithRGBTables,INVNC,to,OUTVNC)

/*
 * rfbTranslateWithSingleTableINVNCtoOUTVNC translates a rectangle of pixel data
 * using a single lookup table.
 */

static void
rfbTranslateWithSingleTableINVNCtoOUTVNC (char *table, rfbPixelFormat *in,
				    rfbPixelFormat *out,
				    char *iptr, char *optr,
				    int bytesBetweenInputLines,
				    int width, int height)
{
    IN_T *ip = (IN_T *)iptr;
    OUT_T *op = (OUT_T *)optr;
    int ipextra = bytesBetweenInputLines / sizeof(IN_T) - width;
    OUT_T *opLineEnd;
    OUT_T *t = (OUT_T *)table;

    while (height > 0) {
	opLineEnd = op + width;

	while (op < opLineEnd) {
	    *(op++) = t[*(ip++)];
	}

	ip += ipextra;
	height--;
    }
}


/*
 * rfbTranslateWithRGBTablesINVNCtoOUTVNC translates a rectangle of pixel data
 * using three separate lookup tables for the red, green and blue values.
 */

static void
rfbTranslateWithRGBTablesINVNCtoOUTVNC (char *table, rfbPixelFormat *in,
				  rfbPixelFormat *out,
				  char *iptr, char *optr,
				  int bytesBetweenInputLines,
				  int width, int height)
{
    IN_T *ip = (IN_T *)iptr;
    OUT_T *op = (OUT_T *)optr;
    int ipextra = bytesBetweenInputLines / sizeof(IN_T) - width;
    OUT_T *opLineEnd;
    OUT_T *redTable = (OUT_T *)table;
    OUT_T *greenTable = redTable + in->redMax + 1;
    OUT_T *blueTable = greenTable + in->greenMax + 1;

    while (height > 0) {
	opLineEnd = op + width;

	while (op < opLineEnd) {
	    *(op++) = (redTable[(*ip >> in->redShift) & in->redMax] |
		       greenTable[(*ip >> in->greenShift) & in->greenMax] |
		       blueTable[(*ip >> in->blueShift) & in->blueMax]);
	    ip++;
	}
	ip += ipextra;
	height--;
    }
}

#undef IN_T
#undef OUT_T
#undef rfbTranslateWithSingleTableINVNCtoOUTVNC
#undef rfbTranslateWithRGBTablesINVNCtoOUTVNC
