#include <stdlib.h>
#include "base.h"
#include "dfpwm.h"

// state variables
short fq, q, s;
int8_t lt = -128;

void au_decompress(int8_t *buf, uint8_t* data, int len) {
	short lq;
	uint8_t d,eq;
	int8_t t;

	//printf("!\n");
	_asm {
		MOV DI, buf
		MOV SI, data
		MOV BX, len

	// eq is 0 if t==lt, 255 if t!=lt

	outer_loop:
		// the XOR is done here to save one below
			MOV AL, [SI]
			NOT AL
			MOV d, AL
			INC SI
			PUSH BX
			MOV BL, 8
		inner_loop:
			// t = (d & 1 ^ 1) + 0x7F;
			MOV AL, d
			MOV CL, AL

			AND AL, 1
			ADD AL, 0x7F
			MOV t, AL
			CBW
			SHR CL, 1
			MOV d, CL
			MOV CL, AL
			XOR CL, lt
			MOV eq, CL

		/*nq = q + ((s * (t-(q)) + 0x80)>>8);
		if(nq == (q) && nq != t)
			q += (t == 127 ? 1 : -1);
		lq = q;
		q = nq;*/

			MOV DX, q
			MOV CX, AX
			SUB CX, DX // CX = t - q
			MOV AX, s
			PUSH DX
			IMUL CX      // DX:AX = CX*AX ((t - q) * s)
			ADD AX, 0x80
			MOV AL, AH
			CBW
			POP DX
			ADD AX, DX
			// AX is now nq
			CMP AL, t // nq != t
			JE qcalc_fin
			CMP AX, DX // nq == q
			JNE qcalc_fin
			CMP t, 127
			JE qcalc_1
			DEC DX
			JMP qcalc_fin
		qcalc_1:
			INC DX
		qcalc_fin:
			MOV lq, DX
			MOV q, AX
		//st = eq ^ 0xFF;
		//sr = (eq ? RD : RI);
		//ns = (s) + ((sr*(st-(s)) + 0x80)>>8);
			XOR CH, CH
			MOV CL, eq
			XOR CL, 0xFF
			JZ scalc_1
			MOV AX, RI
			JMP scalc_2
		scalc_1:
			MOV AX, RD
		scalc_2:
			MOV DX, s
			PUSH CX
			SUB CX, DX
			PUSH DX
			IMUL CX
			ADD AX, 0x80
			MOV AL, AH
			CBW
			POP DX
			ADD AX, DX
			// AX is now ns
			POP CX
		// if(ns == s && ns != starget)
		//	ns += (eq ? 1 : -1);
			CMP CX, AX
			JE scalc_fin
			CMP DX, AX
			JNE scalc_fin
			CMP eq, 0
			JE scalc_3
			DEC AX
			JMP scalc_fin
		scalc_3:
			INC AX
		scalc_fin:
			MOV s, AX

		/* FILTER: perform antijerk */
			MOV AL, t
			MOV CX, q
			CMP AL, lt
			JZ l1
			ADD CX, lq
			SHR CX, 1
		l1:
		// FILTER: lowpass
			PUSH BX
			MOV lt, AL // lt = t in a sneaky place
			MOV BX, fq
			SUB CX, BX
			MOV AX, 100
			MUL CX
			ADD AX, 0x80
			MOV AL, AH
			CBW
			ADD BX, AX
			MOV fq, BX
			XOR BX, 0x80
			MOV [DI], BX
			POP BX
			INC DI
			DEC BL
			JZ test2
			JMP inner_loop
		test2:
			POP BX
			DEC BX
			JZ test3
			JMP outer_loop
		test3:
		}
}
