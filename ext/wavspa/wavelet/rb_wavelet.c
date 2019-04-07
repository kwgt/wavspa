/*
 * Wavelet library interface for Ruby.
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include "ruby.h"
#include "ruby/thread.h"
#include "walet.h"

#define N(x)                        (sizeof((x))/sizeof(*(x)))
#define RUNTIME_ERROR(...)          rb_raise(rb_eRuntimeError, __VA_ARGS__)
#define ARGUMENT_ERROR(...)         rb_raise(rb_eArgError, __VA_ARGS__)
#define RB_FFT(p)                   ((rb_fft_t*)(p))
#define EQ_STR(val,str)             (rb_to_id(val) == rb_intern(str))

typedef struct {
  walet_t* wl;
} rb_wavelet_t;

static VALUE wavspa_module;
static VALUE wavelet_klass;

static const char* wavelet_opts_keys[] = {
  "sigma",            // {float}
  "gabor_threshold",  // {float}
  "frequency",        // {float}
  "range",            // {Range}
  "scale_mode",       // {str}
  "output_width",     // {int}
};

static ID wavelet_opts_ids[N(wavelet_opts_keys)];

VALUE symb_linear_scale;
VALUE symb_log_scale;

static void
rb_wavelet_free(void* _ptr)
{
  rb_wavelet_t* ptr;

  ptr = (rb_wavelet_t*)_ptr;

  if (ptr->wl != NULL) walet_destroy(ptr->wl);

  free(ptr);
}

static VALUE
rb_wavelet_alloc(VALUE self)
{
  rb_wavelet_t* ptr;

  ptr = ALLOC(rb_wavelet_t);
  memset(ptr, 0, sizeof(*ptr));

  return Data_Wrap_Struct(wavelet_klass, 0, rb_wavelet_free, ptr);
}

static void
eval_wavelet_opt_sigma(rb_wavelet_t* ptr, VALUE opt)
{
  int err;

  if (opt != Qundef) {
    err = walet_set_sigma(ptr->wl, NUM2DBL(opt));
    if (err) {
      RUNTIME_ERROR("walet_set_sigma() failed. [err=%d]", err);
    }
  }
}

static void
eval_wavelet_opt_gabor_threshold(rb_wavelet_t* ptr, VALUE opt)
{
  int err;

  if (opt != Qundef) {
    err = walet_set_gabor_threshold(ptr->wl, NUM2DBL(opt));
    if (err) {
      RUNTIME_ERROR("walet_set_gabor_threshold() failed. [err=%d]", err);
    }
  }
}

static void
eval_wavelet_opt_frequency(rb_wavelet_t* ptr, VALUE opt)
{
  int err;

  if (opt != Qundef) {
    err = walet_set_frequency(ptr->wl, NUM2DBL(opt));
    if (err) {
      RUNTIME_ERROR("walet_set_frequency() failed. [err=%d]", err);
    }
  }
}

static void
eval_wavelet_opt_range(rb_wavelet_t* ptr, VALUE opt)
{
  VALUE begin;
  VALUE end;
  int err;

  if (opt != Qundef) {
    begin = rb_funcall(opt, rb_intern("begin"), 0);
    end   = rb_funcall(opt, rb_intern("end"), 0);
    err   = walet_set_range(ptr->wl, NUM2DBL(begin), NUM2DBL(end));
    if (err) {
      RUNTIME_ERROR("walet_set_range() failed. [err=%d]", err);
    }
  }
}

static void
eval_wavelet_opt_scale_mode(rb_wavelet_t* ptr, VALUE opt)
{
  int err;
  int mode;

  if (opt != Qundef) {
    if (EQ_STR(opt, "LINEARSCALE") || EQ_STR(opt, "LINEAR")) {
      mode = WALET_LINEARSCALE_MODE;

    } else if (EQ_STR(opt, "LOGSCALE") || EQ_STR(opt, "LOG")) {
      mode = WALET_LOGSCALE_MODE;

    } else {
      ARGUMENT_ERROR("unsupported value.");
    }

    err = walet_set_scale_mode(ptr->wl, mode);
    if (err) {
      RUNTIME_ERROR("walet_set_scale_mode() failed. [err=%d]", err);
    }
  }
}

static void
eval_wavelet_opt_output_width(rb_wavelet_t* ptr, VALUE opt)
{
  int err;

  if (opt != Qundef) {
    err = walet_set_output_width(ptr->wl, FIX2INT(opt));
    if (err) {
      RUNTIME_ERROR("walet_set_output_width() failed. [err=%d]", err);
    }
  }
}

static void
set_wavelet_context(rb_wavelet_t* ptr, VALUE opt)
{
  VALUE opts[N(wavelet_opts_ids)];

  /*
   * parse options
   */
  rb_get_kwargs(opt, wavelet_opts_ids, 0, N(wavelet_opts_ids), opts);

  /*
   * set context
   */
  eval_wavelet_opt_sigma(ptr, opts[0]);
  eval_wavelet_opt_gabor_threshold(ptr, opts[1]);
  eval_wavelet_opt_frequency(ptr, opts[2]);
  eval_wavelet_opt_range(ptr, opts[3]);
  eval_wavelet_opt_scale_mode(ptr, opts[4]);
  eval_wavelet_opt_output_width(ptr, opts[5]);
}

static VALUE
rb_wavelet_initialize(int argc, VALUE argv[], VALUE self)
{
  rb_wavelet_t* ptr;
  int err;
  VALUE opt;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * parse argument
   */
  rb_scan_args(argc, argv, "01", &opt);

  if (opt != Qnil) {
    Check_Type(opt, T_HASH);
  }

  /*
   * create wavelet context
   */
  err = walet_new(&ptr->wl);
  if (err) {
    RUNTIME_ERROR("walet_new() failes [err=%d]\n", err);
  }

  /*
   * set context
   */
  if (opt != Qnil) {
    set_wavelet_context(ptr, opt);
  }

  return self;
}

static VALUE
rb_wavelet_get_sigma(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return paramter
   */
  ret = DBL2NUM(ptr->wl->sigma);

  return ret;
}

static VALUE
rb_wavelet_set_sigma(VALUE self, VALUE sigma)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_sigma(ptr, sigma);

  return sigma;
}

static VALUE
rb_wavelet_get_gabor_threshold(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return paramter
   */
  ret = DBL2NUM(ptr->wl->gth);

  return ret;
}

static VALUE
rb_wavelet_set_gabor_threshold(VALUE self, VALUE th)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_gabor_threshold(ptr, th);

  return th;
}

static VALUE
rb_wavelet_get_frequency(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return paramter
   */
  ret = DBL2NUM(ptr->wl->fq_s);

  return ret;
}

static VALUE
rb_wavelet_set_frequency(VALUE self, VALUE fq)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_frequency(ptr, fq);

  return fq;
}

static VALUE
rb_wavelet_get_range(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;

  VALUE begin;
  VALUE end;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return paramter
   */
  begin = rb_float_new(ptr->wl->fq_l);
  end   = rb_float_new(ptr->wl->fq_h);
  ret   = rb_range_new(begin, end, 0);

  return ret;
}

static VALUE
rb_wavelet_set_range(VALUE self, VALUE range)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_range(ptr, range);

  return range;
}

static VALUE
rb_wavelet_get_scale_mode(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return parameter
   */
  switch (ptr->wl->mode) {
  case 1:
    ret = symb_linear_scale;
    break;

  case 2:
    ret = symb_log_scale;
    break;

  default:
    RUNTIME_ERROR("Really?");
  }

  return ret;
}

static VALUE
rb_wavelet_set_scale_mode(VALUE self, VALUE mode)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_scale_mode(ptr, mode);

  return mode;
}

static VALUE
rb_wavelet_get_output_width(VALUE self)
{
  VALUE ret;
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return paramter
   */
  ret = INT2FIX(ptr->wl->width);

  return ret;
}


static VALUE
rb_wavelet_set_output_width(VALUE self, VALUE width)
{
  rb_wavelet_t* ptr;
  
  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call setter function
   */
  eval_wavelet_opt_output_width(ptr, width);

  return width;
}

static int
copy_rb_string(char* dst, VALUE _src, int lim)
{
  int ret;
  char* src;
  int n;

  /*
   * initialize
   */
  ret = 0;
  src = RSTRING_PTR(_src); 
  n   = RSTRING_LEN(_src);

  do {
    /*
     * check length
     */
    if (n >= lim) {
      ret = !0;
      break;
    }

    /*
     * copy data
     */
    memset(dst, 0, lim);
    memcpy(dst, src, n);
  } while (0);

  return ret;

}

static VALUE
rb_wavelet_put_in(VALUE self, VALUE _fmt, VALUE _smpl)
{
  rb_wavelet_t* ptr;
  int err;
  void* smpl;
  char fmt[8];
  int n;

  /*
   * check argument
   */
  Check_Type(_fmt, T_STRING);
  Check_Type(_smpl, T_STRING);

  /*
   * stript object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * unpack argument
   */
  smpl = RSTRING_PTR(_smpl);
  n    = RSTRING_LEN(_smpl);

  err  = copy_rb_string(fmt, _fmt, N(fmt));
  if (err) {
    ARGUMENT_ERROR("Illeagal format string.\n");
  }

  /*
   * eval format
   */
  if (strcasecmp("u8", fmt) == 0) {
    /* Nothing */

  } else if(strcasecmp("u16be", fmt) == 0 ||
            strcasecmp("u16le", fmt) == 0 ||
            strcasecmp("s16be", fmt) == 0 ||
            strcasecmp("s16le", fmt) == 0 ) {
    n /= 2;

  } else if(strcasecmp("u24be", fmt) == 0 ||
            strcasecmp("u24le", fmt) == 0) {
    n /= 3;

  } else {
    ARGUMENT_ERROR("Illeagal format string.\n");
  }

  /*
   * call base library
   */
  err = walet_put_in(ptr->wl, fmt, smpl, n);
  if (err) {
    RUNTIME_ERROR("walet_put_in() failed [err=%d]\n", err);
  }

  return self;
}

typedef struct {
  int err;
  walet_t* ptr;
  int pos;
} transform_arg_t;

static void*
_transform(void* data)
{
  transform_arg_t* arg;

  arg = (transform_arg_t*)data;

  arg->err = walet_transform(arg->ptr, arg->pos);

  return NULL;
}

static int
transform(walet_t* ptr, int pos)
{
  transform_arg_t arg;

  arg.ptr = ptr;
  arg.pos = pos;

  rb_thread_call_without_gvl(_transform, &arg, RUBY_UBF_PROCESS, NULL);

  return arg.err; 
}

static VALUE
rb_wavelet_transform(VALUE self, VALUE pos)
{
  rb_wavelet_t* ptr;
  int err;

  /*
   * stript object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * call base library
   */
  err = transform(ptr->wl, NUM2INT(pos));
  if (err) {
    RUNTIME_ERROR("walet_transform() failed [err=%d]\n", err);
  }

  return self;
}

typedef struct {
  int err;
  walet_t* ptr;
  double* dst;
} calc_power_arg_t;

static void*
_calc_power(void* data)
{
  calc_power_arg_t* arg;

  arg = (calc_power_arg_t*)data;

  arg->err = walet_calc_power(arg->ptr, arg->dst);

  return NULL;
}

static int
calc_power(walet_t* ptr, double* dst)
{
  calc_power_arg_t arg;

  arg.ptr = ptr;
  arg.dst = dst;

  rb_thread_call_without_gvl(_calc_power, &arg, RUBY_UBF_PROCESS, NULL);

  return arg.err;
}

static VALUE
rb_wavelet_power(VALUE self)
{
  rb_wavelet_t* ptr;
  VALUE ret;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return buffer
   */
  ret = rb_str_buf_new(sizeof(double) * ptr->wl->width);
  rb_str_set_len(ret, sizeof(double) * ptr->wl->width);

  /*
   * call base library
   */
  err = calc_power(ptr->wl, (double*)RSTRING_PTR(ret));
  if (err) {
    RUNTIME_ERROR("walet_calc_power() failed [err=%d]\n", err);
  }

  return ret;
}

typedef struct {
  int err;
  walet_t* ptr;
  double* dst;
} calc_amplitude_arg_t;

static void*
_calc_amplitude(void* data)
{
  calc_amplitude_arg_t* arg;

  arg = (calc_amplitude_arg_t*)data;

  arg->err = walet_calc_amplitude(arg->ptr, arg->dst);

  return NULL;
}

static int
calc_amplitude(walet_t* ptr, double* dst)
{
  calc_amplitude_arg_t arg;

  arg.ptr = ptr;
  arg.dst = dst;

  rb_thread_call_without_gvl(_calc_amplitude, &arg, RUBY_UBF_PROCESS, NULL);

  return arg.err;
}

static VALUE
rb_wavelet_amplitude(VALUE self)
{
  rb_wavelet_t* ptr;
  VALUE ret;
  int err;

  /*
   * strip object
   */
  Data_Get_Struct(self, rb_wavelet_t, ptr);

  /*
   * create return buffer
   */
  ret = rb_str_buf_new(sizeof(double) * ptr->wl->width);
  rb_str_set_len(ret, sizeof(double) * ptr->wl->width);

  /*
   * call base library
   */
  err = calc_amplitude(ptr->wl, (double*)RSTRING_PTR(ret));
  if (err) {
    RUNTIME_ERROR("walet_calc_amplitude() failed [err=%d]\n", err);
  }

  return ret;
}

void
Init_wavelet()
{
  int i;

  wavspa_module = rb_define_module("WavSpectrumAnalyzer");
	wavelet_klass = rb_define_class_under(wavspa_module, "Wavelet", rb_cObject);

  rb_define_alloc_func(wavelet_klass, rb_wavelet_alloc);

  rb_define_method(wavelet_klass, "initialize", rb_wavelet_initialize, -1);
  rb_define_method(wavelet_klass, "sigma", rb_wavelet_get_sigma, 0);
  rb_define_method(wavelet_klass, "sigma=", rb_wavelet_set_sigma, 1);
  rb_define_method(wavelet_klass, "gabor_threshold",
                                  rb_wavelet_get_gabor_threshold, 0);
  rb_define_method(wavelet_klass, "gabor_threshold=",
                                  rb_wavelet_set_gabor_threshold, 1);
  rb_define_method(wavelet_klass, "frequency", rb_wavelet_get_frequency, 0);
  rb_define_method(wavelet_klass, "frequency=", rb_wavelet_set_frequency, 1);
  rb_define_method(wavelet_klass, "range", rb_wavelet_get_range, 0);
  rb_define_method(wavelet_klass, "range=", rb_wavelet_set_range, 1);
  rb_define_method(wavelet_klass, "scale_mode", rb_wavelet_get_scale_mode, 0);
  rb_define_method(wavelet_klass, "scale_mode=", rb_wavelet_set_scale_mode, 1);
  rb_define_method(wavelet_klass, "width", rb_wavelet_get_output_width, 0);
  rb_define_method(wavelet_klass, "width=", rb_wavelet_set_output_width, 1);

  rb_define_method(wavelet_klass, "put_in", rb_wavelet_put_in, 2);
  rb_define_method(wavelet_klass, "transform", rb_wavelet_transform, 1);
  rb_define_method(wavelet_klass, "power", rb_wavelet_power, 0);
  rb_define_method(wavelet_klass, "amplitude", rb_wavelet_amplitude, 0);
	
  for (i = 0; i < (int)N(wavelet_opts_keys); i++) {
    wavelet_opts_ids[i] = rb_intern(wavelet_opts_keys[i]);
  }

  symb_linear_scale = ID2SYM(rb_intern_const("LINEAR_SCALE"));
  symb_log_scale    = ID2SYM(rb_intern_const("LOG_SCALE"));
}
