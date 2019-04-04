/*
 * Ooura FFT library interface
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "fft.h"

#define N(x)            (sizeof(x)/sizeof(*x))
#define IS_POW2(n)      (!((n) & ((n) - 1)))
#define ALLOC(t)        ((t*)malloc(sizeof(t)))
#define NALLOC(t,n)     ((t*)malloc(sizeof(t) * (n)))
#define MAX(m,n)        (((m) > (n))? (m): (n))

#define ERR             __LINE__

#define FMT_U8          0x0001
#define FMT_U16LE       0x0002
#define FMT_U16BE       0x0012
#define FMT_S16LE       0x0102
#define FMT_S16BE       0x0112
#define FMT_S24LE       0x0203
#define FMT_S24BE       0x0213

typedef struct {
  int pos;
  int n;
} lc_t;

extern void rdft(int, int, double *, int *, double *);

int
fft_new(char* _fmt, int capa, fft_t** _obj)
{
  int ret;
  fft_t* obj;

  int fmt;

  double* data;
  double* wtbl;
  lc_t* line;

  double* a;
  int* ip;
  double* w;

  /*
   * initialize
   */
  ret  = 0;

  obj  = NULL;
  data = NULL;
  wtbl = NULL;
  a    = NULL;
  ip   = NULL;
  w    = NULL;

  /*
   * argument check
   */
  if (_fmt == NULL) ret = ERR;
  if (!capa || !IS_POW2(capa)) ret = ERR;
  if (_obj == NULL) ret = ERR;

  if (strcasecmp("u8", _fmt) == 0) {
    fmt = FMT_U8;

  } else if (strcasecmp("u16be", _fmt) == 0) {
    fmt = FMT_U16BE;

  } else if (strcasecmp("u16le", _fmt) == 0) {
    fmt = FMT_U16LE;

  } else if (strcasecmp("s16be", _fmt) == 0) {
    fmt = FMT_S16BE;

  } else if (strcasecmp("s16le", _fmt) == 0) {
    fmt = FMT_S16LE;

  } else if (strcasecmp("s24be", _fmt) == 0) {
    fmt = FMT_S24BE;

  } else if (strcasecmp("s24le", _fmt) == 0) {
    fmt = FMT_S24LE;

  } else {
    ret = ERR;
  }

  /*
   * memory allocation
   */
  if (!ret) {
    obj = ALLOC(fft_t);
    if (obj == NULL) ret = ERR;
  }

  if (!ret) {
    data = NALLOC(double, capa);
    if (data == NULL) ret = ERR;
  }

  if (!ret) {
    wtbl = NALLOC(double, capa);
    if (wtbl == NULL) ret = ERR;
  }

  if (!ret) {
    line = NALLOC(lc_t, capa / 2);
    if (line == NULL) ret = ERR;
  }

  if (!ret) {
    a = NALLOC(double, capa);
    if (a == NULL) ret = ERR;
  }

  if (!ret) {
    ip = NALLOC(int, 2 + (int)sqrt(capa / 2));
    if (ip == NULL) ret = ERR;
  }

  if (!ret) {
    w = NALLOC(double, capa / 2);
    if (w == NULL) ret = ERR;
  }

  /*
   * set return parameter
   */
  if (!ret) {
    ip[0] = 0;
    rdft(capa, 1, a, ip, w);

    memset(data, 0, sizeof(double) * capa);

    obj->data     = data;
    obj->wtbl     = wtbl;
              
    obj->line     = line;
    obj->width    = capa / 2;
                 
    obj->fmt      = fmt;

    obj->capa     = capa;
    obj->used     = 0;

    obj->magnify  = 1;

    obj->mode     = FFT_LOGSCALE_MODE;
    obj->fq_s     = 44100;
    obj->fq_h     = 16000;
    obj->fq_l     = 100;
              
    obj->a        = a;
    obj->ip       = ip;
    obj->w        = w;

    fft_set_window(obj, FFT_WINDOW_BLACKMAN);
    fft_set_width(obj, 480);

    *_obj        = obj;
  }

  /*
   * post process
   */
  if (ret) {
    if (obj != NULL) free(obj);
    if (data != NULL) free(data);
    if (wtbl != NULL) free(wtbl);
    if (line != NULL) free(line);
    if (a != NULL) free(a);
    if (ip != NULL) free(ip);
    if (w != NULL) free(w);
  }

  return ret;
}

int
fft_destroy(fft_t* fft)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (fft == NULL) ret = ERR;

  /*
   * release memory
   */
  if (!ret) {
    free(fft->data);
    free(fft->wtbl);
    free(fft->line);
    free(fft->a);
    free(fft->ip);
    free(fft->w);

    free(fft);
  }

  return ret;
}

static void
import_u8(double* dst, uint8_t* src, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    dst[i] = ((double)src[i] - 128.0) / 128.0;
  }
}

static void
import_u16le(double* dst, uint8_t* src, int n)
{
  int i;
  uint16_t smpl;

  for (i = 0; i < n; i++, src += 2) {
    smpl   = ((((uint16_t)src[0] << 0) & 0x00ff)|
              (((uint16_t)src[1] << 8) & 0xff00));
    
    dst[i] = ((double)smpl - 32768.0) / 32768.0;
  }
}

static void
import_u16be(double* dst, uint8_t* src, int n)
{
  int i;
  uint16_t smpl;

  for (i = 0; i < n; i++, src += 2) {
    smpl   = ((((uint16_t)src[1] << 0) & 0x00ff)|
              (((uint16_t)src[0] << 8) & 0xff00));

    dst[i] = ((double)smpl - 32768.0) / 32768.0;
  }
}

static void
import_s16le(double* dst, uint8_t* src, int n)
{
  int i;
  int16_t smpl;

  for (i = 0; i < n; i++, src += 2) {
    smpl   = ((((int16_t)src[0] << 0) & 0x00ff)|
              (((int16_t)src[1] << 8) & 0xff00));
    
    dst[i] = (double)smpl  / 32768.0;
  }
}

static void
import_s16be(double* dst, uint8_t* src, int n)
{
  int i;
  int16_t smpl;

  for (i = 0; i < n; i++, src += 2) {
    smpl   = ((((int16_t)src[1] << 0) & 0x00ff)|
              (((int16_t)src[0] << 8) & 0xff00));
    
    dst[i] = (double)smpl  / 32768.0;
  }
}

static void
import_s24le(double* dst, uint8_t* src, int n)
{
  int i;
  int32_t smpl;

  for (i = 0; i < n; i++, src += 3) {
    smpl   = ((((int32_t)src[0] <<  8) & 0x0000ff00)|
              (((int32_t)src[1] << 16) & 0x00ff0000)|
              (((int32_t)src[2] << 24) & 0xff000000));
    
    dst[i] = (double)smpl  / 2147483648.0;
  }
}

static void
import_s24be(double* dst, uint8_t* src, int n)
{
  int i;
  int32_t smpl;

  for (i = 0; i < n; i++, src += 3) {
    smpl   = ((((int32_t)src[2] <<  8) & 0x0000ff00)|
              (((int32_t)src[1] << 16) & 0x00ff0000)|
              (((int32_t)src[0] << 24) & 0xff000000));
    
    dst[i] = (double)smpl  / 2147483648.0;
  }
}

int
fft_shift_in(fft_t* fft, void* src, int n)
{
  int ret;
  double* dst;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (fft == NULL) ret = ERR;
  if (src == NULL) ret = ERR;
  if (n < 0) ret = ERR;

  if (!ret) {
    if (n > fft->capa) ret = ERR;
  }

  /*
   * do import
   */
  if (!ret) {
    memmove(fft->data, fft->data + n, sizeof(double) * (fft->capa - n));

    dst = fft->data + (fft->capa - n);
    switch (fft->fmt) {
    case FMT_U8:
      import_u8(dst, src, n);
      break;

    case FMT_U16BE:
      import_u16be(dst, src, n);
      break;

    case FMT_U16LE:
      import_u16le(dst, src, n);
      break;

    case FMT_S16BE:
      import_s16be(dst, src, n);
      break;

    case FMT_S16LE:
      import_s16le(dst, src, n);
      break;

    case FMT_S24BE:
      import_s24be(dst, src, n);
      break;

    case FMT_S24LE:
      import_s24le(dst, src, n);
      break;
    }

    fft->used += n;

    if (fft->used > fft->capa) {
      fft->used = fft->capa;
    }
  }

  return ret;
}

int
fft_reset(fft_t* fft)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (fft == NULL) ret = ERR;

  /*
   * do reset buffer
   */
  if (!ret) {
    memset(fft->data, 0, sizeof(double) * fft->capa);

    fft->used = 0;
  }

  return ret;
}


static void
set_rectangular_window(double* dst, int n)
{
  int i;

  for (i = 0; i <= n; i++) {
    dst[i] = 1.0;
  }
}

static void
set_hamming_window(double* dst, int n)
{
  int i;
  double x;

  n--;

  for (i = 0; i <= n; i++) {
    x      = ((double)i / (double)n) * M_PI * 2.0;
    dst[i] = 0.54 - (0.46 * cos(x));
  }
}

static void
set_hann_window(double* dst, int n)
{
  int i;
  double x;

  n--;

  for (i = 0; i <= n; i++) {
    x      = ((double)i / (double)n) * M_PI * 2.0;
    dst[i] = 0.50 - (0.50 * cos(x));
  }
}

static void
set_blackman_window(double* dst, int n)
{
  int i;
  double x;

  n--;

  for (i = 0; i <= n; i++) {
    x      = ((double)i / (double)n) * M_PI;
    dst[i] = 0.42 - (0.50 * cos(2.0 * x)) + (0.08 * cos(4.0 * x));
  }
}

static void
set_blackman_nuttall_window(double* dst, int n)
{
  int i;
  double x;

  n--;

  for (i = 0; i <= n; i++) {
    x      = ((double)i / (double)n) * M_PI * 2.0;
    dst[i] = 0.3635819 - (0.4891775 * cos(x)) +
                (0.1365995 * cos(2.0 * x)) - (0.0106411 * cos(3.0 * x));
  }
}

static void
set_flat_top_window(double* dst, int n)
{
  int i;
  double x;

  n--;

  for (i = 0; i <= n; i++) {
    x      = ((double)i / (double)n) * M_PI * 2.0;
    dst[i] = 1.0 - (1.93 * cos(x)) + (1.29 * cos(2.0 * x)) -
                (0.388 * cos(3.0 * x)) + (0.032 * cos(4.0 * x));
  }
}

int
fft_set_window(fft_t* fft, int type)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument chack
   */
  if (fft == NULL) ret = ERR;

  /*
   * set window function
   */
  switch (type) {
  case FFT_WINDOW_RECTANGULAR:
    set_rectangular_window(fft->wtbl, fft->capa);
    break;

  case FFT_WINDOW_HAMMING:
    set_hamming_window(fft->wtbl, fft->capa);
    break;

  case FFT_WINDOW_HANN:
    set_hann_window(fft->wtbl, fft->capa);
    break;

  case FFT_WINDOW_BLACKMAN:
    set_blackman_window(fft->wtbl, fft->capa);
    break;

  case FFT_WINDOW_BLACKMAN_NUTTALL:
    set_blackman_nuttall_window(fft->wtbl, fft->capa);
    break;

  case FFT_WINDOW_FLAT_TOP:
    set_flat_top_window(fft->wtbl, fft->capa);
    break;

  default:
    ret = ERR;
    break;
  }

  return ret;
}

static void
set_linear_mapping(fft_t* fft)
{
  double pos;
  double step;
  int head;
  int tail;
  lc_t* lc;
  int i;

  pos  = fft->capa * (fft->fq_l / fft->fq_s);
  step = (fft->capa * ((fft->fq_h - fft->fq_l) / fft->fq_s)) / fft->width;
  head = (int)round(pos);
  lc   = (lc_t*)fft->line;

  for (i = 0; i < fft->width; i++) {
    pos += step;
    tail = (int)round(pos);

    lc->pos = head;
    lc->n   = (tail == head)? 1: (tail - head);

    head = tail;
    lc++;
  }
}

static void
set_log_mapping(fft_t* fft)
{
  double pos;
  double step;
  int i;
  int head;
  int tail;
  lc_t* lc;

  pos  = fft->capa * (fft->fq_l / fft->fq_s);
  step = pow((fft->fq_h / fft->fq_l), (1.0 / fft->width));
  head = (int)round(pos);
  lc   = (lc_t*)fft->line;

  for (i = 0; i < fft->width; i++) {
    pos *= step;
    tail = (int)round(pos);

    lc->pos = head;
    lc->n   = (tail == head)? 1: (tail - head);

    head = tail;
    lc++;
  }
}

static void
set_line_mapping(fft_t* fft)
{
  /*
   * calc step
   */
  switch (fft->mode) {
  case FFT_LINEARSCALE_MODE:
    set_linear_mapping(fft);
    break;

  case FFT_LOGSCALE_MODE:
    set_log_mapping(fft);
    break;
  }
}

int
fft_set_width(fft_t* fft, int width)
{
  int ret;
  lc_t* line;

  /*
   * initialize
   */
  ret  = 0;
  line = NULL;

  /*
   * argument check
   */
  if (fft == NULL) ret = ERR;
  if (!ret) {
    if (width > (fft->capa / 2)) ret = ERR;
  }

  /*
   * alloc spectrum line buffer
   */
  if (!ret) {
    line = NALLOC(lc_t, width);
    if (line == NULL) ret = ERR;
  }

  /*
   * modify FFT context
   */
  if (!ret) {
    if (fft->line) free(fft->line);
    fft->line  = line;
    fft->width = width;

    set_line_mapping(fft);
  }

  return ret;
}

int
fft_set_scale_mode(fft_t* fft, int mode)
{
  int ret;

  do {
    /*
     * argument check
     */
    if (fft == NULL) {
      ret = ERR;
      break;
    }

    if (mode != FFT_LOGSCALE_MODE &&
        mode != FFT_LINEARSCALE_MODE) {
      ret = ERR;
      break;
    }

    /*
     * modify FFT context
     */
    fft->mode = mode;
    set_line_mapping(fft);

    /*
     * mark success
     */
    ret = 0;
  } while(0);

  return ret;
}

int
fft_set_frequency(fft_t* fft, double s, double l, double h)
{
  int ret;

  do {
    /*
     * argument check
     */
    if (fft == NULL) {
      ret = ERR;
      break;
    }

    if (h > (s / 2.0)) {
      ret = ERR;
      break;
    }

    if (l > h) {
      ret = ERR;
      break;
    }

    /*
     * modify FFT context
     */
    fft->fq_s = s;
    fft->fq_h = h;
    fft->fq_l = l;

    set_line_mapping(fft);

    /*
     * mark success
     */
    ret = 0;
  } while(0);

  return ret;
}

int
fft_transform(fft_t* fft)
{
  int ret;
  int i;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument chack
   */
  if (fft == NULL) ret = ERR;

  /*
   * do transform
   */
  if (!ret) {
    for (i = 0; i < fft->capa; i++) {
      fft->a[i] = fft->data[i] * fft->wtbl[i];
    }

    rdft(fft->capa, 1, fft->a, fft->ip, fft->w);
  }

  return ret;
}

int
fft_calc_spectrum(fft_t* fft, double* dst)
{
  int ret;
  int i;
  int j;
  double* a;
  double v;
  double base;
  lc_t* lc;

  do {
    /*
     * initialize
     */
    ret = 0;

    /*
     * argument check
     */
    if (fft == NULL) {
      ret = ERR;
      break;
    }

    if (dst == NULL) {
      ret = ERR;
      break;
    }

    /*
     * calc power spectrum
     */
    //base = (double)fft->used;

    for (i = 0, lc = (lc_t*)fft->line; i < fft->width; i++, lc++) {
      v = 0;

      for(j = 0, a = fft->a + (lc->pos * 2); j < lc->n; j++, a += 2) {
        //v += 10.0 * log10(((a[0] * a[0]) + (a[1] * a[1])) / base);
        v += 10.0 * log10((a[0] * a[0]) + (a[1] * a[1]));
      }

      dst[i] = v / j;
    }
  } while(0);

  return ret;
}

/*
 * 注意
 *   この関数はfft_calcs_spectrum()と異なり、dB値への変換を
 *   行った結果を返します。
 */
int
fft_calc_amplitude(fft_t* fft, double* dst)
{
  int ret;
  int i;
  int j;
  double* a;
  double v;
  double base;
  lc_t* lc;

  do {
    /*
     * argument check
     */
    if (fft == NULL) {
      ret = ERR;
      break;
    }

    if (dst == NULL) {
      ret = ERR;
      break;
    }

    /*
     * calc power spectrum
     */
    base = (double)fft->used;

    for (i = 0, lc = (lc_t*)fft->line; i < fft->width; i++, lc++) {
      v = 0;

      for(j = 0, a = fft->a + (lc->pos * 2); j < lc->n; j++, a += 2) {
        v += 20.0 * log10(sqrt((a[0] * a[0]) + (a[1] * a[1])) / base);
      }

      dst[i] = v / j;
    }

    /*
     * mark success
     */
    ret = 0;
  } while(0);

  return ret;
}

int
fft_calc_absolute(fft_t* fft, double* dst)
{
  int ret;
  int i;
  int j;
  double* a;
  double v;
  double base;
  lc_t* lc;

  do {
    /*
     * argument check
     */
    if (fft == NULL) {
      ret = ERR;
      break;
    }

    if (dst == NULL) {
      ret = ERR;
      break;
    }

    /*
     * calc power spectrum
     */
    base = (double)fft->used;

    for (i = 0, lc = (lc_t*)fft->line; i < fft->width; i++, lc++) {
      v = 0;

      for(j = 0, a = fft->a + (lc->pos * 2); j < lc->n; j++, a += 2) {
        v += (sqrt((a[0] * a[0]) + (a[1] * a[1])) / base);
      }

      dst[i] = v / j;
    }

    /*
     * mark success
     */
    ret = 0;
  } while(0);

  return ret;
}
