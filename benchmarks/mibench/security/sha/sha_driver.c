/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include "timing.h"

int main(int argc, char **argv)
{
    FILE *fin;
    SHA_INFO sha_info;

    start_timing();

    if (argc < 2) {
	fin = stdin;
	sha_stream(&sha_info, fin);
	sha_print(&sha_info);
    } else {
	while (--argc) {
	    fin = fopen(*(++argv), "rb");
	    if (fin == NULL) {
		printf("error opening %s for reading\n", *argv);
	    } else {
		sha_stream(&sha_info, fin);
		sha_print(&sha_info);
		fclose(fin);
	    }
	}
    }

    end_timing();
    print_timing();

    return(0);
}
