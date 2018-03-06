#ifndef IMAGE_H
#define IMAGE_H
#include <string.h>
#include <vector>
#include "pub.h"
struct Image
{

  unsigned int w, h, bpp, type;
  int ps, ts;
  rgb pl[256];
  unsigned char tr[256];
  unsigned int delay_num, delay_den;
  unsigned char * p;
  ROW * rows;
  Image() : w(0), h(0), bpp(0), type(0), ps(0), ts(0), delay_num(1), delay_den(10), p(0), rows(0)
  {
    memset(pl, 255, sizeof(pl));
    memset(tr, 255, sizeof(tr));
  }
  ~Image() { }
  void init(unsigned int w1, unsigned int h1, unsigned int bpp1, unsigned int type1)
  {
    w = w1; h = h1; bpp = bpp1; type = type1;
    int rowbytes = w * bpp;
    delete[] rows; delete[] p;
    rows = new ROW[h];
    rows[0] = p = new unsigned char[h * rowbytes];
    for (unsigned int j=1; j<h; j++)
      rows[j] = rows[j-1] + rowbytes;
  }
  void init(unsigned int w, unsigned int h, Image * image)
  { 
    init(w, h, image->bpp, image->type);
    if ((ps = image->ps) != 0) memcpy(&pl[0], &image->pl[0], ps*3);
    if ((ts = image->ts) != 0) memcpy(&tr[0], &image->tr[0], ts);
  }
  void init(Image * image) { init(image->w, image->h, image); }
  void free() { delete[] rows; delete[] p; }
};

int load_image(char * szName, Image * image);
unsigned char find_common_coltype(std::vector<Image>& img);
void optim_upconvert(Image * image, unsigned char coltype);
void optim_duplicates(std::vector<Image>& img, unsigned int first);
void optim_dirty_transp(Image * image);
void optim_downconvert(std::vector<Image>& img);
void optim_palette(std::vector<Image>& img);
void optim_add_transp(std::vector<Image>& img);

#endif /* IMAGE_H */
