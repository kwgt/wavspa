/*
 * wavelet transform library
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

/*
 * $Id: walet.c 149 2017-07-28 02:23:16Z kgt $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "walet.h"

#define N(x)                    (sizeof(x)/sizeof(*x))
#define IS_POW2(n)              (!((n) & ((n) - 1)))
#define ALLOC(t)                ((t*)malloc(sizeof(t)))
#define NALLOC(t,n)             ((t*)malloc(sizeof(t) * (n)))
#define MAX(m,n)                (((m) > (n))? (m): (n))
                               
#define ERR                     __LINE__
                               
#define M_PI2                   (M_PI * 2.0)
                               
#define DEFAULT_BASE_FREQ       44100.0
#define DEFAULT_LOW_FREQ        100.0
#define DEFAULT_HIGH_FREQ       2000.0
#define DEFAULT_SIGMA           3.0
#define DEFAULT_GABOR_THRESHOLD 0.01
#define DEFAULT_OUTPUT_WIDTH    360
#define DEFAULT_SCALE_MODE      WALET_LOGSCALE_MODE

#define F_DIRTY                 0x00000001
                
#define CALC_WK0(sig,th)        ((sig) * sqrt(-2.0 * log(th)))
#define CALC_WK1(sig)           (1.0 / sqrt(M_PI2 * (sig) * (sig)))
#define CALC_WK2(sig)           (2.0 * (sig) * (sig))
                             
static double
calc_step(int mode, double low, double high, int width, double* tbl)
{
  double ret;
  int i;

  switch (mode) {
  case WALET_LINEARSCALE_MODE:
    ret = (high - low) / width;
    for (i = 0; i < width; i++) tbl[i] = low + (ret * i);
    break;

  case WALET_LOGSCALE_MODE:
    ret = pow(high / low, 1.0 / (double)width);
    for (i = 0; i < width; i++) tbl[i] = low * pow(ret, i);
    break;

  default:
    ret = NAN;
    break;
  }

  return ret;
}

static void
reset_window_size_table(walet_t* ptr)
{
  int i;

  for (i = 0; i < ptr->width; i++) {
    ptr->ws[i] = (int)(((1.0 / ptr->ft[i]) * ptr->wk0) * ptr->fq_s);
  }
}

int
walet_new(walet_t** _obj)
{
  int ret;
  walet_t* obj;
  int* ws;
  double* wt;
  double* ft;

  /*
   * initialize
   */
  ret = 0;
  obj = NULL;
  wt  = NULL;
  ws  = NULL;
  ft  = NULL;

  do {
    /*
     * chack argument
     */
    if (_obj == NULL) {
      ret = ERR;
      break;
    }

    /*
     * alloc new object
     */
    obj = ALLOC(walet_t);
    if (obj == NULL) {
      ret = ERR;
      break;
    }

    ws = NALLOC(int, DEFAULT_OUTPUT_WIDTH);
    if (ws == NULL) {
      ret = ERR;
      break;
    }

    wt = NALLOC(double, DEFAULT_OUTPUT_WIDTH * 2);
    if (wt == NULL) {
      ret = ERR;
      break;
    }

    ft = NALLOC(double, DEFAULT_OUTPUT_WIDTH);
    if (ft == NULL) {
      ret = ERR;
      break;
    }

    /*
     * set initial parameter
     */
    obj->flags = F_DIRTY;
    obj->fq_s  = DEFAULT_BASE_FREQ;
    obj->fq_l  = DEFAULT_LOW_FREQ;
    obj->fq_h  = DEFAULT_HIGH_FREQ;
    obj->sigma = DEFAULT_SIGMA;
    obj->gth   = DEFAULT_GABOR_THRESHOLD;
    obj->wk0   = CALC_WK0(DEFAULT_SIGMA, obj->gth);
    obj->wk1   = CALC_WK1(DEFAULT_SIGMA);
    obj->wk2   = CALC_WK2(DEFAULT_SIGMA);
    obj->width = DEFAULT_OUTPUT_WIDTH;
    obj->mode  = DEFAULT_SCALE_MODE;
    obj->step  = calc_step(obj->mode, obj->fq_l, obj->fq_h, obj->width, ft);
    obj->smpl  = NULL;
    obj->ws    = ws;
    obj->wt    = wt;
    obj->ft    = ft;

    /*
     * put return parameter
     */
    *_obj = obj;
  } while (0);

  /*
   * post process
   */
  if (ret) {
    if (ws != NULL) free(ws);
    if (wt != NULL) free(wt);
    if (ft != NULL) free(ft);
    if (obj != NULL) free(obj);
  }

  return ret;
}

int
walet_set_sigma(walet_t* ptr, double sigma)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (ptr == NULL) ret = ERR;

  /*
   * set parameter
   */
  if (!ret) {
    ptr->sigma  = sigma;
    ptr->wk0    = CALC_WK0(sigma, ptr->gth);
    ptr->wk1    = CALC_WK1(sigma);
    ptr->wk2    = CALC_WK2(sigma);

    ptr->flags |= F_DIRTY;
  }

  return ret;
}

int
walet_set_gabor_threshold(walet_t* ptr, double th)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (ptr == NULL) ret = ERR;

  /*
   * set parameter
   */
  if (!ret) {
    ptr->gth = th;
    ptr->wk0 = CALC_WK0(ptr->sigma, th);
    ptr->wk1 = CALC_WK1(ptr->sigma);
    ptr->wk2 = CALC_WK2(ptr->sigma);

    ptr->flags |= F_DIRTY;
  }

  return ret;
}

int
walet_set_frequency(walet_t* ptr, double freq)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (freq <= 100) {
      ret = ERR;
      break;
    }

    /*
     * set parameter
     */
    ptr->fq_s   = freq;
    ptr->fq_h   = freq / 2.0;
    ptr->fq_l   = freq / 5.0;

    ptr->flags |= F_DIRTY;
  } while (0);

  return ret;
}

int
walet_set_range(walet_t* ptr, double low, double high)
{
  int ret;
  double step;

  /*
   * initialize
   */
  ret = 0;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (high <= 0 || high > (ptr->fq_s / 2.0)) {
      ret = ERR;
      break;
    }

    if (low >= high) {
      ret = ERR;
      break;
    }

    /*
     * calc step
     */
    step = calc_step(ptr->mode, low, high, ptr->width, ptr->ft);
    if (isnan(step)) {
      ret = ERR;
      break;
    }

    /*
     * set parameter
     */
    ptr->fq_l   = low;
    ptr->fq_h   = high;
    ptr->step   = step;

    ptr->flags |= F_DIRTY;
  } while(0);

  return ret;
}

int
walet_set_scale_mode(walet_t* ptr, int mode)
{
  int ret;
  double step;

  /*
   * initialize
   */
  ret = 0;

  do {
    /* 
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (mode != WALET_LINEARSCALE_MODE &&
        mode != WALET_LOGSCALE_MODE) {
      ret = ERR;
      break;
    }

    /*
     * calc step
     */
    step = calc_step(mode, ptr->fq_l, ptr->fq_h, ptr->width, ptr->ft);
    if (isnan(step)) {
      ret = ERR;
      break;
    }

    /*
     * set parameter
     */
    ptr->mode   = mode;
    ptr->step   = step;

    ptr->flags |= F_DIRTY;
  } while (0);

  return ret;
}

int
walet_set_output_width(walet_t* ptr, int width)
{
  int ret;
  int* ws;
  double* wt;
  double* ft;
  double step;

  /*
   * initialize
   */
  ret = 0;
  ws  = NULL;
  wt  = NULL;
  ft  = NULL;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (width < 32) {
      ret = ERR;
      break;
    }

    /*
     * alloc new buffers
     */
    ws = NALLOC(int, width);
    if (ws == NULL) {
      ret = ERR;
    }

    wt = NALLOC(double, width * 2);
    if (wt == NULL) {
      ret = ERR;
    }

    ft = NALLOC(double, width);
    if (ft == NULL) {
      ret = ERR;
    }

    /*
     * calc step
     */
    step = calc_step(ptr->mode, ptr->fq_l, ptr->fq_h, width, ft);
    if (isnan(step)) {
      ret = ERR;
      break;
    }

    /*
     * set parameter
     */
    if (ptr->wt != NULL) free(ptr->wt);
    if (ptr->ws != NULL) free(ptr->ws);
    if (ptr->ft != NULL) free(ptr->ft);

    ptr->width = width;
    ptr->ws    = ws;
    ptr->wt    = wt;
    ptr->ft    = ft;
    ptr->step  = step;

    ptr->flags |= F_DIRTY;
  } while (0);

  /*
   * post process
   */
  if (ret) {
    if (wt != NULL) free(wt);
    if (ws != NULL) free(ws);
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

static void
import_double(double* dst, double* src, int n)
{
  memcpy(dst, src, sizeof(double) * n);
}

int
walet_put_in(walet_t* ptr, char* fmt, void* data, size_t n)
{
  int ret;
  double* smpl;

  /*
   * initialize
   */
  ret  = 0;
  smpl = NULL;


  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (data == NULL) {
      ret = ERR;
      break;
    }

    /*
     * alloc sample buffer
     */
    smpl = NALLOC(double, n);
    if (smpl == NULL) {
      ret = ERR;
      break;
    }

    /*
     * import samples
     */
    if (strcasecmp("u8", fmt) == 0) {
      import_u8(smpl, data, n);

    } else if (strcasecmp("u16be", fmt) == 0) {
      import_u16be(smpl, data, n);

    } else if (strcasecmp("u16le", fmt) == 0) {
      import_u16le(smpl, data, n);

    } else if (strcasecmp("s16be", fmt) == 0) {
      import_s16be(smpl, data, n);

    } else if (strcasecmp("s16le", fmt) == 0) {
      import_s16le(smpl, data, n);

    } else if (strcasecmp("s24be", fmt) == 0) {
      import_s24be(smpl, data, n);

    } else if (strcasecmp("s24le", fmt) == 0) {
      import_s24le(smpl, data, n);

    } else if (strcasecmp("dbl", fmt) == 0) {
      import_double(smpl, data, n);

    } else {
      ret = ERR;
      break;
    }

    /*
     * put parameter
     */
    ptr->smpl = smpl;
    ptr->n    = n;
  } while (0);

  /*
   * post process
   */
  if (ret) {
    if (smpl != NULL) free(smpl);
  }

  return ret;
}

int
walet_transform(walet_t* ptr, int pos)
{
  int ret;
  int dx;

  int i;
  int j;
  int st;
  int ed;

  double t;
  double gss;  // as gauss
  double omt;  // as omega-t
  double re;
  double im;
  double* wt;

  /*
   * initialize
   */
  ret = 0;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (pos < 0 || pos >= ptr->n) {
      ret = ERR;
      break;
    }

    /*
     * pre process
     */
    if (ptr->flags & F_DIRTY) {
      reset_window_size_table(ptr);

      ptr->flags &= ~F_DIRTY;
    }

    /*
     * integla for window
     */
    for (i = 0, wt = ptr->wt; i < ptr->width; i++, wt += 2) {
      dx = ptr->ws[i];

      st = (dx < pos)? -dx : -pos;
      ed = (dx < (ptr->n - pos))? dx: (ptr->n - (pos + 1));

      re = 0.0;
      im = 0.0;

#ifdef _OPENMP
#pragma omp parallel for private(t,gss,omt) reduction(+:re,im)
#endif /* defined(_OPENMP) */
      for (j = st; j <= ed; j++) {
        t   = ((double)j / ptr->fq_s) * ptr->ft[i];
        gss = ptr->wk1 * exp(-t * (t / ptr->wk2)) * (ptr->smpl[pos + j]);
        omt = M_PI2 * t;

        re += cos(omt) * gss;
        im += sin(omt) * gss;
      }


      wt[0] = re;
      wt[1] = im;
    }

  } while (0);

  return ret;
}

int
walet_calc_power(walet_t* ptr, double* dst)
{
  int ret;
  int i;
  double* wt;

  /*
   * initialize
   */
  ret = 0;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (dst == NULL) {
      ret = ERR;
      break;
    }

    /*
     * put power 
     */

#ifdef _OPENMP
#pragma omp parallel private(wt)
#endif /* defined(_OPENMP) */

    for (i = 0; i < ptr->width; i++) {
      wt = ptr->wt + (i * 2);

      // 末尾の計数256はFFTでの表示に合わせて適当に値を見繕ってるので注意
      dst[i] = (sqrt((wt[0] * wt[0]) + (wt[1] * wt[1])) / ptr->ft[i]) * 256;
    }
  } while (0);

  return ret;
}

int
walet_calc_amplitude(walet_t* ptr, double* dst)
{
  int ret;
  int i;
  double* wt;
  double base;

  /*
   * initialize
   */
  ret = 0;

  do {
    /*
     * argument check
     */
    if (ptr == NULL) {
      ret = ERR;
      break;
    }

    if (dst == NULL) {
      ret = ERR;
      break;
    }

    /*
     * put amplitude 
     */
#ifdef _OPENMP
#pragma omp parallel private(base,wt)
#endif /* defined(_OPENMP) */
    for (i = 0; i < ptr->width; i++) {
      wt     = ptr->wt + (i * 2);
      base   = ptr->ws[i] * 2;
      dst[i] = 20.0 * log10(sqrt(((wt[0] * wt[0]) + (wt[1] * wt[1])) / base));
    }
  } while (0);

  return ret;
}

int
walet_destroy(walet_t* ptr)
{
  int ret;

  /*
   * initialize
   */
  ret = 0;

  /*
   * argument check
   */
  if (ptr == NULL) ret = ERR;

  /*
   * release object
   */
  if (!ret) {
    if (ptr->smpl != NULL) free(ptr->smpl);
    if (ptr->ws != NULL) free(ptr->ws);
    if (ptr->wt != NULL) free(ptr->wt);
    if (ptr->ft != NULL) free(ptr->ft);
    free(ptr);
  }

  return ret;
}
