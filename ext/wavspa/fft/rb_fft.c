/*
 * Ooura FFT library interface fro Ruby
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

#include "ruby.h"
#include "fft.h"

#include <stdint.h>
#include <string.h>

#define N(x)                        (sizeof((x))/sizeof(*(x)))
#define RUNTIME_ERROR(...)          rb_raise(rb_eRuntimeError, __VA_ARGS__)
#define ARGUMENT_ERROR(...)         rb_raise(rb_eArgError, __VA_ARGS__)
#define RB_FFT(p)                   ((rb_fft_t*)(p))
#define EQ_STR(val,str)             (rb_to_id(val) == rb_intern(str))

typedef struct {
  fft_t* fft;
} rb_fft_t;

static VALUE wavspa_module;
static VALUE fft_klass;

static void
rb_fft_free(void* ptr)
{
  fft_destroy(RB_FFT(ptr)->fft);
  free(ptr);
}

static VALUE
rb_fft_alloc(VALUE self)
{
  rb_fft_t* ptr;

  ptr = xmalloc(sizeof(rb_fft_t));

  ptr->fft = NULL;

  return Data_Make_Struct(fft_klass, rb_fft_t, 0, rb_fft_free, ptr);
}

static VALUE
rb_fft_initialize(VALUE self, VALUE fmt, VALUE capa)
{
  rb_fft_t* ptr;
  int err;

  /*
   * check argument
   */
  Check_Type(fmt, T_STRING);
  Check_Type(capa, T_FIXNUM);

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * create fft context
   */
  err = fft_new(RSTRING_PTR(fmt), FIX2INT(capa), &ptr->fft);
  if (err) {
    RUNTIME_ERROR( "fft_new() failed. [err = %d]\n", err);
  }

  return Qtrue;
}

static VALUE
rb_fft_shift_in(VALUE self, VALUE data)
{
  rb_fft_t* ptr;
  int err;
  void* src;
  int n;

  /*
   * check argument
   */
  Check_Type(data, T_STRING);

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call fft library
   */

  src = RSTRING_PTR(data);
  n   = RSTRING_LEN(data) / (ptr->fft->fmt & 0x000f);

  err = fft_shift_in(ptr->fft, src, n);
  if (err) {
    RUNTIME_ERROR( "fft_shift_in() failed. [err = %d]\n", err);
  }

  return self;
}

static VALUE
rb_fft_reset(VALUE self)
{
  rb_fft_t* ptr;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call fft library
   */
  err = fft_reset(ptr->fft);

  if (err) {
    RUNTIME_ERROR( "fft_reset() failed. [err = %d]\n", err);
  }

  return self;
}

static VALUE
rb_fft_transform(VALUE self)
{
  rb_fft_t* ptr;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call transform function
   */
  err = fft_transform(ptr->fft);
  if (err) {
    RUNTIME_ERROR( "fft_transform() failed. [err = %d]\n", err);
  }

  return self;
}

static VALUE
rb_fft_enqueue(VALUE self, VALUE data)
{
  rb_fft_shift_in(self, data);
  rb_fft_transform(self);

  return self;
}

static VALUE
rb_fft_set_window(VALUE self, VALUE _type)
{
  rb_fft_t* ptr;
  int type;
  int err;

  /*
   * check argument
   */
  if (TYPE(_type) != T_STRING && TYPE(_type) != T_SYMBOL) {
    RUNTIME_ERROR( "Window type is unsupported data");

  } else if (EQ_STR(_type, "RECTANGULAR")) {
    type = FFT_WINDOW_RECTANGULAR;

  } else if (EQ_STR(_type, "HAMMING")) {
    type = FFT_WINDOW_HAMMING;

  } else if (EQ_STR(_type, "HANN")) {
    type = FFT_WINDOW_HANN;

  } else if (EQ_STR(_type, "BLACKMAN")) {
    type = FFT_WINDOW_BLACKMAN;

  } else if (EQ_STR(_type, "BLACKMAN_NUTTALL")) {
    type = FFT_WINDOW_BLACKMAN_NUTTALL;

  } else if (EQ_STR(_type, "FLAT_TOP")) {
    type = FFT_WINDOW_FLAT_TOP;

  } else {
    RUNTIME_ERROR( "Unknown window type");
  }

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call set window
   */
  err = fft_set_window(ptr->fft, type);
  if (err) {
    RUNTIME_ERROR( "fft_set_window() failed. [err = %d]\n", err);
  }

  return _type;
}

static VALUE
rb_fft_get_width(VALUE self)
{
  rb_fft_t* ptr;

  Data_Get_Struct(self, rb_fft_t, ptr);

  return INT2FIX(ptr->fft->width);
}

static VALUE
rb_fft_set_width(VALUE self, VALUE width)
{
  rb_fft_t* ptr;
  int err;

  /*
   * check argument
   */
  Check_Type(width, T_FIXNUM);

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call set window
   */
  err = fft_set_width(ptr->fft, FIX2INT(width));
  if (err) {
    RUNTIME_ERROR( "fft_set_width() failed. [err = %d]\n", err);
  }

  return width;
}

static VALUE
rb_fft_get_scale_mode(VALUE self)
{
  VALUE ret;
  rb_fft_t* ptr;

  Data_Get_Struct(self, rb_fft_t, ptr);

  switch (ptr->fft->mode) {
  case FFT_LOGSCALE_MODE:
    ret = ID2SYM(rb_intern("LOGSCALE"));
    break;

  case FFT_LINEARSCALE_MODE:
    ret = ID2SYM(rb_intern("LINEARSCALE"));
    break;

  default:
    RUNTIME_ERROR( "Really?");
  }

  return ret;
}

static VALUE
rb_fft_set_scale_mode(VALUE self, VALUE _mode)
{
  rb_fft_t* ptr;
  int err;
  int mode;

  /*
   * check argument
   */
  if (EQ_STR(_mode, "LOGSCALE") || EQ_STR(_mode, "LOG")) {
    mode = FFT_LOGSCALE_MODE;

  } else if (EQ_STR(_mode, "LINEARSCALE") || EQ_STR(_mode, "LINEAR")) {
    mode = FFT_LINEARSCALE_MODE;

  } else {
    ARGUMENT_ERROR("not supported value");
  }

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call set scale mode function
   */
  err = fft_set_scale_mode(ptr->fft, mode);
  if (err) {
    RUNTIME_ERROR( "fft_set_scale_mode() failed. [err = %d]\n", err);
  }

  return _mode;
}

static VALUE
rb_fft_set_frequency(VALUE self, VALUE freq)
{
  rb_fft_t* ptr;
  int err;
  double s;
  double h;
  double l;

  /*
   * check argument
   */
  Check_Type(freq, T_ARRAY);
  if (RARRAY_LEN(freq) != 3) {
    ARGUMENT_ERROR("frequency set shall be 3 entries contain.");
  }

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * call set fequency
   */
  s   = NUM2DBL(RARRAY_AREF(freq, 0));
  l   = NUM2DBL(RARRAY_AREF(freq, 1));
  h   = NUM2DBL(RARRAY_AREF(freq, 2));
  err = fft_set_frequency(ptr->fft, s, l, h);
  if (err) {
    RUNTIME_ERROR( "fft_set_frequency() failed. [err = %d]\n", err);
  }

  return freq;
}


static VALUE
rb_fft_spectrum(VALUE self)
{
  rb_fft_t* ptr;
  VALUE ret;
  int err;
  int i;
  double* dat;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * alloc return object
   */
  ret = rb_str_buf_new(sizeof(double) * ptr->fft->width);
  rb_str_set_len(ret, sizeof(double) * ptr->fft->width);

  /*
   * call transform function
   */
  dat = RSTRING_PTR(ret);
  err = fft_calc_spectrum(ptr->fft, dat);
  if (err) {
    RUNTIME_ERROR( "fft_calc_spectrum() failed. [err = %d]\n", err);
  }

  return ret;
}

static VALUE
rb_fft_amplitude(VALUE self)
{
  rb_fft_t* ptr;
  VALUE ret;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * alloc return object
   */
  ret = rb_str_buf_new(sizeof(double) * ptr->fft->width);
  rb_str_set_len(ret, sizeof(double) * ptr->fft->width);

  /*
   * call inverse function
   */
  err = fft_calc_amplitude(ptr->fft, (double*)RSTRING_PTR(ret));
  if (err) {
    RUNTIME_ERROR( "fft_calc_amplitude() failed. [err = %d]\n", err);
  }

  return ret;
}

static VALUE
rb_fft_absolute(VALUE self)
{
  rb_fft_t* ptr;
  VALUE ret;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_fft_t, ptr);

  /*
   * alloc return object
   */
  ret = rb_str_buf_new(sizeof(double) * ptr->fft->width);
  rb_str_set_len(ret, sizeof(double) * ptr->fft->width);

  /*
   * call inverse function
   */
  err = fft_calc_absolute(ptr->fft, (double*)RSTRING_PTR(ret));
  if (err) {
    RUNTIME_ERROR( "fft_calc_absolute() failed. [err = %d]\n", err);
  }

  return ret;
}


void
Init_fft()
{
  wavspa_module = rb_define_module("WavSpectrumAnalyzer");
  fft_klass     = rb_define_class_under(wavspa_module, "FFT", rb_cObject);

  rb_define_alloc_func(fft_klass, rb_fft_alloc);

  rb_define_method(fft_klass, "initialize", rb_fft_initialize, 2);

  rb_define_method(fft_klass, "window=", rb_fft_set_window, 1);
  rb_define_method(fft_klass, "width", rb_fft_get_width, 0);
  rb_define_method(fft_klass, "width=", rb_fft_set_width, 1);
  rb_define_method(fft_klass, "scale_mode", rb_fft_get_scale_mode, 0);
  rb_define_method(fft_klass, "scale_mode=", rb_fft_set_scale_mode, 1);
  rb_define_method(fft_klass, "frequency=", rb_fft_set_frequency, 1);

  rb_define_method(fft_klass, "shift_in", rb_fft_shift_in, 1);
  rb_define_method(fft_klass, "reset", rb_fft_reset, 0);
  rb_define_method(fft_klass, "transform", rb_fft_transform, 0);
  rb_define_method(fft_klass, "enqueue", rb_fft_enqueue, 1);
  rb_define_method(fft_klass, "<<", rb_fft_enqueue, 1);
  rb_define_method(fft_klass, "spectrum", rb_fft_spectrum, 0);
  rb_define_method(fft_klass, "amplitude", rb_fft_amplitude, 0);
  rb_define_method(fft_klass, "absolute", rb_fft_absolute, 0);
}
