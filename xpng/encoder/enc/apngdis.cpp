#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "png.h"
#include "zlib.h"
#include "pub.h"
#define notabc(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))

#define id_IHDR 0x52444849
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549

const unsigned long cMaxPNGSize = 1000000UL;

inline unsigned int read_chunk(FILE * f, CHUNK * pChunk)
{
  unsigned char len[4];
  pChunk->size = 0;
  pChunk->p = 0;
  if (fread(&len, 4, 1, f) == 1)
  {
    pChunk->size = png_get_uint_32(len) + 12;
    pChunk->p = new unsigned char[pChunk->size];
    memcpy(pChunk->p, len, 4);
    if (fread(pChunk->p + 4, pChunk->size - 4, 1, f) == 1)
      return *(unsigned int *)(pChunk->p + 4);
  }
  return 0;
}

int load_apng(char * szIn, std::vector<APNGFrame>& frames)
{
  FILE * f;
  unsigned int id, i, j, w, h, w0, h0, x0, y0;
  unsigned int delay_num, delay_den, dop, bop, rowbytes, imagesize;
  unsigned char sig[8];
  png_structp png_ptr;
  png_infop info_ptr;
  CHUNK chunk;
  CHUNK chunkIHDR;
  std::vector<CHUNK> chunksInfo;
  bool isAnimated = false;
  bool skipFirst = false;
  bool hasInfo = false;
  APNGFrame frameRaw = {0};
  APNGFrame frameCur = {0};
  APNGFrame frameNext = {0};
  int res = -1;

  printf("Reading '%s'...\n", szIn);

  if ((f = fopen(szIn, "rb")) != 0)
  {
    if (fread(sig, 1, 8, f) == 8 && png_sig_cmp(sig, 0, 8) == 0)
    {
      id = read_chunk(f, &chunkIHDR);

      if (id == id_IHDR && chunkIHDR.size == 25)
      {
        w0 = w = png_get_uint_32(chunkIHDR.p + 8);
        h0 = h = png_get_uint_32(chunkIHDR.p + 12);

        if (w > cMaxPNGSize || h > cMaxPNGSize)
        {
          fclose(f);
          return res;
        }

        x0 = 0;
        y0 = 0;
        delay_num = 1;
        delay_den = 10;
        dop = 0;
        bop = 0;
        rowbytes = w * 4;
        imagesize = h * rowbytes;

        frameRaw.p = new unsigned char[imagesize];
        frameRaw.rows = new png_bytep[h * sizeof(png_bytep)];
        for (j=0; j<h; j++)
          frameRaw.rows[j] = frameRaw.p + j * rowbytes;

        if (!processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
        {
          frameCur.w = w;
          frameCur.h = h;
          frameCur.p = new unsigned char[imagesize];
          frameCur.rows = new png_bytep[h * sizeof(png_bytep)];
          for (j=0; j<h; j++)
            frameCur.rows[j] = frameCur.p + j * rowbytes;

          while ( !feof(f) )
          {
            id = read_chunk(f, &chunk);
            if (!id)
              break;

            if (id == id_acTL && !hasInfo && !isAnimated)
            {
              isAnimated = true;
              skipFirst = true;
            }
            else
            if (id == id_fcTL && (!hasInfo || isAnimated))
            {
              if (hasInfo)
              {
                if (!processing_finish(png_ptr, info_ptr))
                {
                  frameNext.p = new unsigned char[imagesize];
                  frameNext.rows = new png_bytep[h * sizeof(png_bytep)];
                  for (j=0; j<h; j++)
                    frameNext.rows[j] = frameNext.p + j * rowbytes;

                  if (dop == 2)
                    memcpy(frameNext.p, frameCur.p, imagesize);

                  compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                  frameCur.delay_num = delay_num;
                  frameCur.delay_den = delay_den;

                  frames.push_back(frameCur);

                  if (dop != 2)
                  {
                    memcpy(frameNext.p, frameCur.p, imagesize);
                    if (dop == 1)
                      for (j=0; j<h0; j++)
                        memset(frameNext.rows[y0 + j] + x0*4, 0, w0*4);
                  }
                  frameCur.p = frameNext.p;
                  frameCur.rows = frameNext.rows;
                }
                else
                {
                  delete[] frameCur.rows;
                  delete[] frameCur.p;
                  delete[] chunk.p;
                  break;
                }
              }

              // At this point the old frame is done. Let's start a new one.
              w0 = png_get_uint_32(chunk.p + 12);
              h0 = png_get_uint_32(chunk.p + 16);
              x0 = png_get_uint_32(chunk.p + 20);
              y0 = png_get_uint_32(chunk.p + 24);
              delay_num = png_get_uint_16(chunk.p + 28);
              delay_den = png_get_uint_16(chunk.p + 30);
              dop = chunk.p[32];
              bop = chunk.p[33];

              if (w0 > cMaxPNGSize || h0 > cMaxPNGSize || x0 > cMaxPNGSize || y0 > cMaxPNGSize
                  || x0 + w0 > w || y0 + h0 > h || dop > 2 || bop > 1)
              {
                delete[] frameCur.rows;
                delete[] frameCur.p;
                delete[] chunk.p;
                break;
              }

              if (hasInfo)
              {
                memcpy(chunkIHDR.p + 8, chunk.p + 12, 8);
                if (processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
                {
                  delete[] frameCur.rows;
                  delete[] frameCur.p;
                  delete[] chunk.p;
                  break;
                }
              }
              else
                skipFirst = false;

              if (frames.size() == (skipFirst ? 1 : 0))
              {
                bop = 0;
                if (dop == 2)
                  dop = 1;
              }
            }
            else
            if (id == id_IDAT)
            {
              hasInfo = true;
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                delete[] frameCur.rows;
                delete[] frameCur.p;
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_fdAT && isAnimated)
            {
              png_save_uint_32(chunk.p + 4, chunk.size - 16);
              memcpy(chunk.p + 8, "IDAT", 4);
              if (processing_data(png_ptr, info_ptr, chunk.p + 4, chunk.size - 4))
              {
                delete[] frameCur.rows;
                delete[] frameCur.p;
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_IEND)
            {
              if (hasInfo && !processing_finish(png_ptr, info_ptr))
              {
                compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                frameCur.delay_num = delay_num;
                frameCur.delay_den = delay_den;
                frames.push_back(frameCur);
              }
              else
              {
                delete[] frameCur.rows;
                delete[] frameCur.p;
              }
              delete[] chunk.p;
              break;
            }
            else
            if (notabc(chunk.p[4]) || notabc(chunk.p[5]) || notabc(chunk.p[6]) || notabc(chunk.p[7]))
            {
              delete[] chunk.p;
              break;
            }
            else
            if (!hasInfo)
            {
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                delete[] frameCur.rows;
                delete[] frameCur.p;
                delete[] chunk.p;
                break;
              }
              chunksInfo.push_back(chunk);
              continue;
            }
            delete[] chunk.p;
          }
        }
        delete[] frameRaw.rows;
        delete[] frameRaw.p;

        if (!frames.empty())
          res = (skipFirst) ? 0 : 1;
      }

      for (i=0; i<chunksInfo.size(); i++)
        delete[] chunksInfo[i].p;

      chunksInfo.clear();
      delete[] chunkIHDR.p;
    }
    fclose(f);
  }

  return res;
}

void save_png(char * szOut, APNGFrame * frame)
{
  FILE * f;
  png_structp  png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop    info_ptr = png_create_info_struct(png_ptr);

  if (!png_ptr || !info_ptr)
    return;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return;
  }

  if ((f = fopen(szOut, "wb")) != 0)
  {
    png_init_io(png_ptr, f);
    png_set_compression_level(png_ptr, 9);
    png_set_IHDR(png_ptr, info_ptr, frame->w, frame->h, 8, 6, 0, 0, 0);
    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, frame->rows);
    png_write_end(png_ptr, info_ptr);
    fclose(f);
  }
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

void save_txt(char * szOut, APNGFrame * frame)
{
  FILE * f;
  if ((f = fopen(szOut, "wb")) != 0)
  {
    fprintf(f, "delay=%d/%d\n", frame->delay_num, frame->delay_den);
    fclose(f);
  }
}

int apngdis_main(int argc, char** argv)
{
  unsigned int i, j;
  char * szInput;
  char * szOutPrefix;
  char   szPath[256];
  char   szOut[256];
  std::vector<APNGFrame> frames;

  printf("\nAPNG Disassembler 2.8\n\n");

  if (argc > 1)
    szInput = argv[1];
  else
  {
    printf("Usage: apngdis anim.png [name]\n");
    return 1;
  }

  strcpy(szPath, szInput);
  for (i=j=0; szPath[i]!=0; i++)
  {
    if (szPath[i] == '\\' || szPath[i] == '/' || szPath[i] == ':')
      j = i+1;
  }
  szPath[j] = 0;

  if (argc > 2)
  {
    szOutPrefix = argv[2];

    for (i=j=0; szOutPrefix[i]!=0; i++)
    {
      if (szOutPrefix[i] == '\\' || szOutPrefix[i] == '/' || szOutPrefix[i] == ':')
        j = i+1;
      if (szOutPrefix[i] == '.')
        szOutPrefix[i] = 0;
    }
    strcat(szPath, szOutPrefix+j);
  }
  else
    strcat(szPath, "apngframe");

  int res = load_apng(szInput, frames);
  if (res < 0)
  {
    printf("load_apng() failed: '%s'\n", szInput);
    return 1;
  }

  unsigned int num_frames = frames.size();
  unsigned int len = sprintf(szOut, "%d", num_frames);

  for (i=0; i<num_frames; i++)
  {
    printf("extracting frame %d of %d\n", i+1, num_frames);

    sprintf(szOut, "%s%.*d.png", szPath, len, i+res);
    save_png(szOut, &frames[i]);

    sprintf(szOut, "%s%.*d.txt", szPath, len, i+res);
    save_txt(szOut, &frames[i]);

    delete[] frames[i].rows;
    delete[] frames[i].p;
  }
  frames.clear();

  printf("all done\n");

  return 0;
}
