#ifndef _H_RGBE
#define _H_RGBE
/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

/* utility for reading and writing Ward's rgbe image format.
   See rgbe.txt file for more details.
*/

#include <stdio.h>
#include <string>

typedef struct {
  int valid;            /* indicate which fields are valid */
  char programtype[16]; /* listed at beginning of file to identify it 
                         * after "#?".  defaults to "RGBE" */ 
  float gamma;          /* image has already been gamma corrected with 
                         * given gamma.  defaults to 1.0 (no correction) */
  float exposure;       /* a value of 1.0 in an image corresponds to
			 * <exposure> watts/steradian/m^2. 
			 * defaults to 1.0 */
} rgbe_header_info;

/* flags indicating which fields in an rgbe_header_info are valid */
#define RGBE_VALID_PROGRAMTYPE 0x01
#define RGBE_VALID_GAMMA       0x02
#define RGBE_VALID_EXPOSURE    0x04

/* return codes for rgbe routines */
#define RGBE_RETURN_SUCCESS 0
#define RGBE_RETURN_FAILURE -1


#include <math.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and 
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)
 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/

#ifdef _CPLUSPLUS
/* define if your compiler understands inline commands */
#define INLINE inline
#else
#define INLINE
#endif

/* offsets to red, green, and blue components in a data (float) pixel */
#define RGBE_DATA_RED    0
#define RGBE_DATA_GREEN  1
#define RGBE_DATA_BLUE   2
/* number of floats per pixel */
#define RGBE_DATA_SIZE   3

enum rgbe_error_codes {
  rgbe_read_error,
  rgbe_write_error,
  rgbe_format_error,
  rgbe_memory_error,
};

/* default error routine.  change this to change error handling */
int rgbe_error(int rgbe_error_code, const char *msg);

/* standard conversion from float pixels to rgbe pixels */
/* note: you can remove the "inline"s if your compiler complains about it */
INLINE void 
float2rgbe(unsigned char rgbe[4], float red, float green, float blue);

/* standard conversion from rgbe to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */
INLINE void 
rgbe2float(float *red, float *green, float *blue, unsigned char rgbe[4]);

/* default minimal header. modify if you want more information in header */
int RGBE_WriteHeader(FILE *fp, int width, int height, rgbe_header_info *info);

/* minimal header reading.  modify if you want to parse more information */
int RGBE_ReadHeader(FILE *fp, int *width, int *height, rgbe_header_info *info);

/* simple write routine that does not use run length encoding */
/* These routines can be made faster by allocating a larger buffer and
   fread-ing and fwrite-ing the data in larger chunks */
int RGBE_WritePixels(FILE *fp, float *data, int numpixels);

/* simple read routine.  will not correctly handle run length encoding */
int RGBE_ReadPixels(FILE *fp, float *data, int numpixels);

/* The code below is only needed for the run-length encoded files. */
/* Run length encoding adds considerable complexity but does */
/* save some space.  For each scanline, each channel (r,g,b,e) is */
/* encoded separately for better compression. */
int RGBE_WriteBytes_RLE(FILE *fp, unsigned char *data, int numbytes);

int RGBE_WritePixels_RLE(FILE *fp, float *data, int scanline_width,
			 int num_scanlines);
      
int RGBE_ReadPixels_RLE(FILE *fp, float *data, int scanline_width,
			int num_scanlines);

#endif /* _H_RGBE */



