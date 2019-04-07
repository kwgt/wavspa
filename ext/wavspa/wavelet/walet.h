/*
 * Wavelet transform library
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

/*
 * $Id: walet.h 91 2016-07-04 03:05:03Z kgt $
 */

#ifndef __WALET_H__
#define __WALET_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define WALET_LINEARSCALE_MODE    1
#define WALET_LOGSCALE_MODE       2

typedef struct __walet__ {
  int flags;

  double fq_s;  // as "sampleing frequency"
  double fq_l;  // as "low side frequency"
  double fq_h;  // as "high side fequency"

  double sigma;
  double gth;   // as "gabor threshold"
  double wk0;
  double wk1;
  double wk2;
  int* ws;   // as window size list
  double** exp;

  int width;
  int mode;
  double step;

  double* smpl; // as "sample data"
  int n;        // as "number of sample size"

  double* wt;
} walet_t;

int walet_new(walet_t** ptr);
int walet_destroy(walet_t* ptr);

int walet_set_sigma(walet_t* ptr, double sigma);
int walet_set_gabor_threshold(walet_t* ptr, double th);
int walet_set_frequency(walet_t* ptr, double freq);
int walet_set_range(walet_t* ptr, double low, double high);
int walet_set_scale_mode(walet_t* ptr, int mode);
int walet_set_output_width(walet_t* ptr, int width);

int walet_put_in(walet_t* ptr, char* fmt, void* data, size_t size);
int walet_transform(walet_t* ptr, int pos);
int walet_calc_power(walet_t* ptr, double* dst);
int walet_calc_amplitude(walet_t* ptr, double* dst);

#endif /* !defined(__WALET_H__) */
