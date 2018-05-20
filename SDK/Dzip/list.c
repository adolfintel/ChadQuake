#include "dzip.h"
#include "dzipcon.h"

void dzList (char *src)
{
	int i;
	direntry_t *de;

	if (!dzOpen(src, 0))
		return;

	Q_printf("contents of %s, created using version %u.%u:\n",
		src, maj_ver, min_ver);
	de = directory;
	for (i = 0; i < numfiles; i++, de++)
	{
		if (de->type == TYPE_DIR) 
		{
			Q_printf(" %s\n", de->name);
			continue;
		}

		if (de->pak && de->type != TYPE_PAK)
		{
			Q_printf("  %-17s size: %-8u packed: %-8u\n", 
				de->name, de->real, de->size);
			continue;
		}
		Q_printf(" %-18s size: %-8u packed: %-8u", 
			de->name, de->real, de->size);
		if (maj_ver != 1)
			Q_printf(" %02u.%02u.%4u %02u:%02u:%02u",
				(de->date >> 16) & 0x1f,
				((de->date >> 21) & 0x0f) + 1,
				((de->date >> 25) & 0x7f) + 1980,
				(de->date >> 11) & 0x1f,
				(de->date >> 5) & 0x3f, (de->date & 0x1f) << 1);
		Q_printf("\n");
	}
	Q_printf("\n");
	dzClose();
}
