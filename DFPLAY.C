#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "base.h"
#include "dfpwm.h"

#define FLEN 4096
#define FPL 2048
#define FREQ 32258
#define DMAADDR 0x02
#define DMACOUNT 0x03
#define DMAPAGE 0x83
#define DMACHANNEL 1
#define DMAMASK 0x0A
#define DMACLEAR 0x0C
#define DMAMODE 0x0B
#define SBIRQ 5
#define SBIRQADDR (8 + SBIRQ)
#define SLEN 512
//#define TIME_CONST 65536 - (256000000 / FREQ)
#define TIME_CONST 225 << 8

FILE *file;
int8_t *sample_buf;
uint8_t *data_buf;
int pos;
uint8_t sample_pos;
int endloc;

uint8_t ready;

void sendsb(uint8_t d) {
	ready = 128;
	while (ready > 127) {
	     ready = inportb(0x22C);
	}
	outportb(0x22C, d);
}

void loadone() {
	au_decompress(sample_buf, data_buf + pos, SLEN);
	pos = (pos + SLEN) & (FLEN - 1);
}

void interrupt (*old_interrupt)();
void interrupt new_interrupt()
{
	if (pos != endloc) {
		loadone();
		inportb(0x22E);
		outportb(0x20, 0x20);
	}
}

void loaddata(int pos, int len) {
	size_t loaded = fread(&(data_buf[pos]), 1, len, file);
	if (loaded < len) {
		endloc = pos + loaded;
	}
}

void setupdma() {
	long addr;
	unsigned int page, offset, len;
	addr = FP_SEG(sample_buf);
	addr <<= 4;
	addr += FP_OFF(sample_buf);
	page = addr >> 16;
	offset = addr & 0xFFFF;
	len = (SLEN * 8) - 1;
	outportb(DMAMASK, 0x04 + DMACHANNEL);
	outportb(DMACLEAR, 0xFF);
	outportb(DMAMODE, 0x58 + DMACHANNEL);
	outportb(DMAADDR, offset & 0xFF);
	outportb(DMAADDR, offset >> 8);
	outportb(DMAPAGE, page);
	outportb(DMACOUNT, len & 0xFF);
	outportb(DMACOUNT, len >> 8);
	outportb(DMAMASK, DMACHANNEL);
}

void setupdsp() {
	unsigned int len = (SLEN * 8) - 1;
	sendsb(0x40);
	sendsb(((TIME_CONST) & 0xFF00) >> 8);
	//sendsb(0x41);
	//sendsb((FREQ >> 2) & 0xFF);
	//sendsb((FREQ >> 2) >> 8);
	sendsb(0x48);
	sendsb(len & 0xFF);
	sendsb(len >> 8);

	//sendsb(0x90);
	sendsb(0xC6);
	sendsb(0x00);
	sendsb(len & 0xFF);
	sendsb(len >> 8);
}

void initsb() {
	uint8_t t;

	outportb(0x226, 1);
	delay(1);
	outportb(0x226, 0);
	t = 0;
	while (t < 128) {
		t = inportb(0x22E);
	}
	t = 0;
	while (t != 0xAA) {
		t = inportb(0x22A);
	}

	outportb(0x21, inportb(0x21) & !(1 << SBIRQ));
	printf("Sound Blaster 16 initialized\n");
}

void main(int argc, char *argv[]) {
	uint8_t loadmode;

	if (argc <= 1) {
		printf("Usage: DFPLAY [file]");
		return;
	}

	old_interrupt = getvect(SBIRQADDR);
	setvect(SBIRQADDR, new_interrupt);

	initsb();

	endloc = FLEN;

	file = fopen(argv[1], "rb");
	sample_buf = malloc(SLEN * 8);
	data_buf = malloc(FLEN);
	pos = 0;
	sample_pos = 8;

	loaddata(0, FPL);
	loadmode = 1;

	loadone();
	loadone();

	setupdma();
	setupdsp();

	while (!kbhit()) {
		if (endloc == pos) {
			break;
		} else if (loadmode == 1 && pos >= FPL) {
			loadmode = 0;
			loaddata(FPL, FPL);
		} else if (loadmode == 0 && pos < FPL) {
			loadmode = 1;
			loaddata(0, FPL);
		}
	}

	sendsb(0xDA);
	sendsb(0xD3);
	setvect(SBIRQADDR, old_interrupt);

	fclose(file);

	printf("Done\n");
	sleep(1);

	return;
}
