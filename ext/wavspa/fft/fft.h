/*
 * Ooura FFT library interface
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

/*
 * $Id$
 */

#ifndef __FFT_H__
#define __FFT_H__

#define FFT_WINDOW_RECTANGULAR        0
#define FFT_WINDOW_HAMMING            1
#define FFT_WINDOW_HANN               2
#define FFT_WINDOW_BLACKMAN           3
#define FFT_WINDOW_BLACKMAN_NUTTALL   4
#define FFT_WINDOW_FLAT_TOP           5

#define FFT_LINEARSCALE_MODE          1
#define FFT_LOGSCALE_MODE             2

typedef struct {
  double* data;
  double* wtbl; 
  int fmt;

  int capa;
  int used;

  int magnify;

  void* line;
  int width;

  int mode;
  double fq_s;  // as "sampline frequency"
  double fq_h;  // as "high-side frequency"
  double fq_l;  // as "low-side frequency"

  double* a;
  int* ip;
  double* w;
} fft_t;

int fft_new(char* fmt, int capa, fft_t** obj);
int fft_destroy(fft_t* fft);

int fft_set_window(fft_t* fft, int type);
int fft_set_width(fft_t* fft, int width);
int fft_set_scale_mode(fft_t* fft, int mode);
int fft_set_frequency(fft_t* fft, double s, double l, double h);

int fft_shift_in(fft_t* fft, void* data, int n);
int fft_reset(fft_t* fft);
int fft_transform(fft_t* fft);
int fft_calc_power(fft_t* fft, double* dst);
int fft_calc_amplitude(fft_t* fft, double* dst);
int fft_calc_absolute(fft_t* fft, double* dst);

#endif /* !defined(__FFT_H__) */
