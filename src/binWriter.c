#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int binInfo();

int main() {
	printf("**At any point, enter an invalid character to finish and return to main menu**\n");
	char file, input;

	// Main loop
	while (1) {

		// "Main menu"
		printf("\n  Manual mode or batch mode? (M/B)\n\n->");
		fflush(stdout);
		fflush(stdin);
		scanf("%c", &input);

		if (input == 'b' || input == 'B') {
			// Batch mode

			printf("\n  Enter the name of the batch file (include relative path and "
					"file extension)\n\n->");
			fflush(stdout);
			fflush(stdin);
			char batchFilePath[100], batchFileCompletePath[100];
			scanf("%[^\n]", batchFilePath);

			strcpy(batchFileCompletePath, ".\\");
			strcat(batchFileCompletePath, batchFilePath);

			FILE* batchFile = fopen(batchFileCompletePath, "r");

			if (batchFile == NULL) {
				printf("\n  Batch file not found!\n");
			} else {
				char disk[2];
				unsigned char sector;
				int format;
				printf("\n");

				// For every line in the batch file
				while (1) {
					// Read disk and sector info
					if (!fread(disk, 1, 1, batchFile) || !fread(&sector, 1, 1, batchFile)) {
						// Reached EOF
						printf("\n  Succesfully executed batch file!\n");
						break;
					}
					int isRom = 0;

					if (disk[0] == 114) {
						// Rom
						isRom = 1;
					} else if (!(((disk[0] >= '0' && disk[0] <= '9') || (disk[0] >= 'a' && disk[0] <= 'f')) &&
							((sector >= '0' && sector <= '9') || (sector >= 'a' && sector <= 'f') || sector == 'x'))) {
						// Validating disk and sector formatting
						printf("\n  Invalid disk/sector format!\n");
						break;
					}

					format = 0;
					if (sector == 'x') {
						printf("Formatting %c\n", disk[0]);
						fscanf(batchFile, "%c", &sector); // Reading through the ; and \n at EOL
						fscanf(batchFile, "%c", &sector);
						format = 1;
						sector = 0;
					} else if (isRom) {
						sector = 0;
					}

					char targetDiskPath[100];

					if (isRom) {
						strcpy(targetDiskPath, ".\\data\\r.rom");
					} else {
						strcpy(targetDiskPath, ".\\data\\");
						disk[1] = '\0';
						strcat(targetDiskPath, disk);
						strcat(targetDiskPath, ".dsk");
					}

					FILE* targetDisk;

					if (format) {
						targetDisk = fopen(targetDiskPath, "wb");
					} else {
						targetDisk = fopen(targetDiskPath, "rb+");
					}

					if (targetDisk == NULL) {
						printf("\n An error occurred, verify that all data files are present!\n");
						printf(targetDiskPath);
						break;
					}

					if (!format) {
						char tmpStringBuffer[2]; // So that we can use strtol to convert hex ascii to an int
						tmpStringBuffer[1] = '\0';
						memset(tmpStringBuffer, sector, 1);

						fseek(targetDisk, strtol(tmpStringBuffer, NULL, 16) * 0x1000, SEEK_SET);
					}


					// Write
					char sourceFilePath[100];
					char sourceFileCompletePath[100];
					FILE* sourceFile = NULL;

					int buffer;
					unsigned char c;
					int invalidBreak = 0;
					int fileErrorBreak = 0;
					int lastFile = 0;
					int scanResult = 0;
					int doneBreak = 1;
					unsigned char nothing = 0;
					// Filling the targetfile
					for (int i = 0; (isRom && i < 0x4000) || (!isRom &&
							((!format && i < 0x1000) || (format && i < 0x10000))); i++) {
						if (doneBreak && !lastFile && !format) {
							doneBreak = 0;
							// Find next source file
							fscanf(batchFile, "%s", sourceFilePath);

							// Checking to see if we just loaded the last file for the target
							for (int j = 0; j < 100; j++) {
								if (sourceFilePath[j] == ';') {
									// Last file!
									sourceFilePath[j] = '\0';
									lastFile = 1;
									fscanf(batchFile, "%c", &c); // Reading through the \n at EOL
									break;
								}
							}

							strcpy(sourceFileCompletePath, "./");
							strcat(sourceFileCompletePath, sourceFilePath);
							if (sourceFile != NULL) {
								fclose(sourceFile);
							}
							sourceFile = fopen(sourceFileCompletePath, "r");

							printf("Reading \"%s\"\n", sourceFileCompletePath);

						}

						if (sourceFile == NULL && !format) {
							fileErrorBreak = 1;
						}

						if (invalidBreak || fileErrorBreak || doneBreak || format) {
							// If something went wrong or we're formatting, we want to finish writing all 0's
							fwrite(&nothing, 1, 1, targetDisk);
						} else {
							scanResult = fscanf(sourceFile, "%x", &buffer);
							if (scanResult == EOF) {
								doneBreak = 1;
								i--; // nothing was written, so take back the for loop's increment
							} else if (scanResult != 0 && (buffer >= 0 && buffer <= 255)) {
								c = buffer;
								fwrite(&c, 1, 1, targetDisk);
							} else {
								// invalid, exit
								invalidBreak = 1;
							}
						}
					}

					fclose(targetDisk);

					if (fileErrorBreak) {
						printf("\n  Source file not found!\n");
						if (sourceFile != NULL) {
							fclose(sourceFile);
						}
						break;
					}

					if (invalidBreak) {
						printf("\n  Invalid character found in a source file!\n");
						if (sourceFile != NULL) {
							fclose(sourceFile);
						}
						break;
					}

					if (!doneBreak && (fscanf(sourceFile, "%x", &buffer) != EOF || !lastFile) && !format) {
						printf("\n  File size limit exceded!\n");
						if (sourceFile != NULL) {
							fclose(sourceFile);
						}
						break;
					}
					if (sourceFile != NULL) {
						fclose(sourceFile);
					}
				}
			}

			fclose(batchFile);

		} else if (input == 'm' || input == 'M') {
			// Manual mode

			printf("\n  What file would you like to edit? (r, 0, 1 ... e, f)\n\n->");
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
			} else if ((file >= '0' && file <= '9') || (file >= 'a' && file <= 'f')) {
				//disk
				char diskIdChar[1];
				char path[100];
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

					fclose(f);

					printf("\n  Disk formatting complete!\n");
					fflush(stdout);
				} else if ((sector >= '0' && sector <= '9') || (sector >= 'a' && sector <= 'f')) {
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

					fclose(f);

					if (!invalidBreak) {
						printf("\n\n  Sector size limit reached\n");
						fflush(stdout);
						fflush(stdin);
					}
				}
			}
		}

		// TODO exit condition
		printf("\n  Exit? (Y/N)\n\n->");
		fflush(stdout);
		fflush(stdin);
		scanf("%c", &input);

		if (input == 'y' || input == 'Y') {
			break;
		}

	}
	return EXIT_SUCCESS;
}

int binInfo() {
	printf("\n  Binary writer: input hexadecimal numbers (no leading 0x, from 00 to FF)\n");
	printf("  Enter a non-hex letter to exit\n\n->");
	fflush(stdout);
	return 0;
}















