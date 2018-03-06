#include "pub.h"
int processing_start(png_structp & png_ptr, png_infop & info_ptr, void * frame_ptr, bool hasInfo, CHUNK & chunkIHDR, std::vector<CHUNK>& chunksInfo)
{
	unsigned char header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	if (!png_ptr || !info_ptr)
		return 1;

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return 1;
	}

	png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
	png_set_progressive_read_fn(png_ptr, frame_ptr, info_fn, row_fn, NULL);

	png_process_data(png_ptr, info_ptr, header, 8);
	png_process_data(png_ptr, info_ptr, chunkIHDR.p, chunkIHDR.size);

	if (hasInfo)
		for (unsigned int i = 0; i<chunksInfo.size(); i++)
			png_process_data(png_ptr, info_ptr, chunksInfo[i].p, chunksInfo[i].size);
	return 0;
}

int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char * p, unsigned int size)
{
	if (!png_ptr || !info_ptr)
		return 1;

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return 1;
	}

	png_process_data(png_ptr, info_ptr, p, size);
	return 0;
}

int processing_finish(png_structp png_ptr, png_infop info_ptr)
{
	unsigned char footer[12] = { 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130 };

	if (!png_ptr || !info_ptr)
		return 1;

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return 1;
	}

	png_process_data(png_ptr, info_ptr, footer, 12);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return 0;
}

void info_fn(png_structp png_ptr, png_infop info_ptr)
{
	png_set_expand(png_ptr);
	png_set_strip_16(png_ptr);
	png_set_gray_to_rgb(png_ptr);
	png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
	(void)png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
}

void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
	APNGFrame * frame = (APNGFrame *)png_get_progressive_ptr(png_ptr);
	png_progressive_combine_row(png_ptr, frame->rows[row_num], new_row);
}

void compose_frame(unsigned char ** rows_dst, unsigned char ** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	unsigned int  i, j;
	int u, v, al;

	for (j = 0; j<h; j++)
	{
		unsigned char * sp = rows_src[j];
		unsigned char * dp = rows_dst[j + y] + x * 4;

		if (bop == 0)
			memcpy(dp, sp, w * 4);
		else
			for (i = 0; i<w; i++, sp += 4, dp += 4)
			{
				if (sp[3] == 255)
					memcpy(dp, sp, 4);
				else
					if (sp[3] != 0)
					{
						if (dp[3] != 0)
						{
							u = sp[3] * 255;
							v = (255 - sp[3])*dp[3];
							al = u + v;
							dp[0] = (sp[0] * u + dp[0] * v) / al;
							dp[1] = (sp[1] * u + dp[1] * v) / al;
							dp[2] = (sp[2] * u + dp[2] * v) / al;
							dp[3] = al / 255;
						}
						else
							memcpy(dp, sp, 4);
					}
			}
	}
}

int cmp_colors(const void *arg1, const void *arg2)
{
	if (((COLORS*)arg1)->a != ((COLORS*)arg2)->a)
		return (int)(((COLORS*)arg1)->a) - (int)(((COLORS*)arg2)->a);

	if (((COLORS*)arg1)->num != ((COLORS*)arg2)->num)
		return (int)(((COLORS*)arg2)->num) - (int)(((COLORS*)arg1)->num);

	if (((COLORS*)arg1)->r != ((COLORS*)arg2)->r)
		return (int)(((COLORS*)arg1)->r) - (int)(((COLORS*)arg2)->r);

	if (((COLORS*)arg1)->g != ((COLORS*)arg2)->g)
		return (int)(((COLORS*)arg1)->g) - (int)(((COLORS*)arg2)->g);

	return (int)(((COLORS*)arg1)->b) - (int)(((COLORS*)arg2)->b);
}
