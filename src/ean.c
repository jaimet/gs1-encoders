/**
 * GS1 barcode encoder application
 *
 * @author Copyright (c) 2000-2020 GS1 AISBL.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "rssenc.h"


// includes qz's
#define EAN13_ELMNTS 61
// includes 7X quiet zones:
#define EAN13_W	109
// total ht in x:
#define EAN13_H	74

#define EAN13_L_PAD 3 // EAN-13 7-X qz - CCA-2 3 offset
#define EAN13_R_PAD 5 // EAN-13 WIDTH - MAX_WIDTH - EAN13_L_PAD

extern int errFlag;
extern int line1;
extern UCHAR ccPattern[MAX_CCB4_ROWS][CCB4_ELMNTS];

int EAN13enc(UCHAR str[], UCHAR pattern[] );
int EAN8enc(UCHAR str[], UCHAR pattern[] );
int UPCEenc(UCHAR str[], UCHAR pattern[] );

void EAN13(struct sParams *params) {

struct sPrints prints;
struct sPrints sepPrnt;

UCHAR linPattern[EAN13_ELMNTS];
UCHAR sepPat1[5] = { 7,1,EAN13_W-16,1,7 }; // separator pattern 1
UCHAR sepPat2[5] = { 6,1,EAN13_W-14,1,6 }; // separator pattern 2

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;

		ccStr = strchr(params->dataStr, '|');
		if (ccStr == NULL) ccFlag = FALSE;
		else {
			ccFlag = TRUE;
			ccStr[0] = '\0'; // separate primary data
			ccStr++; // point to secondary data
		}

		if (strlen(params->dataStr) > 12) {
			errFlag = TRUE;
			printf("\nprimary data exceeds 12 digits");
			return;
		}

		strcpy(tempStr, "000000000000");
		strcat(tempStr, params->dataStr);
		strcat(tempStr, "0"); // check digit = 0 for now
		strcpy(primaryStr, tempStr + strlen(tempStr) - 13);
	
		if (EAN13enc((UCHAR*)primaryStr, linPattern)) {
			if (errFlag) {
				printf("\nerror occurred, exiting.");
				return;
			}
#if PRNT
			printf("\n%s", primaryStr);
			printf("\n");
			for (i = 0; i < EAN13_ELMNTS; i++) {
				printf("%d", linPattern[i]);
			}
			printf("\n");
#endif
		}
		line1 = TRUE; // so first line is not Y undercut
		// init most likely prints values
		prints.elmCnt = EAN13_ELMNTS;
		prints.pattern = linPattern;
		prints.guards = FALSE;
		prints.height = params->pixMult*EAN13_H;
		prints.leftPad = 0;
		prints.rightPad = 0;
		prints.whtFirst = TRUE;
		prints.reverse = FALSE;
		// init most likely separator values
		sepPrnt.elmCnt = 5;
		sepPrnt.pattern = sepPat1;
		sepPrnt.guards = FALSE;
		sepPrnt.height = params->pixMult*2;
		sepPrnt.leftPad = 0;
		sepPrnt.rightPad = 0;
		sepPrnt.whtFirst = TRUE;
		sepPrnt.reverse = FALSE;
		if (ccFlag) {
			if ((rows = CC4enc((UCHAR*)ccStr, ccPattern)) > 0) {
				if (errFlag) {
					printf("\nComposite encoding error.");
					return;
				}
#if PRNT
				printf("\n%s", ccStr);
				printf("\n");
				for (i = 0; i < rows; i++) {
					for (j = 0; j < CCB4_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
					printf("\n");
				}
#endif
			}

			if (params->bmp) {
				// note: BMP is bottom to top inverted
				bmpHeader(params->pixMult*EAN13_W, params->pixMult*(rows*2 + 6 + EAN13_H), params->outfp);

				// EAN-13
				printElmnts(params, &prints);

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// Composite Component
				prints.elmCnt = CCB4_ELMNTS;
				prints.height = params->pixMult*2;
				prints.leftPad = EAN13_L_PAD;
				prints.rightPad = EAN13_R_PAD;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
			else {
				tifHeader(params->pixMult*EAN13_W, params->pixMult*(rows*2 + 6 + EAN13_H), params->outfp);

				// Composite Component
				prints.elmCnt = CCB4_ELMNTS;
				prints.height = params->pixMult*2;
				prints.leftPad = EAN13_L_PAD;
				prints.rightPad = EAN13_R_PAD;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// EAN-13
				prints.elmCnt = EAN13_ELMNTS;
				prints.pattern = linPattern;
				prints.height = params->pixMult*EAN13_H;
				prints.leftPad = 0;
				prints.rightPad = 0;
				printElmnts(params, &prints);
			}
		}
		else { // primary only
			if (params->bmp) {
				bmpHeader(params->pixMult*EAN13_W, params->pixMult*EAN13_H, params->outfp);
			}
			else {
				tifHeader(params->pixMult*EAN13_W, params->pixMult*EAN13_H, params->outfp);
			}

			// EAN-13
			printElmnts(params, &prints);
		}
		return;
}

// call with str = 13-digit primary with check digit = 0
int EAN13enc(UCHAR str[], UCHAR pattern[] ) {

static UINT upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
														0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
static UINT upcTblB[10] = {	0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
														0x1321, 0x4111, 0x2131, 0x3121, 0x2113 };
static UINT abArr[10] = { 0 /*0x07*/,0x0B,0x0D,0x0E,0x13,0x19,0x1C,0x15,0x16,0x1A };
static UCHAR lGuard[4] = { 7,1,1,1 };
static UCHAR center[5] = { 1,1,1,1,1 }; 
static UCHAR rGuard[4] = { 1,1,1,7 };

int i, j, abMask, bars, sNdx, pNdx, abBits;

	// calculate UPC parity
	for (j = 0, i = 0; i < 12; i += 2) {
		j += str[i] - '0';
		j += (str[i+1] - '0') * 3;
	}
	j = (j%10);
	if (j > 0) {
		j = 10 - j;
	}
	str[12] = (UCHAR)(j + '0');

	sNdx = 1;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	abBits = abArr[str[0]-'0'];
	for (abMask = 0x20, i = 0; i < 6; abMask >>= 1, i++) {
		if ((abBits & abMask) == 0) {
			bars = upcTblA[str[sNdx++]-'0'];
		}
		else {
			bars = upcTblB[str[sNdx++]-'0'];
		}
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (bars >> j) & 0xf;
		}
	}
	for (i = 0; i < 5; i++) {
		pattern[pNdx++] = center[i];
	} 
	for (i = 0; i < 6; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (bars >> j) & 0xf;
		}
	} 
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(TRUE);
}

// includes qz's
#define EAN8_ELMNTS 45
// includes 7X quiet zones:
#define EAN8_W	81
// total ht in x:
#define EAN8_H	60

#define EAN8_L_PAD 2 // EAN-8 7-X qz - CCA-2 offset
#define EAN8_R_PAD 5 // EAN-8 WIDTH - MAX_WIDTH of cca - EAN8_L_PAD
#define EAN8_L_PADB 8 // EAN-8 left pad for ccb

void EAN8(struct sParams *params) {

struct sPrints prints;
struct sPrints sepPrnt;

UCHAR linPattern[EAN8_ELMNTS];
UCHAR sepPat1[5] = { 7,1,EAN8_W-16,1,7 }; // separator pattern 1
UCHAR sepPat2[5] = { 6,1,EAN8_W-14,1,6 }; // separator pattern 2

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;
int lpadCC;
int lpadEAN;
int elmntsCC;

		ccStr = strchr(params->dataStr, '|');
		if (ccStr == NULL) ccFlag = FALSE;
		else {
			ccFlag = TRUE;
			ccStr[0] = '\0'; // separate primary data
			ccStr++; // point to secondary data
		}

		if (strlen(params->dataStr) > 12) {
			errFlag = TRUE;
			printf("\nprimary data exceeds 12 digits");
			return;
		}

		strcpy(tempStr, "000000000000");
		strcat(tempStr, params->dataStr);
		strcat(tempStr, "0"); // check digit = 0 for now
		strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

		if (EAN8enc((UCHAR*)primaryStr, linPattern)) {
			if (errFlag) {
				printf("\nerror occurred, exiting.");
				return;
			}
#if PRNT
			printf("\n%s", primaryStr);
			printf("\n");
			for (i = 0; i < EAN8_ELMNTS; i++) {
				printf("%d", linPattern[i]);
			}
			printf("\n");
#endif
		}
		line1 = TRUE; // so first line is not Y undercut
		// init most likely prints values
		lpadEAN = 0;
		lpadCC = EAN8_L_PAD;
		elmntsCC = CCA3_ELMNTS;
		prints.elmCnt = EAN8_ELMNTS;
		prints.pattern = linPattern;
		prints.guards = FALSE;
		prints.height = params->pixMult*EAN8_H;
		prints.leftPad = 0;
		prints.rightPad = 0;
		prints.whtFirst = TRUE;
		prints.reverse = FALSE;
		// init most likely separator values
		sepPrnt.elmCnt = 5;
		sepPrnt.pattern = sepPat1;
		sepPrnt.guards = FALSE;
		sepPrnt.height = params->pixMult*2;
		sepPrnt.leftPad = 0;
		sepPrnt.rightPad = 0;
		sepPrnt.whtFirst = TRUE;
		sepPrnt.reverse = FALSE;
		if (ccFlag) {
			if ((rows = CC3enc((UCHAR*)ccStr, ccPattern)) > 0) {
				if (errFlag) {
					printf("\nComposite encoding error.");
					return;
				}
			}
			if (rows > MAX_CCA3_ROWS) { // CCB composite
				lpadEAN = EAN8_L_PADB;
				lpadCC = 0;
				elmntsCC = CCB3_ELMNTS;
				prints.leftPad = EAN8_L_PADB;
				sepPrnt.leftPad = EAN8_L_PADB;
			}
#if PRNT
			printf("\n%s", ccStr);
			printf("\n");
			for (i = 0; i < rows; i++) {
				for (j = 0; j < elmntCC; j++) {
					printf("%d", ccPattern[i][j]);
				}
			}
			printf("\n");
#endif

			if (params->bmp) {
				// note: BMP is bottom to top inverted
				bmpHeader(params->pixMult*(EAN8_W+lpadEAN), params->pixMult*(rows*2 + 6 + EAN8_H), params->outfp);

				// EAN-8
				printElmnts(params, &prints);

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// Composite Component
				prints.elmCnt = elmntsCC;
				prints.height = params->pixMult*2;
				prints.leftPad = lpadCC;
				prints.rightPad = EAN8_R_PAD;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
			else {
				tifHeader(params->pixMult*(EAN8_W+lpadEAN), params->pixMult*(rows*2 + 6 + EAN8_H), params->outfp);

				// Composite Component
				prints.elmCnt = elmntsCC;
				prints.height = params->pixMult*2;
				prints.leftPad = lpadCC;
				prints.rightPad = EAN8_R_PAD;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// EAN-8
				prints.elmCnt = EAN8_ELMNTS;
				prints.pattern = linPattern;
				prints.height = params->pixMult*EAN8_H;
				prints.leftPad = lpadEAN;
				prints.rightPad = 0;
				printElmnts(params, &prints);
			}
		}
		else { // primary only
			if (params->bmp) {
				bmpHeader(params->pixMult*EAN8_W, params->pixMult*EAN8_H, params->outfp);
			}
			else {
				tifHeader(params->pixMult*EAN8_W, params->pixMult*EAN8_H, params->outfp);
			}

			// EAN-8
			printElmnts(params, &prints);
		}
		return;
}


// call with str = 13-digit primary with check digit = 0
int EAN8enc(UCHAR str[], UCHAR pattern[] ) {

static UINT upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
														0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
static UCHAR lGuard[4] = { 7,1,1,1 };
static UCHAR center[5] = { 1,1,1,1,1 };
static UCHAR rGuard[4] = { 1,1,1,7 };

int i, j, bars, sNdx, pNdx;

	// calculate UPC parity
	for (j = 0, i = 0; i < 12; i += 2) {
		j += str[i] - '0';
		j += (str[i+1] - '0') * 3;
	}
	j = (j%10);
	if (j > 0) {
		j = 10 - j;
	}
	str[12] = (UCHAR)(j + '0');

	sNdx = 5;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	for (i = 0; i < 4; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (bars >> j) & 0xf;
		}
	} 
	for (i = 0; i < 5; i++) {
		pattern[pNdx++] = center[i];
	} 
	for (i = 0; i < 4; i++) {
		bars = upcTblA[str[sNdx++]-'0'];
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (bars >> j) & 0xf;
		}
	}
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(TRUE);
}

// includes qz's
#define UPCE_ELMNTS 35
// includes 7X quiet zones:
#define UPCE_W	65
// total ht in x:
#define UPCE_H	74

#define UPCE_L_PAD 3 // UPC-E 7X qz - 4X
#define UPCE_R_PAD 5 // UPCE_W - MAX_WIDTH - UPCE_L_PAD

void UPCE(struct sParams *params) {

struct sPrints prints;
struct sPrints sepPrnt;

UCHAR linPattern[UPCE_ELMNTS];
UCHAR sepPat1[5] = { 7,1,UPCE_W-16,1,7 }; // separator pattern 1
UCHAR sepPat2[5] = { 6,1,UPCE_W-14,1,6 }; // separator pattern 2

char primaryStr[14+1];
char tempStr[28+1];

int i;
int rows, ccFlag;
char *ccStr;

		ccStr = strchr(params->dataStr, '|');
		if (ccStr == NULL) ccFlag = FALSE;
		else {
			ccFlag = TRUE;
			ccStr[0] = '\0'; // separate primary data
			ccStr++; // point to secondary data
		}

		if (strlen(params->dataStr) > 12) {
			errFlag = TRUE;
			printf("\nprimary data exceeds 12 digits");
			return;
		}

		strcpy(tempStr, "000000000000");
		strcat(tempStr, params->dataStr);
		strcat(tempStr, "0"); // check digit = 0 for now
		strcpy(primaryStr, tempStr + strlen(tempStr) - 13);

		if (UPCEenc((UCHAR*)primaryStr, linPattern)) {
			if (errFlag) {
				printf("\nerror occurred, exiting.");
				return;
			}
#if PRNT
			printf("\n%s", primaryStr);
			printf("\n");
			for (i = 0; i < UPCE_ELMNTS; i++) {
				printf("%d", linPattern[i]);
			}
			printf("\n");
#endif
		}
		line1 = TRUE; // so first line is not Y undercut
		// init most likely prints values
		prints.elmCnt = UPCE_ELMNTS;
		prints.pattern = linPattern;
		prints.guards = FALSE;
		prints.height = params->pixMult*UPCE_H;
		prints.leftPad = 0;
		prints.rightPad = 0;
		prints.whtFirst = TRUE;
		prints.reverse = FALSE;
		// init most likely separator values
		sepPrnt.elmCnt = 5;
		sepPrnt.pattern = sepPat1;
		sepPrnt.guards = FALSE;
		sepPrnt.height = params->pixMult*2;
		sepPrnt.leftPad = 0;
		sepPrnt.rightPad = 0;
		sepPrnt.whtFirst = TRUE;
		sepPrnt.reverse = FALSE;
		if (ccFlag) {
			if ((rows = CC2enc((UCHAR*)ccStr, ccPattern)) > 0) {
				if (errFlag) {
					printf("\nComposite encoding error.");
					return;
				}
#if PRNT
				printf("\n%s", ccStr);
				printf("\n");
				for (i = 0; i < rows; i++) {
					for (j = 0; j < CCB2_ELMNTS; j++) {
						printf("%d", ccPattern[i][j]);
					}
					printf("\n");
				}
#endif
			}

			if (params->bmp) {
				// note: BMP is bottom to top inverted
				bmpHeader(params->pixMult*UPCE_W, params->pixMult*(rows*2 + 6 + UPCE_H), params->outfp);

				// UPC-E
				printElmnts(params, &prints);

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// Composite Component
				prints.elmCnt = CCB2_ELMNTS;
				prints.height = params->pixMult*2;
				prints.leftPad = UPCE_L_PAD;
				prints.rightPad = UPCE_R_PAD;
				for (i = rows-1; i >= 0; i--) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}
			}
			else {
				tifHeader(params->pixMult*UPCE_W, params->pixMult*(rows*2 + 6 + UPCE_H), params->outfp);

				// Composite Component
				prints.elmCnt = CCB2_ELMNTS;
				prints.height = params->pixMult*2;
				prints.leftPad = UPCE_L_PAD;
				prints.rightPad = UPCE_R_PAD;
				for (i = 0; i < rows; i++) {
					prints.pattern = ccPattern[i];
					printElmnts(params, &prints);
				}

				// CC separator
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat2;
				printElmnts(params, &sepPrnt);
				sepPrnt.pattern = sepPat1;
				printElmnts(params, &sepPrnt);

				// UPC-E
				prints.elmCnt = UPCE_ELMNTS;
				prints.pattern = linPattern;
				prints.height = params->pixMult*UPCE_H;
				prints.leftPad = 0;
				prints.rightPad = 0;
				printElmnts(params, &prints);
			}
		}
		else { // primary only
			if (params->bmp) {
				bmpHeader(params->pixMult*UPCE_W, params->pixMult*UPCE_H, params->outfp);
			}
			else {
				tifHeader(params->pixMult*UPCE_W, params->pixMult*UPCE_H, params->outfp);
			}

			// UPC-E
			printElmnts(params, &prints);
		}
		return;
}

// call with str = 13-digit primary with check digit = 0
int UPCEenc(UCHAR str[], UCHAR pattern[] ) {

static UINT upcTblA[10] = {	0x3211, 0x2221, 0x2122, 0x1411, 0x1132,
														0x1231, 0x1114, 0x1312, 0x1213, 0x3112 };
static UINT upcTblB[10] = {	0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
														0x1321, 0x4111, 0x2131, 0x3121, 0x2113 };
static UINT abArr[10] = { 0x07,0x0B,0x0D,0x0E,0x13,0x19,0x1C,0x15,0x16,0x1A };
static UCHAR lGuard[4] = { 7,1,1,1 }; 
static UCHAR rGuard[7] = { 1,1,1,1,1,1,7 };

UCHAR data6[6+1];

int i, j, abMask, bars, sNdx, pNdx, abBits;

	// calculate UPC parity
	for (j = 0, i = 0; i < 12; i += 2) {
		j += str[i] - '0';
		j += (str[i+1] - '0') * 3;
	}
	j = (j%10);
	if (j > 0) {
		j = 10 - j;
	}
	str[12] = (UCHAR)(j + '0');

	for (i = 0; i < 5; i++) {
		data6[i] = str[i+2];
	}
	if (str[4] >= '0' && str[4] <= '2' && 
			str[5] == '0' && str[6] == '0' && str[7] == '0' && str[8] == '0') {
		// 00abc0000hij = abhijc, where c = 0-2      
		data6[2] = str[9];
		data6[3] = str[10];
		data6[4] = str[11];
		data6[5] = str[4];
	}
	else if (str[5] == '0' && str[6] == '0' && str[7] == '0' &&
			str[8] == '0' && str[9] == '0') {
		// 00abc00000ij = abcij3
		data6[3] = str[10];
		data6[4] = str[11];
		data6[5] = '3';
	}
	else if (str[6] == '0' && str[7] == '0' &&
			str[8] == '0' && str[9] == '0' && str[10] == '0') {
		// 00abcd00000j = abcdj4
		data6[4] = str[11];
		data6[5] = '4';
	}
	else if (str[11] >= '5' && str[11] <= '9' && str[7] == '0' &&
			str[8] == '0' && str[9] == '0' && str[10] == '0') {
		// 00abcde0000j = abcdej where j = 5-9
		data6[5] = str[11];
	}
	else {
		printf("\nData cannot be converted to UPC-E");
		errFlag = TRUE;
		return(FALSE);
	}

	data6[6] = '\0';
	printf("\n%s", data6);

	sNdx = 0;
	pNdx = 0;
	for (i = 0; i < 4; i++) {
		pattern[pNdx++] = lGuard[i];
	}
	abBits = abArr[str[12]-'0'];
	for (abMask = 0x20, i = 0; i < 6; abMask >>= 1, i++) {
		if ((abBits & abMask) != 0) {
			bars = upcTblA[data6[sNdx++]-'0'];
		}
		else {
			bars = upcTblB[data6[sNdx++]-'0'];
		}
		for (j = 12; j >= 0; j -= 4) {
			pattern[pNdx++] = (bars >> j) & 0xf;
		}
	}
	for (i = 0; i < 7; i++) {
		pattern[pNdx++] = rGuard[i];
	}
	return(TRUE);
}
