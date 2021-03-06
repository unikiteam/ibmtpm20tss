/********************************************************************************/
/*										*/
/*			   PCR_Event 						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: pcrevent.c 945 2017-02-27 23:24:31Z kgoldman $		*/
/*										*/
/* (c) Copyright IBM Corporation 2015.						*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

/* 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tssresponsecode.h>

static void printUsage(void);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    PCR_Event_In 		in;
    PCR_Event_Out 		out;
    TPMI_DH_PCR 		pcrHandle = IMPLEMENTATION_PCR;
    const char 			*data = NULL;
    const char 			*datafilename = NULL;
    const char			*outFilename1 = NULL;
    const char			*outFilename2 = NULL;
    const char			*outFilename3 = NULL;
   
    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1");

    /* command line argument defaults */
    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-ha") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%u", &pcrHandle);
	    }
	    else {
		printf("Missing parameter for -ha\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ic") == 0) {
	    i++;
	    if (i < argc) {
		data = argv[i];
	    }
	    else {
		printf("-ic option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-if")  == 0) {
	    i++;
	    if (i < argc) {
		datafilename = argv[i];
	    } else {
		printf("-if option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-of1")  == 0) {
	    i++;
	    if (i < argc) {
		outFilename1 = argv[i];
	    } else {
		printf("-of1 option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-of2")  == 0) {
	    i++;
	    if (i < argc) {
		outFilename2 = argv[i];
	    } else {
		printf("-of2 option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-of3")  == 0) {
	    i++;
	    if (i < argc) {
		outFilename3 = argv[i];
	    } else {
		printf("-of3 option needs a value\n");
		printUsage();
	    }
	}
 	else if (strcmp(argv[i],"-h") == 0) {
	    printUsage();
	}
	else if (strcmp(argv[i],"-v") == 0) {
	    verbose = TRUE;
	    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "2");
	}
	else {
	    printf("\n%s is not a valid option\n", argv[i]);
	    printUsage();
	}
    }
    if (pcrHandle >= IMPLEMENTATION_PCR) {
	printf("Missing or bad PCR handle parameter -ha\n");
	printUsage();
    }
    if ((data == NULL) && (datafilename == NULL)) {
	printf("Data string or data file must be specified\n");
	printUsage();
    }
    if ((data != NULL) && (datafilename != NULL)) {
	printf("Data string and data file cannot both be specified\n");
	printUsage();
    }
    if (rc == 0) {
	in.pcrHandle = pcrHandle;
    }
    if (rc == 0) {
	if (data != NULL) {
	    if (verbose) printf("Event data %u bytes\n", (unsigned int)strlen(data));
	    rc = TSS_TPM2B_StringCopy(&in.eventData.b, data, 1024);
	}
    }
    if (datafilename != NULL) {
	rc = TSS_File_Read2B(&in.eventData.b,
			     1024,
			     datafilename);
    }
    /* Start a TSS context */
    if (rc == 0) {
	rc = TSS_Create(&tssContext);
    }
    /* call TSS to execute the command */
    if (rc == 0) {
	rc = TSS_Execute(tssContext,
			 (RESPONSE_PARAMETERS *)&out, 
			 (COMMAND_PARAMETERS *)&in,
			 NULL,
			 TPM_CC_PCR_Event,
			 TPM_RS_PW, NULL, 0,
			 TPM_RH_NULL, NULL, 0);
    }
    {
	TPM_RC rc1 = TSS_Delete(tssContext);
	if (rc == 0) {
	    rc = rc1;
	}
    }
    if (rc == 0) {
	uint32_t c;
	printf("pcrevent: success\n");
	/* Table 100 - Definition of TPML_DIGEST_VALUES Structure */
	/* Table 71 - Definition of TPMT_HA Structure <IN/OUT> digests[] */
	/* Table 70 - Definition of TPMU_HA Union <IN/OUT, S> digests */
	printf("pcrevent: count %u\n", out.digests.count);

	for (c = 0 ;  c < out.digests.count ;c++) {
	    switch (out.digests.digests[c].hashAlg) {
	      case TPM_ALG_SHA1:
		if (verbose) printf("Hash algorithm SHA-1\n");
		if (verbose) TSS_PrintAll("Digest",
					  (uint8_t *)&out.digests.digests[c].digest.sha1,
					  SHA1_DIGEST_SIZE);
		if (outFilename1 != NULL) {
		    rc = TSS_File_WriteBinaryFile((uint8_t *)&out.digests.digests[c].digest.sha1,
						  SHA1_DIGEST_SIZE,
						  outFilename1); 
		}
		break;
	      case TPM_ALG_SHA256:
		if (verbose) printf("Hash algorithm SHA-256\n");
		if (verbose) TSS_PrintAll("Digest",
					  (uint8_t *)&out.digests.digests[c].digest.sha256,
					  SHA256_DIGEST_SIZE);
		if (outFilename2 != NULL) {
		    rc = TSS_File_WriteBinaryFile((uint8_t *)&out.digests.digests[c].digest.sha256,
						  SHA256_DIGEST_SIZE,
						  outFilename2); 
		}
		break;
	      case TPM_ALG_SHA384:
		if (verbose) printf("Hash algorithm SHA-384\n");
		if (verbose) TSS_PrintAll("Digest",
					  (uint8_t *)&out.digests.digests[c].digest.sha384,
					  SHA384_DIGEST_SIZE);
		if (outFilename3 != NULL) {
		    rc = TSS_File_WriteBinaryFile((uint8_t *)&out.digests.digests[c].digest.sha384,
						  SHA384_DIGEST_SIZE,
						  outFilename3); 
		}
		break;
	      default:
		printf("Hash algorithm %04x unknown\n", out.digests.digests[c].hashAlg);
		break;
	    }
	}
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("pcrevent: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printUsage(void)
{
    printf("\n");
    printf("pcrevent\n");
    printf("\n");
    printf("Runs TPM2_PCR_Event\n");
    printf("\n");
    printf("\t-ha pcr handle\n");
    printf("\t-ic data string\n");
    printf("\t-if data file\n");
    printf("\t[-of1 sha1 output digest file]\n");
    printf("\t[-of2 sha256 output digest file]\n");
    printf("\t[-of3 sha384 output digest file]\n");
   exit(1);	
}
