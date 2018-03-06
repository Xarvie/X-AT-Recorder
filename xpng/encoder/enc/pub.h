#ifndef PUB_H
#define PUB_H
#include "png.h"
#include "zlib.h"
#include <vector>

#define notabc(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))

#define id_IHDR 0x52444849
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549
typedef unsigned char * ROW;
typedef struct { unsigned char *p; unsigned int size; int x, y, w, h, valid; } OP3;
struct OP { unsigned char * p; unsigned int size; int x, y, w, h, valid, filters; };
struct rgb { unsigned char r, g, b; };
struct CHUNK { unsigned char * p; unsigned int size; };
struct COLORS{ unsigned int num; unsigned char r, g, b, a; } ;
int processing_start(png_structp & png_ptr, png_infop & info_ptr, void * frame_ptr, bool hasInfo, CHUNK & chunkIHDR, std::vector<CHUNK>& chunksInfo);
int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char * p, unsigned int size);
int processing_finish(png_structp png_ptr, png_infop info_ptr);

struct APNGFrame { unsigned char * p, ** rows; unsigned int w, h, delay_num, delay_den; };
void info_fn(png_structp png_ptr, png_infop info_ptr);
void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass);

void compose_frame(unsigned char ** rows_dst, unsigned char ** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
int cmp_colors(const void *arg1, const void *arg2);

#endif