#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int binInfo();

int main() {
	printf("**At any point, enter an invalid character to exit the program**\n\n");
	printf("  What file would you like to edit? (r, 0, 1 ... e, f)\n\n->");
	char file;
	fflush(stdout);
	fflush(stdin);
	scanf("%c", &file);
	if (file == 'r') {
		//rom
		binInfo();
		FILE* f = fopen(".\\data\\r.rom", "wb");
		int buffer;
		unsigned char c;
		while (scanf("%x", &buffer) != 0) {
			if (buffer >= 0 && buffer <= 255) {
				c = buffer;
				fwrite(&c, 1, 1, f);
				printf("->");
			} else {
				printf("  number ignored, out of range\n->");
			}
			fflush(stdout);
		}

		fclose(f);
	} else if ((file >= 48 && file <= 57) || (file >= 97 && file <= 102)){
		//      0-9                           a-f
		//disk
		char diskIdChar[1];
		char path[50];
		sprintf(diskIdChar, "%c", file);
		strcpy(path, ".\\data\\");
		strcat(path, diskIdChar);
		strcat(path, ".dsk");

		printf("\n  What would you like to do with disk %c? \n\n"
				"  Format:         x\n"
				"  Rewrite sector: 0, 1 ... e, f\n\n->", file);

		fflush(stdout);
		fflush(stdin);
		char sector;
		scanf("%c", &sector);
		if (sector == 'x') {
			//format
			FILE* f = fopen(path, "wb");
			char nothing = 0;
			for (int i = 0; i < 0x10000; i++) {
				fwrite(&nothing, 1, 1, f);
			}

			printf("\n  Disk formatting complete!\n");
			fflush(stdout);
		} else if ((sector >= 48 && sector <= 57) || (sector >= 97 && sector <= 102)) {
			//      0-9                           a-f
			//sector
			binInfo();
			FILE* f = fopen(path, "rb+");
			char behave[2];
			memset(behave, sector, 1);

			fseek(f, strtol(behave, NULL, 16) * 0x1000, SEEK_SET);

			// Write
			int buffer;
			unsigned char c;
			int invalidBreak = 0;
			unsigned char nothing = 0;
			for (int i = 0; i < 0x1000; i++) {
				if (invalidBreak) {
					fwrite(&nothing, 1, 1, f);
				} else if (scanf("%x", &buffer) != 0) {
					if (buffer >= 0 && buffer <= 255) {
						c = buffer;
						fwrite(&c, 1, 1, f);
						printf("->");
					} else {
						printf("  number ignored, out of range\n->");
					}
					fflush(stdout);
				} else {
					//invalid, exit
					invalidBreak = 1;
				}
			}
			if (!invalidBreak) {
				printf("\n\n  Sector size limit reached\n");
				fflush(stdout);
				fflush(stdin);
			}
		}
	}


	printf("\n  Program terminated, press enter to exit");
	fflush(stdout);
	fflush(stdin);
	char z;
	scanf("%c", &z);
	return EXIT_SUCCESS;
}

int binInfo() {
	printf("\n  Binary writer: input hexadecimal numbers (lowercase, no leading 0x, from 00 to FF)\n");
	printf("  Enter a non-hex letter to exit\n\n->");
	fflush(stdout);
	return 0;
}















