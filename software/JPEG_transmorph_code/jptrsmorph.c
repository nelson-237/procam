/*
 * jptrsmorph.c
 *
 * Copyright (C) 2015, Lin Yuan (lin.yuan@epfl.ch).
 * This file belongs to Multimedia Signal Processing Group .
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for JPEG transmorphing.
 * It takes two JPEG images and a
 * It is very similar to cjpeg.c, but provides lossless transcoding between
 * different JPEG file formats.  It also provides some lossless and sort-of-
 * lossless transformations of JPEG data.
 */

#include "src/cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "src/transupp.h"	/* Support routines for jpegtran */
#include "src/jversion.h"	/* for version message */

#ifdef USE_CCOMMAND		/* command-line reader for Macintosh */
#ifdef __MWERKS__
#include <SIOUX.h>              /* Metrowerks needs this */
#include <console.h>		/* ... and this */
#endif
#ifdef THINK_C
#include <console.h>		/* Think declares it here */
#endif
#endif

/* For reading the txt file. (Lin YUAN) */
#include <stdio.h>
#include <stdlib.h> /* srand, rand */
#include <string.h>

/* Math lib for ceil/floor/round operations */
#include <math.h>


/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static const char * progname;	/* program name for error messages */
static char * outfilename;	    /* for -outfile switch */
static JCOPY_OPTION copyoption;	/* -copy switch */
static jpeg_transform_info transformoption; /* image transformation options */

static char * maskfilename;	    /* mask filename */
static int transMode = 0;       /* mode of transcoding: 0-normal JPEG transcoding; 3-JPEG Transmorphing; 4-JPEG Re-Transmorphing. */
static boolean scrambleOption = 0;     /* Scramble the image or not. Default 0*/
static long long int key;       /* Scrambling key. */
static char * static_output_filename = "temp.jpg";

/* Number of MCUs */
static int mcu_counter = 0;

#define MASKMATX 3500
#define MASKMATY 2500

//static int maskMat[5000][5000]; /* Mask matrix */
static int maskMat[MASKMATX][MASKMATY]; /* Mask matrix -- procam update */
static int maskArray[25000000];

#define TRANSMORPH 3
#define RETRANSMORPH 4

LOCAL(void)
usage (void)
/* complain about bad command line */
{
	fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
	fprintf(stderr, "inputfile outputfile\n");
#else
	fprintf(stderr, "[inputfile]\n");
#endif
	
	fprintf(stderr, "Switches (names may be abbreviated):\n");
	
	/* Transmorph options. Added by Lin YUAN (lin.yuan@epfl.ch) EPFL. */
	fprintf(stderr, "  -morph maskfile       Transmorph the image according to the mask matrix defined in a text file.\n");
	fprintf(stderr, "  -remorph       Remorph the image by extracting sub-image from APP9 markers and replace corresponding parts in morphed image.\n");
	fprintf(stderr, "  -key  key      The key to scramble or descramble the embedded sub-image. If not set, sub-image is not scrambled.\n");
	fprintf(stderr, "  -copy none     Copy no extra markers from source file\n");
	fprintf(stderr, "  -copy comments Copy only comment markers (default)\n");
	fprintf(stderr, "  -copy all      Copy all extra markers\n");
#ifdef ENTROPY_OPT_SUPPORTED
	fprintf(stderr, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef C_PROGRESSIVE_SUPPORTED
	fprintf(stderr, "  -progressive   Create progressive JPEG file\n");
#endif
#if TRANSFORMS_SUPPORTED
	fprintf(stderr, "Switches for modifying the image:\n");
	fprintf(stderr, "  -grayscale     Reduce to grayscale (omit color data)\n");
	fprintf(stderr, "  -flip [horizontal|vertical]  Mirror image (left-right or top-bottom)\n");
	fprintf(stderr, "  -rotate [90|180|270]         Rotate image (degrees clockwise)\n");
	fprintf(stderr, "  -transpose     Transpose image\n");
	fprintf(stderr, "  -transverse    Transverse transpose image\n");
	fprintf(stderr, "  -trim          Drop non-transformable edge blocks\n");
#endif /* TRANSFORMS_SUPPORTED */
	fprintf(stderr, "Switches for advanced users:\n");
	fprintf(stderr, "  -restart N     Set restart interval in rows, or in blocks with B\n");
	fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
	fprintf(stderr, "  -outfile name  Specify name for output file\n");
	fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
	fprintf(stderr, "Switches for wizards:\n");
#ifdef C_ARITH_CODING_SUPPORTED
	fprintf(stderr, "  -arithmetic    Use arithmetic coding\n");
#endif
#ifdef C_MULTISCAN_FILES_SUPPORTED
	fprintf(stderr, "  -scans file    Create multi-scan JPEG per script file\n");
#endif
	exit(EXIT_FAILURE);
}


LOCAL(void)
select_transform (JXFORM_CODE transform)
/* Silly little routine to detect multiple transform options,
 * which we can't handle.
 */
{
#if TRANSFORMS_SUPPORTED
	if (transformoption.transform == JXFORM_NONE ||
		transformoption.transform == transform) {
		transformoption.transform = transform;
	} else {
		fprintf(stderr, "%s: can only do one image transformation at a time\n",
				progname);
		usage();
	}
#else
	fprintf(stderr, "%s: sorry, image transformation was not compiled\n",
	  progname);
	exit(EXIT_FAILURE);
#endif
}

/* Function to read mask matrix into maskMat from a text file txtFilename. Added by Lin YUAN (lin.yuan@epfl.ch). */
void readMaskFromText (char *txtFilename)
{
	// If image_width is not 0, it means the jpeg file is read already.
	fprintf(stderr, "  Mask filename: %s\n", txtFilename);
	
	FILE *file;
	char *buffer;
	int ret,row=0,i,j;
	int rows=5000, cols=5000;
	
	// Initilize the scrambling mask matrix to 0
	for (i = 0; i < rows; i++) {
		for (j = 0; j < rows; j++)
			maskMat[i][j] = 0;
	}
	
	// Set Field Separator here
	char delims[]=" \t"; //\t";
	char *result=NULL;
	
	// Memory allocation
	int **mat = malloc( rows*sizeof(int*));
	for(i = 0; i < rows; i++)
		mat[i] = malloc( cols*sizeof(int));
	
	if ((file = fopen(txtFilename, "r")) == NULL){
		fprintf(stderr, "Error: Can't open TXT file !\n");
		return;
	}
	
	while(!feof(file))
	{
		buffer = malloc(sizeof(char) * 25000000);
		memset(buffer, 0, 25000000);
		ret = fscanf(file, "%65533[^\n]\n", buffer);
		if (ret != EOF) {
			int field = 0;
			result = strtok(buffer,delims);
			while(result!=NULL){
				// Set no of fields according to your requirement
				if(field>25000000) break;
				mat[row][field]=atof(result);
				result=strtok(NULL,delims);
				field++;
			}
			++row;
		}
		free(buffer);
	}
	
	fclose(file);
	
	//	FILE *fp2;
	//	if ((fp2 = fopen("scramble_mask_array.txt", "w")) == NULL)
	//		printf("cannot open file!\n");
	
	for(i=0;i<rows;i++){
		for(j=0;j<cols;j++) {
			maskMat[i][j] = mat[i][j];
			// Write the matrix into a text file: scramble_mask_array.txt
			//			fprintf(fp2, "%d%s", mat[i][j], j < cols-1 ? " " : "\n");
		}
		free(mat[i]);
	}
	
	//	fclose(fp2);
	free(mat);
}



LOCAL(int)
parse_switches (j_compress_ptr cinfo, int argc, char **argv,
				int last_file_arg_seen, boolean for_real)
/* Parse optional switches.
 * Returns argv[] index of first file-name argument (== argc if none).
 * Any file names with indexes <= last_file_arg_seen are ignored;
 * they have presumably been processed in a previous iteration.
 * (Pass 0 for last_file_arg_seen on the first or only iteration.)
 * for_real is FALSE on the first (dummy) pass; we may skip any expensive
 * processing.
 */
{
	int argn;
	char * arg;
	boolean simple_progressive;
	char * scansarg = NULL;	/* saves -scans parm if any */
	
	/* Set up default JPEG parameters. */
	simple_progressive = FALSE;
	outfilename = NULL;
	// procam update
	//copyoption = JCOPYOPT_DEFAULT;
	copyoption = JCOPYOPT_ALL;
	transformoption.transform = JXFORM_NONE;
	transformoption.trim = FALSE;
	transformoption.force_grayscale = FALSE;
	cinfo->err->trace_level = 0;
	
	/* Scan command line options, adjust parameters */
	
	for (argn = 1; argn < argc; argn++) {
		arg = argv[argn];
		if (*arg != '-') {
			/* Not a switch, must be a file name argument */
			if (argn <= last_file_arg_seen) {
				outfilename = NULL;	/* -outfile applies to just one input file */
				continue;		/* ignore this name if previously processed */
			}
			break;			/* else done parsing switches */
		}
		arg++;			/* advance past switch marker character */
		
		if (keymatch(arg, "morph", 1)) {
			
			// procam update -- no need a maskfile. it is generated in-memory, on the fly. see procam updates in this file.
			//if (++argn >= argc)	/* advance to next argument */
			if (argn >= argc)	/* advance to next argument */
				usage();
			
			/* JPEG Transmorphing mode. transMode = 1 */
			transMode = TRANSMORPH;
			
			// procam update -- no need a maskfile. it is generated in-memory, on the fly. see procam updates in this file.
			//maskfilename = argv[argn];
			
		} else if (keymatch(arg, "remorph", 1)) {
			
			/* JPEG Transmorphing reconstruction mode. transMode = 2 */
			transMode = RETRANSMORPH;
			
		} else if (keymatch(arg, "key", 3)) {
			if (++argn >= argc)	/* advance to next argument */
				usage();
			scrambleOption = 1;
			
			/* scrambling key. %llu specifies the long long int value */
			if (sscanf(argv[argn], "%llu", &key) != 1)
				usage();
			
		} else if (keymatch(arg, "arithmetic", 1)) {
			/* Use arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
			cinfo->arith_code = TRUE;
#else
			fprintf(stderr, "%s: sorry, arithmetic coding not supported\n",
					progname);
			exit(EXIT_FAILURE);
#endif
			
		} else if (keymatch(arg, "copy", 1)) {
			/* Select which extra markers to copy. */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			if (keymatch(argv[argn], "none", 1)) {
				copyoption = JCOPYOPT_NONE;
			} else if (keymatch(argv[argn], "comments", 1)) {
				copyoption = JCOPYOPT_COMMENTS;
			} else if (keymatch(argv[argn], "all", 1)) {
				copyoption = JCOPYOPT_ALL;
			} else
				usage();
			
		} else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
			/* Enable debug printouts. */
			/* On first -d, print version identification */
			static boolean printed_version = FALSE;
			
			if (! printed_version) {
				fprintf(stderr, "Independent JPEG Group's JPEGTRAN, version %s\n%s\n",
						JVERSION, JCOPYRIGHT);
				printed_version = TRUE;
			}
			cinfo->err->trace_level++;
			
		} else if (keymatch(arg, "flip", 1)) {
			/* Mirror left-right or top-bottom. */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			if (keymatch(argv[argn], "horizontal", 1))
				select_transform(JXFORM_FLIP_H);
			else if (keymatch(argv[argn], "vertical", 1))
				select_transform(JXFORM_FLIP_V);
			else
				usage();
			
		} else if (keymatch(arg, "grayscale", 1) || keymatch(arg, "greyscale",1)) {
			/* Force to grayscale. */
#if TRANSFORMS_SUPPORTED
			transformoption.force_grayscale = TRUE;
#else
			select_transform(JXFORM_NONE);	/* force an error */
#endif
			
		} else if (keymatch(arg, "maxmemory", 3)) {
			/* Maximum memory in Kb (or Mb with 'm'). */
			long lval;
			char ch = 'x';
			
			if (++argn >= argc)	/* advance to next argument */
				usage();
			if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
				usage();
			if (ch == 'm' || ch == 'M')
				lval *= 1000L;
			cinfo->mem->max_memory_to_use = lval * 1000L;
			
		} else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
			/* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
			cinfo->optimize_coding = TRUE;
#else
			fprintf(stderr, "%s: sorry, entropy optimization was not compiled\n",
					progname);
			exit(EXIT_FAILURE);
#endif
			
		} else if (keymatch(arg, "outfile", 4)) {
			/* Set output file name. */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			outfilename = argv[argn];	/* save it away for later use */
			
		} else if (keymatch(arg, "progressive", 1)) {
			/* Select simple progressive mode. */
#ifdef C_PROGRESSIVE_SUPPORTED
			simple_progressive = TRUE;
			/* We must postpone execution until num_components is known. */
#else
			fprintf(stderr, "%s: sorry, progressive output was not compiled\n",
					progname);
			exit(EXIT_FAILURE);
#endif
			
		} else if (keymatch(arg, "restart", 1)) {
			/* Restart interval in MCU rows (or in MCUs with 'b'). */
			long lval;
			char ch = 'x';
			
			if (++argn >= argc)	/* advance to next argument */
				usage();
			if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
				usage();
			if (lval < 0 || lval > 65535L)
				usage();
			if (ch == 'b' || ch == 'B') {
				cinfo->restart_interval = (unsigned int) lval;
				cinfo->restart_in_rows = 0; /* else prior '-restart n' overrides me */
			} else {
				cinfo->restart_in_rows = (int) lval;
				/* restart_interval will be computed during startup */
			}
			
		} else if (keymatch(arg, "rotate", 2)) {
			/* Rotate 90, 180, or 270 degrees (measured clockwise). */
			if (++argn >= argc)	/* advance to next argument */
				usage();
			if (keymatch(argv[argn], "90", 2))
				select_transform(JXFORM_ROT_90);
			else if (keymatch(argv[argn], "180", 3))
				select_transform(JXFORM_ROT_180);
			else if (keymatch(argv[argn], "270", 3))
				select_transform(JXFORM_ROT_270);
			else
				usage();
			
		} else if (keymatch(arg, "scans", 1)) {
			/* Set scan script. */
#ifdef C_MULTISCAN_FILES_SUPPORTED
			if (++argn >= argc)	/* advance to next argument */
				usage();
			scansarg = argv[argn];
			/* We must postpone reading the file in case -progressive appears. */
#else
			fprintf(stderr, "%s: sorry, multi-scan output was not compiled\n",
					progname);
			exit(EXIT_FAILURE);
#endif
			
		} else if (keymatch(arg, "transpose", 1)) {
			/* Transpose (across UL-to-LR axis). */
			select_transform(JXFORM_TRANSPOSE);
			
		} else if (keymatch(arg, "transverse", 6)) {
			/* Transverse transpose (across UR-to-LL axis). */
			select_transform(JXFORM_TRANSVERSE);
			
		} else if (keymatch(arg, "trim", 3)) {
			/* Trim off any partial edge MCUs that the transform can't handle. */
			transformoption.trim = TRUE;
			
		} else {
			usage();			/* bogus switch */
		}
	}
	
	/* Post-switch-scanning cleanup */
	
	if (for_real) {
		
#ifdef C_PROGRESSIVE_SUPPORTED
		if (simple_progressive)	/* process -progressive; -scans can override */
			jpeg_simple_progression(cinfo);
#endif
		
#ifdef C_MULTISCAN_FILES_SUPPORTED
		if (scansarg != NULL)	/* process -scans if it was present */
			if (! read_scan_script(cinfo, scansarg))
				usage();
#endif
	}
	
	return argn;			/* return index of next arg (file name) */
}


/* 
 * Function to write a file as bytes into JPEG Marker. Added by Lin YUAN (lin.yuan@epfl.ch). 
 */
void writeFileToMarker (j_compress_ptr cinfo, char *filename, int markerIndex, long bufsize)
{
	char *source = NULL;
	FILE *fp3 = fopen(filename, "r");
	if (fp3 != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp3, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
//			long bufsize = ftell(fp3);
//			if (bufsize == -1) { /* Error */ }
			
			/* Allocate our buffer to that size. */
			source = malloc(sizeof(char) * (bufsize + 1));
			
			/* Go back to the start of the file. */
			if (fseek(fp3, 0L, SEEK_SET) == 0) { /* Error */ }
			
			/* Read the entire file into memory. */
			size_t newLen = fread(source, sizeof(char), bufsize, fp3);
			if (newLen == 0) {
				fputs("Error reading file", stderr);
			} else {
				source[++newLen] = '\0'; /* Just to be safe. */
			}
			
//			fprintf(stderr, "The file to be embedded has %ld bytes.\n\n", bufsize);
			
			/* The first mark records some information about the morphing, e.g. the number of bytes that are embedded in the header. */
			// Beause bufsize is a long int which is larger than 1 byte, seperate the long into several bytes and write each byte to marker.
//			unsigned int byteSize = sizeof(bufsize);
//			//			fprintf(stderr, "byteSize %d\n", byteSize);
//			jpeg_write_m_header(cinfo, JPEG_APP0 + markerIndex, byteSize + 1);
//			//			int shift[16] = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 };
//			jpeg_write_m_byte(cinfo, byteSize); // First byte in the APP marker denotes the byte number of the long int (the size of embedded file)
//			int x;
//			for (x = 0; x < byteSize; x++) {
//				jpeg_write_m_byte(cinfo, (int)((bufsize >> 8*(byteSize-1-x)) & 0xFF));
//			}
			
			int num_makers, i_marker, i_byte;
            /* Limit of each marker is 65533 bytes (Lin Yuan) */
			if (bufsize > 65533) {
				// If the number of bytes is larger than 65533, segment the information into [num_makers] markers
				num_makers = bufsize/65533;
				
				// Process the first few segments
				for (i_marker=0; i_marker<num_makers; i_marker++) {
                    /* Use jpeg_write_m_header() to create an APP marker */
					jpeg_write_m_header(cinfo, JPEG_APP0 + markerIndex, (unsigned int) (65533));
					for (i_byte=0; i_byte<65533; i_byte++) {
                        /* Use jpeg_write_m_byte() to write each byte into the app marker */
						jpeg_write_m_byte(cinfo, source[ i_marker*65533 + i_byte]);
					}
				}
				
				if (bufsize % 65533 != 0) {
					// Process the last segment
					jpeg_write_m_header(cinfo, JPEG_APP0 + markerIndex, (unsigned int) (bufsize % 65533));
					for (i_byte=0; i_byte < (bufsize % 65533); i_byte++) {
						jpeg_write_m_byte(cinfo, source[ num_makers*65533 + i_byte]);
					}
				}
				
			} else {
				// Buffer size is smaller than MAX size
				jpeg_write_m_header(cinfo, JPEG_APP0 + markerIndex, (unsigned int) (bufsize));
				for (i_byte=0; i_byte < bufsize; i_byte++) {
					jpeg_write_m_byte(cinfo, source[i_byte]);
				}
			}
		}
		fclose(fp3);
	}
	free(source); /* Don't forget to call free() later! */
}

/*
 * Write 8 bits into one byte
 */
unsigned int bits2byte( int * bits ) {
	unsigned int sum = 0;
	int i;
	for (i = 7; i >= 0; i--) {
		sum = (sum << 1) | bits[i];
	}
	
	return sum;
}

/*
 * Read 8 bits from one byte
 */
int * byte2bits( unsigned int byte ) {
	static int bits[8];
	int i;
	for (i = 0; i < 8; i++) {
		bits[i] = (byte >> i) & 1;
	}
	
	return bits;
}

/*
 * The main program.
 */

int
main (int argc, char **argv)
{
	fprintf(stderr, "NUMBER OF PARAMETERS: %d\n", argc);
	
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_compress_struct dstinfo;
	struct jpeg_error_mgr jsrcerr, jdsterr;
#ifdef PROGRESS_REPORT
	struct cdjpeg_progress_mgr progress;
#endif
	jvirt_barray_ptr * src_coef_arrays;
	jvirt_barray_ptr * dst_coef_arrays;
	int file_index;
	FILE * input_file;
	FILE * output_file;
	
	/* On Mac, fetch a command line. */
#ifdef USE_CCOMMAND
	argc = ccommand(&argv);
#endif
	
	progname = argv[0];
	if (progname == NULL || progname[0] == 0)
		progname = "jpegtran";	/* in case C library doesn't provide it */
	
	/* Initialize the JPEG decompression object with default error handling. */
	srcinfo.err = jpeg_std_error(&jsrcerr);
	jpeg_create_decompress(&srcinfo);
	/* Initialize the JPEG compression object with default error handling. */
	dstinfo.err = jpeg_std_error(&jdsterr);
	jpeg_create_compress(&dstinfo);
	
	/* Now safe to enable signal catcher.
	 * Note: we assume only the decompression object will have virtual arrays.
	 */
#ifdef NEED_SIGNAL_CATCHER
	enable_signal_catcher((j_common_ptr) &srcinfo);
#endif
	
	/* Scan command line to find file names.
	 * It is convenient to use just one switch-parsing routine, but the switch
	 * values read here are mostly ignored; we will rescan the switches after
	 * opening the input file.  Also note that most of the switches affect the
	 * destination JPEG object, so we parse into that and then copy over what
	 * needs to affects the source too.
	 */
	
	file_index = parse_switches(&dstinfo, argc, argv, 0, FALSE);
	jsrcerr.trace_level = jdsterr.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;
	
#ifdef TWO_FILE_COMMANDLINE
	/* Must have either -outfile switch or explicit output file name */
	if (outfilename == NULL) {
		if (file_index != argc-2) {
			fprintf(stderr, "%s: must name one input and one output file\n",
					progname);
			usage();
		}
		outfilename = argv[file_index+1];
		fprintf(stderr, "Out put file is %s\n", outfilename);
	} else {
		if (file_index != argc-1) {
			fprintf(stderr, "%s: must name one input and one output file\n",
					progname);
			usage();
		}
	}
	
#else
	/* Unix style: expect zero or one file name */
	//  if (file_index < argc-1) {
	//    fprintf(stderr, "%s: only one input file\n", progname);
	//    usage();
	//  }
	if (file_index < argc-4) {
		fprintf(stderr, "%s: only one input file\n", progname);
		usage();
	}
#endif /* TWO_FILE_COMMANDLINE */
	
	
	/* Open the input file. */
	if (file_index < argc) {
		if ((input_file = fopen(argv[file_index], READ_BINARY)) == NULL) {
			fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index]);
			exit(EXIT_FAILURE);
		}
		// procam update -- static outfile is assigned as it does not produce what we need
		//if ((output_file = fopen(argv[file_index+1], WRITE_BINARY)) == NULL) {
		if ((output_file = fopen(static_output_filename, WRITE_BINARY)) == NULL) {
			//fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index+1]);
			fprintf(stderr, "%s: can't open %s\n", progname, static_output_filename);
			exit(EXIT_FAILURE);
		}
	} else {
		/* default input file is stdin */
		input_file = read_stdin();
		/* default output file is stdout */
		output_file = write_stdout();
	}

#ifdef PROGRESS_REPORT
	start_progress_monitor((j_common_ptr) &dstinfo, &progress);
#endif
	
	/* Specify data source for decompression */
	jpeg_stdio_src(&srcinfo, input_file);
	
	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup(&srcinfo, copyoption);
	
	/* Read file header */
	(void) jpeg_read_header(&srcinfo, TRUE);
	
	/* Any space needed by a transform option must be requested before
	 * jpeg_read_coefficients so that memory allocation will be done right.
	 */
#if TRANSFORMS_SUPPORTED
	jtransform_request_workspace(&srcinfo, &transformoption);
#endif
	
	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients(&srcinfo);
	
	/* scramblign is enbled. */
	if (scrambleOption) {
		/* Initialize scrambling random generator. */
		srand(key);
	}
	
	/* JPEG Transmorphing (transMode = 3) and Reconstruction (transMode = 4) */
	if (transMode == TRANSMORPH) {
		fprintf(stderr, "---------- Morphing ----------\n");
		
		// procam update 
		//fprintf(stderr, "  Input file is %s\n", argv[file_index+2]);
		//fprintf(stderr, "  Output file is %s\n", argv[file_index+3]);
		fprintf(stderr, "  Input file is %s\n", argv[file_index+1]);
		fprintf(stderr, "  Output file is %s\n", argv[file_index+2]);
		
		// procam update
		/* Read mask matrix from maskfile */
		//readMaskFromText(maskfilename);
		// statically assingn the mask to all 1s for the defined sizes (see MASKMATX and MASKMATY constants above)
		// Initilize the scrambling mask matrix to 0
		int i,j;
         	for (i = 0; i < MASKMATX; i++) {
                	for (j = 0; j < MASKMATY; j++)
                        	maskMat[i][j] = 1;
          	}	
	
		/* Process DCT in the following code */
		JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN];
		//	JBLOCKROW MCU_buffer[C_MAX_BLOCKS_IN_MCU];
		JBLOCKROW buffer_ptr;
		JDIMENSION MCU_col_num;	/* index of current MCU within row */
		JDIMENSION last_MCU_col = srcinfo.MCUs_per_row - 1;
		JDIMENSION last_iMCU_row = srcinfo.total_iMCU_rows - 1;
		jpeg_component_info *compptr;
		JDIMENSION iMCU_row_num = 0;
		JDIMENSION start_col;
		/* Align the virtual buffers for the components used in this scan. */
		int blkn, ci, xindex, yindex, yoffset, blockcnt;
		int MCU_vert_offset = 0;
		int MCU_rows_per_iMCU_row = 1;
		int mcu_ctr = 0;
		int blkindex, k;
		
		int h_factor = (srcinfo.max_h_samp_factor==1)? 2:1;
		int v_factor = (srcinfo.max_v_samp_factor==1)? 2:1;
		
		/* int array storing the mask matrix one mcu by one mcu */
		//		int maskArray[65533 * 2];
		
		JDIMENSION MCU_total_rows = ceil( ( (float) srcinfo.image_height / 8 ) / srcinfo.max_v_samp_factor );
		fprintf(stderr, "  Image width: %d; height: %d\n", srcinfo.image_width, srcinfo.image_height);
		fprintf(stderr, "  Total MCU rows: %d\n", MCU_total_rows);
		fprintf(stderr, "  Total MCU cols: %d\n", srcinfo.MCUs_per_row);
		fprintf(stderr, "  max_h_samp_factor: %d\n", srcinfo.max_h_samp_factor);
		fprintf(stderr, "  max_v_samp_factor: %d\n", srcinfo.max_v_samp_factor);
		
		for (iMCU_row_num = 0; iMCU_row_num < MCU_total_rows; iMCU_row_num++) {
			for (ci = 0; ci < srcinfo.comps_in_scan; ci++) {
				compptr = srcinfo.cur_comp_info[ci];
				buffer[ci] = (srcinfo.mem->access_virt_barray) ((j_common_ptr) &srcinfo, src_coef_arrays[compptr->component_index], iMCU_row_num * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor, FALSE);
			}
			//		fprintf(stderr, "MCU Row: %d\n", iMCU_row_num);
			for (yoffset = MCU_vert_offset; yoffset < MCU_rows_per_iMCU_row; yoffset++) {
				for (MCU_col_num = mcu_ctr; MCU_col_num < srcinfo.MCUs_per_row; MCU_col_num++) {
					/* Construct list of pointers to DCT blocks belonging to this MCU */
					//				fprintf(stderr, "MCU Col: %d\n", MCU_col_num);
					blkn = 0;			/* index of current DCT block within MCU */
					for (ci = 0; ci < srcinfo.comps_in_scan; ci++) {
						compptr = srcinfo.cur_comp_info[ci];
						start_col = MCU_col_num * compptr->MCU_width;
						blockcnt = (MCU_col_num < last_MCU_col) ? compptr->MCU_width : compptr->last_col_width;
						for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
							if (iMCU_row_num < last_iMCU_row || yindex+yoffset < compptr->last_row_height) {
								/* Fill in pointers to real blocks in this row */
								buffer_ptr = buffer[ci][yindex+yoffset] + start_col;
								/****************** Morphing. Added by Lin YUAN (lin.yuan@epfl.ch) EPFL.*********************/
								if ( maskMat[iMCU_row_num/v_factor][MCU_col_num/h_factor] == 0 ) {
									//								fprintf(stderr, "mcu_counter: %d\n", mcu_counter);
									maskArray[ mcu_counter ] = 0;
									// blkindex: index of DCT block within the current MCU. k: index of DCT coefficients.
									for (blkindex = 0; blkindex < blockcnt; blkindex++) {
										for (k = 0; k < 64; k++) {
											buffer_ptr[blkindex][k] = 0; // Set all DCT to 0 outside the selected region.
										}
									}
								} else {
									maskArray[ mcu_counter ] = 1;
									// Scrambling each DCT blocks
									if (scrambleOption) {
										for (blkindex = 0; blkindex < blockcnt; blkindex++) {
											for (k = 0; k < 64; k++) {
												/* Scramble the DCT parameters of the selected region. */
												buffer_ptr[blkindex][k] = ((rand()%2)*2-1) * buffer_ptr[blkindex][k];
											}
										}
									}
								}
								//							for (xindex = 0; xindex < blockcnt; xindex++)
								//								MCU_buffer[blkn++] = buffer_ptr++;
							} else {
								/* At bottom of image, need a whole row of dummy blocks */
								xindex = 0;
							}
						}
					}
					mcu_counter ++;
				}
			}
		}
		
		fprintf(stderr, "  Number of MCU blocks: %d\n", mcu_counter);
		
	} else if (transMode == RETRANSMORPH) {
		fprintf(stderr, "---------- Re-Morphing ----------\n");
		/* Added by Lin YUAN (lin.yuan@epfl.ch)*/
		/* Read embedded sub-image from JPEG APP9 marker, save it to a JPEG file. */
		// procam update
		//char *subimageName = argv[file_index+2];
		char *subimageName = argv[file_index+1];
		FILE *subimgFile = fopen(subimageName, "w");
		char *bufArray = NULL;
		int row, col, bytes_filesize, x, y, mCounter = 0;
		long bufsize = 0;
		jpeg_saved_marker_ptr marker, maskArrayMarker;
		
		/* Extract the sub-image from the APP9 marker */
		int num_makers, marker_counter = 0;
		
		for (marker = srcinfo.marker_list; marker != NULL; marker = marker->next) {
			if (marker->marker == JPEG_APP0 + 11) { // APP11 marker that stores mask array
				if (marker_counter == 0) {
					fprintf(stderr, "  APP11 marker found.\n");
					maskArrayMarker = marker; // maskArrayMarker points to APP11 marker
					
					int protection_id = GETJOCTET( maskArrayMarker->data[0] );
					if (protection_id == 3) {
						fprintf(stderr, "  Protection: JPEG Transmorphing.\n");
					}
					bytes_filesize = GETJOCTET( maskArrayMarker->data[1] );
					for (x = 0; x < bytes_filesize; x++) {
						bufsize += ( GETJOCTET(maskArrayMarker->data[2 + x]) << 8*(bytes_filesize-1-x) );
					}
					
					/* Number of markers storing the subimage bytes */
					num_makers = ceil( bufsize/65533.0 );
					
					/* Allocate bufArray to that size. */
					bufArray = malloc(sizeof(char) * (bufsize + 1));
					
				} else if ( marker_counter >= 1 && marker_counter < num_makers) {
					for (y = 0; y < 65533; y++) {
						bufArray[ 65533 * (marker_counter - 1) + y ] = GETJOCTET(marker->data[y]);
					}
				} else if (marker_counter == num_makers) { // The last marker
					if ( bufsize % 65533 == 0) {
						for (y = 0; y < 65533; y++) {
							bufArray[ 65533 * (marker_counter - 1) + y ] = GETJOCTET(marker->data[y]);
						}
					} else {
						for (y = 0; y < bufsize%65533; y++) {
							bufArray[ 65533 * (marker_counter - 1) + y ] = GETJOCTET(marker->data[y]);
						}
					}
				}
				marker_counter++;
			}
		}
		
//			} else if (marker->marker == JPEG_APP0 + 10) {
//				fprintf(stderr, "  APP10 marker found.\n");
//				if (mCounter == 0) { // The first APP9 marker records the number of bytes (bufsize) embedded in the header
//					fprintf(stderr, "  The byteSize is %d\n", GETJOCTET(marker->data[0]));
//					byteSize = GETJOCTET(marker->data[0]);
//					for (x = 0; x < byteSize; x++) {
//						bufsize += ( GETJOCTET(marker->data[x+1]) << 8*(byteSize-1-x) );
//					}
//					fprintf(stderr, "  The bufSize is %ld\n", bufsize);
//					
//					/* Allocate bufArray to that size. */
//					bufArray = malloc(sizeof(char) * (bufsize + 1));
//					
//				} else if (mCounter < ceil( (float)bufsize/65533 )) { // In the following APP9 markers, read each byte to buffer.
//					// mCounter < ceil( (float)bufsize/65533 ) means the current marker is not the last one, so read all 65533 bytes
//					for (y = 0; y < 65533; y++) {
//						bufArray[ 65533 * (mCounter-1) + y ] = GETJOCTET(marker->data[y]);
//					}
//				} else { // The last APP9 marker
//					for (y = 0; y < bufsize%65533; y++) {
//						bufArray[ 65533 * (mCounter-1) + y ] = GETJOCTET(marker->data[y]);
//					}
//				}
//				mCounter ++;
//			}
		
		size_t totalbytes = fwrite(bufArray, 1 , bufsize , subimgFile ); /* Total number of bytes read from APP10 marker */
		fprintf(stderr, "  %ld bytes written in %s.\n", totalbytes, subimageName);
		
		fclose(subimgFile);
		free(bufArray);
		
		
		/* Decoding struct of extracted sub image */
		struct jpeg_decompress_struct srcinfo_2;
		struct jpeg_error_mgr jsrcerr_2;
		jvirt_barray_ptr * src_coef_arrays_2;
		FILE * input_file_2 = fopen(subimageName, READ_BINARY);
		srcinfo_2.err = jpeg_std_error(&jsrcerr_2);
		jpeg_create_decompress(&srcinfo_2);
		jsrcerr_2.trace_level = jdsterr.trace_level;
		srcinfo_2.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;
		
		/* Specify data source for decompression */
		jpeg_stdio_src(&srcinfo_2, input_file_2);
		
		/* Enable saving of extra markers that we want to copy */
		jcopy_markers_setup(&srcinfo_2, JCOPYOPT_NONE);
		
		/* Read file header */
		(void) jpeg_read_header(&srcinfo_2, TRUE);
		
		/* Any space needed by a transform option must be requested before
		 * jpeg_read_coefficients so that memory allocation will be done right.
		 */
#if TRANSFORMS_SUPPORTED
		jtransform_request_workspace(&srcinfo_2, &transformoption);
#endif
		
		/* Read source file as DCT coefficients */
		src_coef_arrays_2 = jpeg_read_coefficients(&srcinfo_2);
		
		/* Process DCT in the following code */
		JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN], buffer_2[MAX_COMPS_IN_SCAN];
		JBLOCKROW MCU_buffer[C_MAX_BLOCKS_IN_MCU];
		JBLOCKROW buffer_ptr, buffer_ptr_2;
		JDIMENSION MCU_col_num;	/* index of current MCU within row */
		JDIMENSION last_MCU_col = srcinfo.MCUs_per_row - 1;
		JDIMENSION last_iMCU_row = srcinfo.total_iMCU_rows - 1;
		jpeg_component_info *compptr, *compptr_2;
		JDIMENSION iMCU_row_num = 0;
		JDIMENSION start_col;
		/* Align the virtual buffers for the components used in this scan. */
		int blkn, ci, xindex, yindex, yoffset, blockcnt;
		int MCU_vert_offset = 0;
		int MCU_rows_per_iMCU_row = 1;
		int mcu_ctr = 0;
		int blkindex, k;
		
		int h_factor = (srcinfo.max_h_samp_factor==1)? 2:1;
		int v_factor = (srcinfo.max_v_samp_factor==1)? 2:1;
		
		JDIMENSION MCU_total_rows = ceil( ( (float) srcinfo.image_height / 8 ) / srcinfo.max_v_samp_factor );
		fprintf(stderr, "  Image width: %d; height: %d\n", srcinfo.image_width, srcinfo.image_height);
		fprintf(stderr, "  Total MCU rows: %d\n", MCU_total_rows);
		fprintf(stderr, "  Pro-image: H sampling %d, V sampling %d\n", srcinfo.max_h_samp_factor, srcinfo.max_v_samp_factor);
		fprintf(stderr, "  Sub-image: H sampling %d, V sampling %d\n", srcinfo_2.max_h_samp_factor, srcinfo_2.max_v_samp_factor);
		fprintf(stderr, "  Colorspace: Pro-image: %d, Sub-image: %d\n", srcinfo.jpeg_color_space, srcinfo_2.jpeg_color_space);
		
		
		int counter_mcu = 0;
		for (iMCU_row_num = 0; iMCU_row_num < MCU_total_rows; iMCU_row_num++) {
			for (ci = 0; ci < srcinfo.comps_in_scan; ci++) {
				compptr = srcinfo.cur_comp_info[ci];
				compptr_2 = srcinfo_2.cur_comp_info[ci];
				buffer[ci] = (srcinfo.mem->access_virt_barray) ((j_common_ptr) &srcinfo, src_coef_arrays[compptr->component_index], iMCU_row_num * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor, FALSE);
				buffer_2[ci] = (srcinfo_2.mem->access_virt_barray) ((j_common_ptr) &srcinfo_2, src_coef_arrays_2[compptr_2->component_index], iMCU_row_num * compptr_2->v_samp_factor, (JDIMENSION) compptr_2->v_samp_factor, FALSE);
			}
			
			for (yoffset = MCU_vert_offset; yoffset < MCU_rows_per_iMCU_row; yoffset++) {
				for (MCU_col_num = mcu_ctr; MCU_col_num < srcinfo.MCUs_per_row; MCU_col_num++) {
					
					/* Current 8 bits */
					int * current_8bits = byte2bits( GETJOCTET( maskArrayMarker->data[2 + bytes_filesize + counter_mcu/8] ) );
					if ( current_8bits[counter_mcu % 8] == 1 ) {
						/* Construct list of pointers to DCT blocks belonging to this MCU */
						blkn = 0;			/* index of current DCT block within MCU */
						for (ci = 0; ci < srcinfo.comps_in_scan; ci++) {
							compptr   = srcinfo.cur_comp_info[ci];
							compptr_2 = srcinfo_2.cur_comp_info[ci];
							start_col = MCU_col_num * compptr->MCU_width;
							blockcnt = (MCU_col_num < last_MCU_col) ? compptr->MCU_width : compptr->last_col_width;
							
							for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
								if (iMCU_row_num < last_iMCU_row || yindex+yoffset < compptr->last_row_height) {
									/* Fill in pointers to real blocks in this row */
									buffer_ptr   = buffer[ci][yindex+yoffset] + start_col;
									buffer_ptr_2 = buffer_2[ci][yindex+yoffset] + start_col;
									
									/****************** Re-Morphing. Added by Lin YUAN (lin.yuan@epfl.ch) EPFL.*********************/
									
									for (blkindex = 0; blkindex < blockcnt; blkindex++) {
										for (k = 0; k < 64; k++) {
											// If scrambleOption is enbled, descramble the DCT values with random number.
											buffer_ptr[blkindex][k] = buffer_ptr_2[blkindex][k] * (scrambleOption ? ((rand()%2)*2-1):1);
										}
									}
								} else {
									/* At bottom of image, need a whole row of dummy blocks */
									xindex = 0;
								}
							}
						}
					}
					counter_mcu ++;
				}
			}
		}
		fclose(input_file_2);
		fprintf(stderr, "  Number of scaned MCU: %d\n\n", counter_mcu);
	}
	
	
	
	
	
	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters(&srcinfo, &dstinfo);
	
	/* Adjust destination parameters if required by transform options;
	 * also find out which set of coefficient arrays will hold the output.
	 */
#if TRANSFORMS_SUPPORTED
	dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
												   src_coef_arrays,
												   &transformoption);
#else
	dst_coef_arrays = src_coef_arrays;
#endif
	
	/* Adjust default compression parameters by re-parsing the options */
	file_index = parse_switches(&dstinfo, argc, argv, 0, TRUE);

	/* Specify data destination for compression */
	jpeg_stdio_dest(&dstinfo, output_file);
	
	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients(&dstinfo, dst_coef_arrays);
	
	/* Copy to the output file any extra markers that we want to preserve */
	jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);
	
	/* Execute image transformation, if any */
#if TRANSFORMS_SUPPORTED
	jtransform_execute_transformation(&srcinfo, &dstinfo,
									  src_coef_arrays,
									  &transformoption);
#endif
	
	/* Finish compression and release memory */
	jpeg_finish_compress(&dstinfo);
	jpeg_destroy_compress(&dstinfo);
	(void) jpeg_finish_decompress(&srcinfo);
	jpeg_destroy_decompress(&srcinfo);
	
	/* Close files, if we opened them */
	if (input_file != stdin)
		fclose(input_file);
	if (output_file != stdout)
		fclose(output_file);
	
#ifdef PROGRESS_REPORT
	end_progress_monitor((j_common_ptr) &dstinfo);
#endif
	
	
	/* Transmorphing mode: write the sub-image into the JPEG marker of the target image. */
	if (transMode == TRANSMORPH) {
		/* Now begin embedding the subimage of original image to the processed image */
		struct jpeg_decompress_struct srcinfo2;
		struct jpeg_compress_struct dstinfo2;
		struct jpeg_error_mgr jsrcerr2, jdsterr2;
#ifdef PROGRESS_REPORT
		struct cdjpeg_progress_mgr progress2;
#endif
		jvirt_barray_ptr * src_coef_arrays2;
		jvirt_barray_ptr * dst_coef_arrays2;
		//	int file_index;
		FILE * input_file2;
		FILE * output_file2;
		
		//		// This time normal JPEG transcoding
		//		dstinfo2.transMode = 0;
		
		/* Initialize the JPEG decompression object with default error handling. */
		srcinfo2.err = jpeg_std_error(&jsrcerr2);
		jpeg_create_decompress(&srcinfo2);
		/* Initialize the JPEG compression object with default error handling. */
		dstinfo2.err = jpeg_std_error(&jdsterr2);
		jpeg_create_compress(&dstinfo2);
		
		/* Open the input file. */
		if (file_index < argc) {
			// procam update -- visible_sub is removed from argument list, so arguments shifted by 1
			//if ((input_file2 = fopen(argv[file_index+2], READ_BINARY)) == NULL) {
			if ((input_file2 = fopen(argv[file_index+1], READ_BINARY)) == NULL) {
				//fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index+2]);
				fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index+1]);
				exit(EXIT_FAILURE);
			}
			// procam update -- visible_sub is removed from argument list, so arguments shifted by 1
			//if ((output_file2 = fopen(argv[file_index+3], WRITE_BINARY)) == NULL) {
			if ((output_file2 = fopen(argv[file_index+2], WRITE_BINARY)) == NULL) {
				//fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index+3]);
				fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index+2]);
				exit(EXIT_FAILURE);
			}
		} else {
			fprintf(stderr, "Can't open input files\n");
			exit(EXIT_FAILURE);
			//			/* default input file is stdin */
			//			input_file2 = read_stdin();
			//			/* default output file is stdout */
			//			output_file2 = write_stdout();
		}
		
#ifdef PROGRESS_REPORT
		start_progress_monitor((j_common_ptr) &dstinfo2, &progress2);
#endif
		
		/* Specify data source for decompression */
		jpeg_stdio_src(&srcinfo2, input_file2);
		
		/* Enable saving of extra markers that we want to copy */
		jcopy_markers_setup(&srcinfo2, JCOPYOPT_ALL);
		
		/* Read file header */
		(void) jpeg_read_header(&srcinfo2, TRUE);
		
		/* Any space needed by a transform option must be requested before
		 * jpeg_read_coefficients so that memory allocation will be done right.
		 */
#if TRANSFORMS_SUPPORTED
		jtransform_request_workspace(&srcinfo2, &transformoption);
#endif
		
		/* Read source file as DCT coefficients */
		src_coef_arrays2 = jpeg_read_coefficients(&srcinfo2);
		
		/* Initialize destination compression parameters from source values */
		jpeg_copy_critical_parameters(&srcinfo2, &dstinfo2);
		
		/* Adjust destination parameters if required by transform options;
		 * also find out which set of coefficient arrays will hold the output.
		 */
#if TRANSFORMS_SUPPORTED
		dst_coef_arrays2 = jtransform_adjust_parameters(&srcinfo2, &dstinfo2,
														src_coef_arrays2,
														&transformoption);
#else
		dst_coef_arrays2 = src_coef_arrays2;
#endif
		
		//	/* Adjust default compression parameters by re-parsing the options */
		//	file_index = parse_switches(&dstinfo, argc, argv, 0, TRUE);
		
		/* Specify data destination for compression */
		jpeg_stdio_dest(&dstinfo2, output_file2);
		
		/* Start compressor (note no image data is actually written here) */
		jpeg_write_coefficients(&dstinfo2, dst_coef_arrays2);
		
		/* Copy to the output file any extra markers that we want to preserve */
		jcopy_markers_execute(&srcinfo2, &dstinfo2, JCOPYOPT_ALL);
		
		/* Write the mask array elements to APP11 marker*/
		/* Number of bytes needed to record the mask array */
		int bytes_mask_array = ceil(mcu_counter/8.0);
		
		/* The subimage to be writted in markers */
		// procam update
		//FILE *fileToWrite = fopen(argv[file_index + 1], "r");
		FILE *fileToWrite = fopen(static_output_filename, "r");
		/* File size of the subimage (in bytes) */
		long filesize;
		if (fileToWrite != NULL) {
			/* Go to the end of the file. */
			if (fseek(fileToWrite, 0L, SEEK_END) == 0) {
				filesize = ftell(fileToWrite);
				fprintf(stderr, "  File size of the sub-image: %li\n", filesize);
			}
		}
		/* number of bytes need to record the value of the file size. As the file size can be large, one byte (256) is not enough. */
		unsigned int bytes_filesize = sizeof(filesize);
		
		/* Initialized the APP11 marker */
		jpeg_write_m_header(&dstinfo2, JPEG_APP0 + 11, 1 + 1 + bytes_filesize + bytes_mask_array);
		fprintf(stderr, "  Number of bytes to write in APP11: 1 + 1 + %d + %d\n", bytes_filesize, bytes_mask_array);
		jpeg_write_m_byte(&dstinfo2, TRANSMORPH);       /* 1st byte - protection method: 3 - TRANSMORPH */
		jpeg_write_m_byte(&dstinfo2, bytes_filesize);   /* 2nd byte - bytes_filesize */
		/* Record the filesize in the following <bytes_filesize> bytes */
		int x;
		for (x = 0; x < bytes_filesize; x++) {
			jpeg_write_m_byte(&dstinfo2, (int)((filesize >> 8*(bytes_filesize-1-x)) & 0xFF));
		}
		
		/* Record the bytes signaled to the mask matrix */
		int mask_array_byte_index;
		for (mask_array_byte_index = 0; mask_array_byte_index < bytes_mask_array; mask_array_byte_index++) {
			jpeg_write_m_byte(&dstinfo2, bits2byte( maskArray + mask_array_byte_index * 8 ));
		}
		
		/* Write the subimage (by byte) into the next JPEG APP11 marker(s). Added by Lin YUAN (lin.yuan@epfl.ch). */
		// procam update
		//writeFileToMarker( &dstinfo2, argv[file_index + 1], 11, filesize );
		writeFileToMarker( &dstinfo2, static_output_filename, 11, filesize );
		
		/* Execute image transformation, if any */
#if TRANSFORMS_SUPPORTED
		jtransform_execute_transformation(&srcinfo2, &dstinfo2,
										  src_coef_arrays2,
										  &transformoption);
#endif
		
		/* Finish compression and release memory */
		jpeg_finish_compress(&dstinfo2);
		jpeg_destroy_compress(&dstinfo2);
		(void) jpeg_finish_decompress(&srcinfo2);
		jpeg_destroy_decompress(&srcinfo2);
		
		/* Close files, if we opened them */
		if (input_file2 != stdin)
			fclose(input_file2);
		if (output_file2 != stdout)
			fclose(output_file2);

		
#ifdef PROGRESS_REPORT
		end_progress_monitor((j_common_ptr) &dstinfo2);
#endif
	}
	
	// procam update
	// delete the intermediate file
	if( remove( static_output_filename ) != 0 )
    		perror( "DEBUG: Error deleting the intermediate file" );
  	//else
  	//	puts( "DEBUG: Intermedaite file successfully deleted" );
	/* All done. */

	exit(jsrcerr.num_warnings + jdsterr.num_warnings ?EXIT_WARNING:EXIT_SUCCESS);
	return 0;			/* suppress no-return-value warnings */
}
