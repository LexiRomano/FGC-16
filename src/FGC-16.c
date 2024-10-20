#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <conio.h>
#include <windows.h>

// This isn't actually in windows.h because screw you I guess :/
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004

// Instructions (see README.md for datasheet):

#define LDA_IM  0x02
#define LDA_LOC 0x22
#define LDA_X   0x42
#define LDA_LOX 0x62
#define LDA_P   0x82
#define LDA_PNP 0xA2

#define STA     0x23
#define STA_X   0x43
#define STA_LOX 0x63
#define STA_P   0x83
#define STA_PNP 0xA3

#define LDX_IM  0x04
#define LDX     0x24
#define STX     0x25

#define INA     0x08
#define DEA     0x09
#define INX     0x0A
#define DEX     0x0B

#define ADD_IM  0x0C
#define ADD_LOC 0x2C
#define ADD_X   0x4C
#define ADD_LOX 0x6C
#define ADD_P   0x8C
#define ADD_PNP 0xAC

#define SUB_IM  0x0D
#define SUB_LOC 0x2D
#define SUB_X   0x4D
#define SUB_LOX 0x6D
#define SUB_P   0x8D
#define SUB_PNP 0xAD

#define BLT     0x0E
#define BLC     0x1E
#define BRT     0x0F
#define BRC     0x1F

#define AND_IM  0x18
#define AND_LOC 0x38
#define AND_X   0x58
#define AND_LOX 0x78
#define AND_P   0x98
#define AND_PNP 0xB8
#define ORR_IM  0x19
#define ORR_LOC 0x39
#define ORR_X   0x59
#define ORR_LOX 0x79
#define ORR_P   0x99
#define ORR_PNP 0xB9
#define XOR_IM  0x1A
#define XOR_LOC 0x3A
#define XOR_X   0x5A
#define XOR_LOX 0x7A
#define XOR_P   0x9A
#define XOR_PNP 0xBA
#define NOT     0x1B

#define JMP_LOC 0x30
#define JMP_PT  0x90
#define JZR_LOC 0x31
#define JZR_PT  0x91
#define JPO_LOC 0x32
#define JPO_PT  0x92
#define JNE_LOC 0x33
#define JNE_PT  0x93
#define JOV_LOC 0x34
#define JOV_PT  0x94

#define JMP_LOS 0x70
#define JMP_PTS 0xD0
#define JZR_LOS 0x71
#define JZR_PTS 0xD1
#define JPO_LOS 0x72
#define JPO_PTS 0xD2
#define JNE_LOS 0x73
#define JNE_PTS 0xD3
#define JOV_LOS 0x74
#define JOV_PTS 0xD4

#define RFS     0x15
#define NOP     0x00
#define SHD     0xFF
#define REL     0x01
#define REP     0x81

#define PIN     0x35
#define RIN     0x55
#define CIN     0x75

#define TGI_IM  0x06
#define TGI_LOC 0x26
#define TGI_X   0x46
#define TGI_LOX 0x66
#define TGI_P   0x86

#define FAA_IM  0x16
#define FAA_LOC 0x36
#define FAA_X   0x56
#define FAA_LOX 0x76
#define FAA_P   0x96
#define FAA_PNP 0xB6

#define SYS     0
#define PROG    1

#define NEG     0x01
#define ZRO     0x02
#define POS     0x04
#define OVF     0x08

#define LOC     0
#define PT      1

#define N       0
#define Y       1

#define STACK_A_OFFSET 0x40
#define STACK_SIZE     0x20

#define KEYBOARD_INTERRUPT 0x03

int load(unsigned char* dest, unsigned char a, unsigned char b, int permission);
#define LOAD(dest,a,b,per) if (load(dest, a, b, per)) {go = 0; strcpy(error, "Load permission error"); break;}

int loadAdr();
#define LOADADR() if (loadAdr()) {go = 0; break;}

int store(unsigned char origin, unsigned char a, unsigned char b, int permission);
#define STORE(origin,a,b,per) if (store(origin, a, b, per)) {go = 0; strcpy(error, "Store permission error"); break;}

void inc(unsigned char* a, unsigned char* b);
void plus(unsigned char aA, unsigned char aB, unsigned char bA, unsigned char bB,
		unsigned char* resultA, unsigned char* resultB);

void add(unsigned char* b);
void sub(unsigned char* b);

int conditionalJump(unsigned char comparison, unsigned char depth, unsigned char stack);
#define CONDITIONALJUMP(c, d, s) if (conditionalJump(c, d, s)) {go = 0; strcpy(error, "Jump Error"); break;}

// Returns address bytes a and b when taking into account REL
unsigned char relativeAddressA(unsigned char a, unsigned char b);
unsigned char relativeAddressB(unsigned char b);

// Same as above, but taking into account REP
unsigned char relativeAddressAP(unsigned char a, unsigned char b, unsigned char orgA, unsigned char orgB);
unsigned char relativeAddressBP(unsigned char b, unsigned char orgB);

int queue_interrupt(unsigned char type, unsigned char id);
#define QUEUE_INTERRUPT(type, id) if (queue_interrupt(type, id)) {go = 0; strcpy(error, "Interrupt queue error"); break;}

int trigger_interrupt();
#define TRIGGER_INTERRUPT() if (trigger_interrupt()) {go = 0; strcpy(error, "Interrupt trigger error"); break;}

int jumpLOC();
int jumpPT();
int saveToStack();

#ifdef DEBUG_MODE
	void debug();
#endif

// Flags register: X, X, X, X, OVF, POS, ZER, NEG
// Relative register: X, X, X, X, X, X, REP, REL
unsigned char a, xA, xB, ptA, ptB, ins, adrA, adrB, flg, stA, stB, tmpA, tmpB, tmpC, tmpD,
																	rel, comA, comB, inp;
unsigned char ram[0x100][0x100];
unsigned char rom[0x40][0x100];

char ioBuffer[0x100];
unsigned char ioBufferPt, dskB = 0x00;

int main() {
	unsigned char buffer;

	// Reading ROM
	FILE* romFile = fopen(".\\data\\r.rom", "rb");

	unsigned char i = 0x00;
	unsigned char j = 0x00;
	#ifdef DEBUG_MODE
		int count = 0;
		int lastNonZero = 0;
	#endif
	while (fread(&buffer , 1, 1, romFile) != 0) {
		rom[j][i] = buffer;
		i++;
		if (i == 0) {
			j++;
		}
		#ifdef DEBUG_MODE
			count++;
			if (buffer != 0) {
				lastNonZero = count;
			}
		#endif
	}

	fclose(romFile);

	// Printing ROM
	#ifdef DEBUG_MODE
		printf("\nSuccesfully read from ROM, contents:\n");
		i = 0x00;
		j = 0x00;
		while (lastNonZero > 0) {
			if (i % 0x10 == 0) {
				printf(" ");
			}
			if (i % 0x20 == 0) {
				printf("\n");
			}
			printf("%02x ", rom[j][i]);

			i++;
			if (i == 0) {
				j++;

			}
			lastNonZero--;
		}

		// Executing
		printf("\n\nexecuting...\n");
		fflush(stdout);
	#endif

	int go = 1;
	char error[1024] = "\0";

	// Initializing interrupt queue pointers
	ram[0x77][0xFC] = 0x78;
	ram[0x77][0xFE] = 0x78;

	// Allow ANSI escape on windows 10
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hConsole, &mode); //
	SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);


	while (go) {

		// Keeping track of the original command location (for relative stuff)
		comA = ptA;
		comB = ptB;
		// Loading next instruction
		LOAD(&ins, ptA, ptB, PROG);
		inc(&ptA, &ptB);

		switch(ins) {
		case LDA_IM:
			LOAD(&a, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			break;

		case LDA_LOC:
			LOADADR();

			LOAD(&a, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			break;

		case LDA_X:
			LOAD(&a, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			break;

		case LDA_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&a, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			break;

		case LDA_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&a, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			break;

		case LDA_PNP:

			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&a, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			break;

		case STA:
			LOADADR();
			STORE(a, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			break;

		case STA_X:
			STORE(a, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			break;

		case STA_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			STORE(a, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			break;

		case STA_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			STORE(a, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			break;
		case STA_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			STORE(a, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			break;

		case LDX_IM:
			LOAD(&xA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			LOAD(&xB, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			break;

		case LDX:
			LOADADR();

			LOAD(&xA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&xB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			break;

		case STX:
			LOADADR();

			STORE(xA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			STORE(xB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			break;
		case INA:
			a++;
			if (a == 0) {
				flg = OVF;
			} else {
				flg = POS;
			}
			break;

		case DEA:
			a--;
			if (a == 0) {
				flg = ZRO;
			} else if (a == 0xff) {
				flg = NEG;
			} else {
				flg = POS;
			}
			break;

		case INX:
			inc(&xA, &xB);
			if (xA == 0 && xB == 0) {
				flg = OVF;
			} else {
				flg = POS;
			}
			break;

		case DEX:
			if (--xB == 0xFF) {
				xA--;
			}
			if (xA == 0 && xB == 0) {
				flg = ZRO;
			} else if (xA == 0xff &&  xB == 0xff) {
				flg = NEG;
			} else {
				flg = POS;
			}
			break;

		case ADD_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			add(&tmpA);
			break;

		case ADD_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			add(&tmpA);
			break;

		case ADD_X:
			LOAD(&tmpA, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			add(&tmpA);
			break;

		case ADD_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&tmpA, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			add(&tmpA);
			break;

		case ADD_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			add(&tmpA);
			break;

		case ADD_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			add(&tmpA);
			break;

		case SUB_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			sub(&tmpA);
			break;

		case SUB_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			sub(&tmpA);
			break;

		case SUB_X:
			LOAD(&tmpA, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			sub(&tmpA);
			break;

		case SUB_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&tmpA, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			sub(&tmpA);
			break;

		case SUB_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			sub(&tmpA);
			break;

		case SUB_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			sub(&tmpA);
			break;

		case BLT:
			if (a == 0) {
				flg = ZRO;
			} else if (a < 0x80) {
				flg = POS;
			} else {
				flg = OVF;
			}
			a = a << 1;
			break;

		case BLC:
			if (a >= 0x80) {
				a = a << 1;
				a += 1;
				flg = OVF;
			} else {
				a = a << 1;
				if (a == 0) {
					flg = ZRO;
				} else {
					flg = POS;
				}
			}
			break;

		case BRT:
			if (a == 0) {
				flg = ZRO;
			} else if (a % 2 == 1) {
				flg = NEG;
			} else {
				flg = POS;
			}
			a = a >> 1;
			break;

		case BRC:
			if (a % 2 == 1) {
				a = a >> 1;
				a += 0x80;
				flg = NEG;
			} else {
				a = a >> 1;
				if (a == 0) {
					flg = ZRO;
				} else {
					flg = POS;
				}
			}
			break;

		case AND_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case AND_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&ptA, &ptB);

			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case AND_X:
			LOAD(&tmpA, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case AND_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&tmpA, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case AND_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case AND_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a & tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&ptA, &ptB);

			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_X:
			LOAD(&tmpA, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&tmpA, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case ORR_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a | tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&ptA, &ptB);

			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_X:
			LOAD(&tmpA, relativeAddressA(xA, xB), relativeAddressB(xB), PROG);
			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);
			LOAD(&tmpA, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case XOR_PNP:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			adrA = tmpA;
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);
			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			a = a ^ tmpA;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case NOT:
			a = ~a;
			flg = 0x04;
			if (a == 0) {
				flg = 0x02;
			}
			break;

		case JMP_LOC:
			CONDITIONALJUMP(0xff, LOC, N);
			break;

		case JMP_PT:
			CONDITIONALJUMP(0xff, PT, N);
			break;

		case JZR_LOC:
			CONDITIONALJUMP(ZRO, LOC, N);
			break;

		case JZR_PT:
			CONDITIONALJUMP(ZRO, PT, N);
			break;

		case JPO_LOC:
			CONDITIONALJUMP(POS, LOC, N);
			break;

		case JPO_PT:
			CONDITIONALJUMP(POS, PT, N);
			break;

		case JNE_LOC:
			CONDITIONALJUMP(NEG, LOC, N);
			break;

		case JNE_PT:
			CONDITIONALJUMP(NEG, PT, N);
			break;

		case JOV_LOC:
			CONDITIONALJUMP(OVF, LOC, N);
			break;

		case JOV_PT:
			CONDITIONALJUMP(OVF, PT, N);
			break;

		case JMP_LOS:
			CONDITIONALJUMP(0xff, LOC, Y);
			break;

		case JMP_PTS:
			CONDITIONALJUMP(0xff, PT, Y);
			break;

		case JZR_LOS:
			CONDITIONALJUMP(ZRO, LOC, Y);
			break;

		case JZR_PTS:
			CONDITIONALJUMP(ZRO, PT, Y);
			break;

		case JPO_LOS:
			CONDITIONALJUMP(POS, LOC, Y);
			break;

		case JPO_PTS:
			CONDITIONALJUMP(POS, PT, Y);
			break;

		case JNE_LOS:
			CONDITIONALJUMP(NEG, LOC, Y);
			break;

		case JNE_PTS:
			CONDITIONALJUMP(NEG, PT, Y);
			break;

		case JOV_LOS:
			CONDITIONALJUMP(OVF, LOC, Y);
			break;

		case JOV_PTS:
			CONDITIONALJUMP(OVF, PT, Y);
			break;
		case PIN:
			inp = 1;
			break;

		case RIN:
			inp = 0;
			break;

		case CIN:
			inp = 0;
			// Intentionally no break since we want to do an RFS after a CIN
		case RFS:
			if (stA == 0x00 && stB == 0x00) {
				strcpy(error, "Stack underflow error");
				go = 0;
				break;
			}
			if (--stB == 0xFF) {
				stA--;
			}
			LOAD(&ptB, stA + STACK_A_OFFSET, stB, SYS);

			if (--stB == 0xFF) {
				stA--;
			}
			LOAD(&ptA, stA + STACK_A_OFFSET, stB, SYS);
			break;

		case TGI_IM:
			LOAD(&tmpA, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			LOAD(&tmpB, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			QUEUE_INTERRUPT(tmpA, tmpB);
			break;

		case TGI_LOC:
			LOADADR();

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);
			LOAD(&tmpB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);

			QUEUE_INTERRUPT(tmpA, tmpB);
			break;

		case TGI_X:
			tmpA = xA;
			tmpB = xB;
			LOAD(&tmpC, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			inc(&tmpA, &tmpB);
			LOAD(&tmpD, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);

			QUEUE_INTERRUPT(tmpC, tmpD);
			break;

		case TGI_LOX:
			LOADADR();

			plus(adrA, adrB, xA, xB, &tmpA, &tmpB);

			LOAD(&tmpC, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);
			inc(&tmpA, &tmpB);
			LOAD(&tmpD, relativeAddressA(tmpA, tmpB), relativeAddressB(tmpB), PROG);

			QUEUE_INTERRUPT(tmpC, tmpD);
			break;

		case TGI_P:
			LOADADR();

			tmpC = relativeAddressA(adrA, adrB);
			tmpD = relativeAddressB(adrB);

			LOAD(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);
			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			adrA = tmpA;

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);
			inc(&adrA, &adrB);

			LOAD(&tmpA, relativeAddressAP(adrA, adrB, tmpC, tmpD), relativeAddressBP(adrB, tmpD), PROG);

			QUEUE_INTERRUPT(tmpA, tmpB);
			break;

		case FAA_IM:
			xA = adrA;
			xB = adrB;
			// Decrement to get the address of the command
			if (--xB == 0xFF) {
				xA--;
			}
			break;

		case FAA_LOC:
			LOADADR();

			xA = relativeAddressA(adrA, adrB);
			xB = relativeAddressB(adrB);
			break;

		case FAA_X:
			tmpA = xA;
			tmpB = xB;
			xA = relativeAddressA(xA, xB);
			xB = relativeAddressB(xB);
			break;

		case FAA_LOX:
			LOADADR();

			plus(xA,xB, adrA, adrB, &tmpA, &tmpB);
			xA = relativeAddressA(tmpA, tmpB);
			xB = relativeAddressB(tmpB);
			break;

		case FAA_P:
			LOADADR();
			tmpA = adrA;
			tmpB = adrB;

			LOAD(&tmpC, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			adrA = tmpC;

			xA = relativeAddressAP(adrA, adrB, tmpA, tmpB);
			xB = relativeAddressBP(adrB, tmpB);
			break;

		case FAA_PNP:
			LOADADR();
			tmpA = adrA;
			tmpB = adrB;

			LOAD(&tmpC, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			inc(&adrA, &adrB);

			LOAD(&adrB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG);
			adrA = tmpC;

			LOAD(&tmpC, ptA, ptB, PROG);
			inc(&ptA, &ptB);

			plus(adrA, adrB, 0, tmpA, &adrA, &adrB);

			xA = relativeAddressAP(adrA, adrB, tmpA, tmpB);
			xB = relativeAddressBP(adrB, tmpB);
			 break;

		case NOP:
			break;

		case REL:
			rel = rel | 0b00000001;
			break;

		case REP:
			rel = rel | 0b00000010;
			break;

		case SHD:
			go = 0;
			break;

		default:
			go = 0;
			strcpy(error, "Invalid Command");
		}

		// Reseting the REL register
		if (ins != REP && ins != REL) {
			rel = 0;
		}

		// Handling keyboard interrupts
		if (kbhit()) {
			QUEUE_INTERRUPT(KEYBOARD_INTERRUPT, getch());
		} else if (inp == 0 && rel == 0) {
			TRIGGER_INTERRUPT();
		}

		#ifdef DEBUG_MODE
			if (go) debug();
		#endif
	}

	if (error[0] != '\0') {
		printf("\n%s\n", error);
	}
	printf("\nProgram terminated, press enter to exit");
	fflush(stdout);
	char z;
	scanf("%c", &z);
	return EXIT_SUCCESS;
}

/*
 * Loads a byte from the memory address specified by a and b into destination. Permission signifies if it's code
 * or the processor that is performing this operation.
 */

int load(unsigned char* dest, unsigned char a, unsigned char b, int permission) {
	if (permission != SYS && permission != PROG) {
		return 1;
	}

	if (a >= 0x80) {
		//Ram
		*dest = ram[a][b];
		return 0;
	} else if (a >= 0x70 && a <= 0x7F) {
		//Interrupts
		if ((a == 0x77 && b >= 0xFC) || a >= 0x78) {
			// interrupt queue
			if (permission == SYS) {
				*dest = ram[a][b];
				return 0;
			}
			return 1;
		}
		if ((a == 0x70 && (b == 0x00 || b == 0x01)) || a == 0x71 || a == 0x72) {
			// current interrupt/handle pointers
			*dest = ram[a][b];
			return 0;
		}
		return 1;
	} else if (a == 0x60 && (b >= 0x00 && b <= 0x02)) {
		//I/O Port
		if (b == 0x02) {
			*dest = dskB;
			return 0;
		}
		return 1;
	} else if (a < 0x40) {
		//Rom
		*dest = rom[a][b];
		return 0;
	} else if (a >= 0x40 && a <= 0x5F && permission == SYS) {
		//Stack (needs SYS permission)
		*dest = ram[a][b];
		return 0;
	}
	return 1;
}

/*
 * Loads the next 2 bytes pointed to by the pointer register into the address register.
 */

int loadAdr() {
	if (load(&adrA, ptA, ptB, SYS)) return 1;
	inc(&ptA, &ptB);

	if (load(&adrB, ptA, ptB, SYS)) return 1;
	inc(&ptA, &ptB);

	return 0;
}

/*
 * Stores a byte from the origin in the memory address specified by a and b. Permission signifies if it's code
 * or the processor that is performing this operation.
 */

int store(unsigned char origin, unsigned char a, unsigned char b, int permission) {
	if (permission != SYS && permission != PROG) {
		return 1;
	}

	if (a>= 0x80) {
		//Ram
		ram[a][b] = origin;
		return 0;
	} else if (a >= 0x70 && a <= 0x7F) {
		//Interrupts
		if (permission == PROG) {
			// PROG is only allowed to write to the handle pointers
			if (a == 0x71 || a ==0x72) {
				ram[a][b] = origin;
				return 0;
			}
			return 1;
		}
		if ((a == 0x70 && b > 0x01) || (a >= 0x73 && a <= 0x76) || (a == 0x77 && b < 0xFC)) {
			// Unused addresses
			return 1;
		}
		ram[a][b] = origin;
		return 0;
	} else if (a == 0x60 && (b >= 0x00 && b <= 0x02)) {
		//I/O Port
		switch (b) {
		case 0x00:
			if (printf("%c", origin) == -1) {
				ram[0x60][0x02] = 0xFF; // Write unsuccessful
			} else {
				ram[0x60][0x02] = 0x10; // Write successful
			}
			fflush(stdout);
			break;
		case 0x01:
			; //Yes, this semicolon is important. No, I'm not happy about it.
			unsigned char rw, ramSec, diskId, diskSec;
			rw = origin / 0x80;
			ramSec = origin % 0x08;
			diskId = dskB / 0x10;
			diskSec = dskB % 0x10;
			char diskIdChar[50], path[50];

			sprintf(diskIdChar, "%01x", diskId);
			strcpy(path, ".\\data\\");
			strcat(path, diskIdChar);
			strcat(path, ".dsk");

			FILE* disk = fopen(path, "rb+");
			fseek(disk, ((int)diskSec) * 0x1000, SEEK_SET);
			unsigned char rmA, rmB = 0x00;
			rmA = 0x80 + (ramSec << 4);
			if (rw) {
				// Write
				for (int i = 0; i < 0x1000; i++) {
					fwrite(&ram[rmA][rmB], 1, 1, disk);
					if (++rmB == 0) {
						rmA++;
					}
				}
			} else {
				// Read
				for (int i = 0; i < 0x1000; i++) {
					fread(&ram[rmA][rmB], 1, 1, disk);
					if (++rmB == 0) {
						rmA++;
					}
				}
			}
			fclose(disk);

			break;
		case 0x02:
			dskB = origin;
			break;
		}

		return 0;
	} else if (a >= 0x40 && a <= 0x5F && permission == SYS) {
		//Stack (needs SYS permission)
		ram[a][b] = origin;
		return 0;
	}
	return 1;

}

/*
 * Increments a 2 byte number
 */

void inc(unsigned char* a, unsigned char* b) {
	if (++(*b) == 0) {
		(*a)++;
	}
}

/*
 * Adds together a pair of 2 byte numbers
 */

void plus(unsigned char aA, unsigned char aB, unsigned char bA, unsigned char bB,
		unsigned char* resultA, unsigned char* resultB) {
	*resultB = aB + bB;
	*resultA = aA + bA;
	if (*resultB < aB) { // overflow
		(*resultA)++;
	}
}

/*
 * Adds the number b to the A register
 */

void add(unsigned char* b) {
	int original = a;
	a += *b;
	if (a < original) {
		flg = OVF;
	} else if (a == 0) {
		flg = ZRO;
	} else {
		flg = POS;
	}
}

/*
 * Subtracts the number b from the A register
 */

void sub(unsigned char* b) {
	int original = a;
	a -= *b;
	if (a == 0) {
		flg = ZRO;
	} else if (a > original) {
		flg = NEG;
	} else {
		flg = POS;
	}
}

/*
 * Jumps to the location stored in the address register if the flags register matches the provided comparison.
 * depth specifies whether it's a basic "location" jump or a pointer jump and stack is whether or not the location
 * immediately after the jump should be saved to the stack.
 */

int conditionalJump(unsigned char comparison, unsigned char depth, unsigned char stack) {
	if (flg == comparison || comparison == 0xff) {
		if (stack == Y) {
			if (saveToStack()) return 1;
		}
		if (depth == LOC) {
			if (jumpLOC()) return 1;
		} else {
			if (jumpPT()) return 1;
		}
	} else {
		inc(&ptA, &ptB);
		inc(&ptA, &ptB);
	}
	return 0;
}

/*
 * Jumps to the next 2 bytes pointed to by the pointer register as the location to jump to.
 */

int jumpLOC() {
	if (load(&tmpA, ptA, ptB, PROG)) return 1;
	inc(&ptA, &ptB);

	if (load(&tmpB, ptA, ptB, PROG)) return 1;
	ptA = relativeAddressA(tmpA, tmpB);
	ptB = relativeAddressB(tmpB);
	return 0;
}

/*
 * Looks at the next 2 bytes pointed to by the pointer register as the location to for another
 * 2 bytes which is then used as the location to jump to.
 */

int jumpPT() {
	if (loadAdr()) return 1;

	tmpC = relativeAddressA(adrA, adrB);
	tmpD = relativeAddressB(adrB);

	if (load(&tmpA, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG)) return 1;
	inc(&adrA, &adrB);

	if (load(&tmpB, relativeAddressA(adrA, adrB), relativeAddressB(adrB), PROG)) return 1;
	ptA = relativeAddressAP(tmpA, tmpB, tmpC, tmpD);
	ptB = relativeAddressBP(tmpB, tmpD);
	return 0;
}

/*
 * Saves the address that's 2 addresses after the current address pointed to by the pointer register.
 * Used for JMS commands.
 */

int saveToStack() {
	tmpA = ptA;
	tmpB = ptB;
	inc(&tmpA, &tmpB);
	inc(&tmpA, &tmpB);


	if (stA != STACK_SIZE) {
		if (store(tmpA, stA + STACK_A_OFFSET, stB, SYS)) return 1;
		inc(&stA, &stB);

		if (store(tmpB, stA + STACK_A_OFFSET, stB, SYS)) return 1;
		inc(&stA, &stB);

		return 0;
	}
	return 1;
}

/*
 * Converts a relative address specified by a and b into the a portion of an absolute address if the rel
 * register's REL bit is set
 */

unsigned char relativeAddressA(unsigned char a, unsigned char b) {
	// relativeAddressB() < adrB returns 1 when the B byte overflows
	// comA is the a location of the command.
	return a + ((rel & 0b00000001) * ((relativeAddressB(b) < b) + comA));
}

/*
 * Converts a relative address specified by b into the b portion of an absolute address if the rel register's
 * REL bit is set.
 */

unsigned char relativeAddressB(unsigned char b) {
	// comB is the b location of the command.
	return b + (rel & 0b00000001) * comB;
}

/*
 * Converts a relative pointer address specified by a and b into the a portion of an absolute address if the rel
 * register's REP bit is set. orgA and orgB is the original location of the pointer (not where it's pointing to)
 */

unsigned char relativeAddressAP(unsigned char a, unsigned char b, unsigned char orgA, unsigned char orgB) {
	// relativeAddressB() < adrB returns 1 when the B byte overflows
	return a + (((rel & 0b00000010) >> 1) * ((relativeAddressBP(b, orgB) < b) + orgA));
}

/*
 * Converts a relative pointer address specified by b into the b portion of an absolute address if the rel register's
 * REP bit is set. orgB the b part of the original location of the pointer (not where it's pointing to)
 */

unsigned char relativeAddressBP(unsigned char b, unsigned char orgB) {
	return b + ((rel & 0b00000010) >> 1) * orgB;
}

/*
 * Adds an interrupt to the queue and attempts to trigger the next in queue if possible.
 */

int queue_interrupt(unsigned char type, unsigned char id) {
	unsigned char qha, qhb;
	if (load(&qha, 0x77, 0xFC, SYS)) return 1;
	if (load(&qhb, 0x77, 0xFD, SYS)) return 1;
	if (store(type, qha, qhb, SYS)) return 1;
	inc(&qha, &qhb);
	if (qha >= 0x80) qha = 0x78;

	if (store(id, qha, qhb, SYS)) return 1;
	inc(&qha, &qhb);
	if (qha >= 0x80) qha = 0x78;

	if (store(qha, 0x77, 0xFC, SYS)) return 1;
	if (store(qhb, 0x77, 0xFD, SYS)) return 1;

	if (inp == 0 && rel == 0) {
		return trigger_interrupt();
	}
	return 0;

}

/*
 * If the queue head and queue tail aren't pointing to the same location, the next interrupt will
 * be triggered and the queue tail moved up.
 */

int trigger_interrupt() {
	unsigned char qha, qhb, qta, qtb;
	if (load(&qha, 0x77, 0xFC, SYS)) return 1;
	if (load(&qhb, 0x77, 0xFD, SYS)) return 1;
	if (load(&qta, 0x77, 0xFE, SYS)) return 1;
	if (load(&qtb, 0x77, 0xFF, SYS)) return 1;

	// Only interrupt if there's something in the queue
	if (qha != qta || qhb != qtb) {
		if (stA != STACK_SIZE) {
			// Save original place to the stack
			if (store(ptA, stA + STACK_A_OFFSET, stB, SYS)) return 1;
			inc(&stA, &stB);

			if (store(ptB, stA + STACK_A_OFFSET, stB, SYS)) return 1;
			inc(&stA, &stB);

			// Get the interrupt, store it in the active interrupt spot, and move up the tail pointer
			if (load(&tmpA, qta, qtb, SYS)) return 1;
			inc(&qta, &qtb);
			if (qta >= 0x80) qta = 0x78;

			if (load(&tmpB, qta, qtb, SYS)) return 1;
			inc(&qta, &qtb);
			if (qta >= 0x80) qta = 0x78;



			if (store(tmpA, 0x70, 0x00, SYS)) return 1;
			if (store(tmpB, 0x70, 0x01, SYS)) return 1;
			if (store(qta, 0x77, 0xFE, SYS)) return 1;
			if (store(qtb, 0x77, 0xFF, SYS)) return 1;

			// Jump to the handle pointer
			if (load(&ptA, 0x71, tmpA, SYS)) return 1;
			if (load(&ptB, 0x72, tmpA, SYS)) return 1;

			inp = 1;

			return 0;
		}
		return 1;
	}
	return 0;
}


#ifdef DEBUG_MODE
	void debug() {
		printf("\n  A:   0x%02x", (unsigned int) a);
		printf("\n  X:   0x%02x%02x", (unsigned int) xA, (unsigned int) xB);
		printf("\n  flg: 0x%02x", (unsigned int) flg);
		printf("\n  pt:  0x%02x%02x", (unsigned int) ptA, (unsigned int) ptB);
		printf("\n  ins: 0x%02x", (unsigned int) ins);
		printf("\n  adr: 0x%02x%02x", (unsigned int) adrA, (unsigned int) adrB);
		printf("\n  rel: 0x%02x", (unsigned int) rel);
		fflush(stdout);
		char z;
		scanf("%c", &z);
	}
#endif



