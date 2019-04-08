﻿/*
 * Simple Frame buffer
 *
 *  Copyright (C) 2016 Hiroshi Kuwagata <kgt9221@gmail.com>
 */

#include "ruby.h"

#include <stdint.h>
#include <string.h>

#define N(x)                        (sizeof((x))/sizeof(*(x)))
#define RUNTIME_ERROR(...)          rb_raise(rb_eRuntimeError, __VA_ARGS__)
#define ARGUMENT_ERROR(...)         rb_raise(rb_eArgError, __VA_ARGS__)
#define RB_FFT(p)                   ((rb_fft_t*)(p))
#define EQ_STR(val,str)             (rb_to_id(val) == rb_intern(str))

typedef struct {
  int width;
  int height;
  int step;
  int margin_x;
  int margin_y;

  int stride;
  int size;

  double ceil;
  double floor;
  double range;

  VALUE buf;
} rb_fb_t;

/*
 * from M+ font (M+ gothic 10r)
 */
const uint8_t font[] = {
  0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x00, /* 0x00 */
  0x00, 0x00, 0x00, 0x60, 0xf0, 0x60, 0x00, 0x00, 0x00, 0x00, /* 0x01 */
  0x50, 0xa8, 0x50, 0xa8, 0x50, 0xa8, 0x50, 0xa8, 0x50, 0xa8, /* 0x02 */
  0x90, 0xf0, 0x90, 0x90, 0x00, 0xf8, 0x20, 0x20, 0x20, 0x00, /* 0x03 */
  0xf0, 0x80, 0xf0, 0x80, 0x00, 0xf0, 0x80, 0xf0, 0x80, 0x00, /* 0x04 */
  0xf0, 0x90, 0x80, 0xf0, 0x00, 0xf0, 0x90, 0xe0, 0x90, 0x00, /* 0x05 */
  0x80, 0x80, 0x80, 0xf0, 0x00, 0xf0, 0x80, 0xf0, 0x80, 0x00, /* 0x06 */
  0x60, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x07 */
  0x00, 0x40, 0x40, 0xf0, 0x40, 0x40, 0x00, 0xf0, 0x00, 0x00, /* 0x08 */
  0x90, 0xd0, 0xb0, 0x90, 0x00, 0x80, 0x80, 0x80, 0xf0, 0x00, /* 0x09 */
  0x90, 0x90, 0xa0, 0xc0, 0x00, 0xf8, 0x20, 0x20, 0x20, 0x00, /* 0x0a */
  0x20, 0x20, 0x20, 0x20, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0b */
  0x00, 0x00, 0x00, 0x00, 0xe0, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x0c */
  0x00, 0x00, 0x00, 0x00, 0x38, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x0d */
  0x20, 0x20, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0e */
  0x20, 0x20, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x0f */
  0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10 */
  0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11 */
  0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x12 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, /* 0x13 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, /* 0x14 */
  0x20, 0x20, 0x20, 0x20, 0x38, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x15 */
  0x20, 0x20, 0x20, 0x20, 0xe0, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x16 */
  0x20, 0x20, 0x20, 0x20, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x17 */
  0x00, 0x00, 0x00, 0x00, 0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x18 */
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x19 */
  0x00, 0x20, 0x40, 0x80, 0x40, 0x20, 0x00, 0xf0, 0x00, 0x00, /* 0x1a */
  0x00, 0x40, 0x20, 0x10, 0x20, 0x40, 0x00, 0xf0, 0x00, 0x00, /* 0x1b */
  0x00, 0x00, 0x00, 0xf0, 0x50, 0x50, 0x90, 0x90, 0x00, 0x00, /* 0x1c */
  0x00, 0x20, 0x20, 0xf0, 0x40, 0xf0, 0x40, 0x40, 0x00, 0x00, /* 0x1d */
  0x00, 0x30, 0x40, 0x40, 0xf0, 0x40, 0x40, 0xb0, 0x00, 0x00, /* 0x1e */
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20 */
  0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20, 0x00, 0x00, /* 0x21 */
  0x50, 0x50, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x22 */
  0x00, 0x50, 0x50, 0xf0, 0x50, 0xf0, 0x50, 0x50, 0x00, 0x00, /* 0x23 */
  0x00, 0x20, 0x70, 0xa0, 0xe0, 0x70, 0x50, 0xe0, 0x40, 0x00, /* 0x24 */
  0x00, 0xc0, 0xc0, 0x10, 0x60, 0x80, 0x30, 0x30, 0x00, 0x00, /* 0x25 */
  0x00, 0x40, 0xa0, 0xa0, 0x40, 0xb0, 0xa0, 0x50, 0x00, 0x00, /* 0x26 */
  0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x27 */
  0x10, 0x20, 0x20, 0x40, 0x40, 0x40, 0x20, 0x20, 0x10, 0x00, /* 0x28 */
  0x80, 0x40, 0x40, 0x20, 0x20, 0x20, 0x40, 0x40, 0x80, 0x00, /* 0x29 */
  0x00, 0x00, 0x50, 0x20, 0xf0, 0x20, 0x50, 0x00, 0x00, 0x00, /* 0x2a */
  0x00, 0x00, 0x20, 0x20, 0xf0, 0x20, 0x20, 0x00, 0x00, 0x00, /* 0x2b */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x40, 0x00, /* 0x2c */
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2d */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, /* 0x2e */
  0x00, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x00, /* 0x2f */
  0x00, 0x60, 0x90, 0xb0, 0xd0, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x30 */
  0x00, 0x20, 0x60, 0xa0, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, /* 0x31 */
  0x00, 0x60, 0x90, 0x10, 0x20, 0x40, 0x80, 0xf0, 0x00, 0x00, /* 0x32 */
  0x00, 0xf0, 0x10, 0x20, 0x60, 0x10, 0x90, 0x60, 0x00, 0x00, /* 0x33 */
  0x00, 0x20, 0x60, 0xa0, 0xa0, 0xf0, 0x20, 0x20, 0x00, 0x00, /* 0x34 */
  0x00, 0xf0, 0x80, 0xe0, 0x10, 0x10, 0x90, 0x60, 0x00, 0x00, /* 0x35 */
  0x00, 0x60, 0x80, 0xe0, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x36 */
  0x00, 0xf0, 0x10, 0x20, 0x20, 0x40, 0x40, 0x40, 0x00, 0x00, /* 0x37 */
  0x00, 0x60, 0x90, 0x90, 0x60, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x38 */
  0x00, 0x60, 0x90, 0x90, 0x90, 0x70, 0x10, 0x60, 0x00, 0x00, /* 0x39 */
  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, /* 0x3a */
  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x40, 0x00, 0x00, /* 0x3b */
  0x00, 0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x00, 0x00, /* 0x3c */
  0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, /* 0x3d */
  0x00, 0x80, 0x40, 0x20, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, /* 0x3e */
  0x00, 0xe0, 0x10, 0x10, 0x20, 0x40, 0x00, 0x40, 0x00, 0x00, /* 0x3f */
  0x00, 0x60, 0x90, 0xb0, 0xa0, 0xb0, 0x80, 0x70, 0x00, 0x00, /* 0x40 */
  0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x41 */
  0x00, 0xe0, 0x90, 0x90, 0xe0, 0x90, 0x90, 0xe0, 0x00, 0x00, /* 0x42 */
  0x00, 0x60, 0x90, 0x80, 0x80, 0x80, 0x80, 0x70, 0x00, 0x00, /* 0x43 */
  0x00, 0xe0, 0x90, 0x90, 0x90, 0x90, 0x90, 0xe0, 0x00, 0x00, /* 0x44 */
  0x00, 0xf0, 0x80, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0x45 */
  0x00, 0xf0, 0x80, 0x80, 0xe0, 0x80, 0x80, 0x80, 0x00, 0x00, /* 0x46 */
  0x00, 0x70, 0x80, 0x80, 0xb0, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0x47 */
  0x00, 0x90, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x48 */
  0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0x49 */
  0x00, 0x10, 0x10, 0x10, 0x10, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x4a */
  0x00, 0x90, 0xa0, 0xc0, 0xc0, 0xa0, 0x90, 0x90, 0x00, 0x00, /* 0x4b */
  0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0x4c */
  0x00, 0x90, 0xb0, 0xd0, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x4d */
  0x00, 0x90, 0xd0, 0xb0, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x4e */
  0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x4f */
  0x00, 0xe0, 0x90, 0x90, 0x90, 0xe0, 0x80, 0x80, 0x00, 0x00, /* 0x50 */
  0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x40, 0x30, /* 0x51 */
  0x00, 0xe0, 0x90, 0x90, 0xe0, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x52 */
  0x00, 0x70, 0x80, 0x80, 0x60, 0x10, 0x10, 0xe0, 0x00, 0x00, /* 0x53 */
  0x00, 0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, /* 0x54 */
  0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x55 */
  0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0xa0, 0xc0, 0x00, 0x00, /* 0x56 */
  0x00, 0x90, 0x90, 0x90, 0x90, 0xd0, 0xb0, 0x90, 0x00, 0x00, /* 0x57 */
  0x00, 0x90, 0x90, 0x90, 0x60, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x58 */
  0x00, 0x90, 0x90, 0x90, 0xa0, 0x40, 0x40, 0x40, 0x00, 0x00, /* 0x59 */
  0x00, 0xf0, 0x10, 0x20, 0x40, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0x5a */
  0x70, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x70, 0x00, /* 0x5b */
  0x00, 0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x00, /* 0x5c */
  0xe0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xe0, 0x00, /* 0x5d */
  0x20, 0x50, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x5e */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, /* 0x5f */
  0x40, 0x40, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60 */
  0x00, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0x61 */
  0x00, 0x80, 0x80, 0xe0, 0x90, 0x90, 0x90, 0xe0, 0x00, 0x00, /* 0x62 */
  0x00, 0x00, 0x00, 0x60, 0x90, 0x80, 0x80, 0x70, 0x00, 0x00, /* 0x63 */
  0x00, 0x10, 0x10, 0x70, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0x64 */
  0x00, 0x00, 0x00, 0x60, 0x90, 0xf0, 0x80, 0x70, 0x00, 0x00, /* 0x65 */
  0x00, 0x30, 0x40, 0x40, 0xf0, 0x40, 0x40, 0x40, 0x00, 0x00, /* 0x66 */
  0x00, 0x00, 0x00, 0x70, 0x90, 0x90, 0x90, 0x70, 0x10, 0x60, /* 0x67 */
  0x00, 0x80, 0x80, 0xe0, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x68 */
  0x20, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0x69 */
  0x20, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0xc0, /* 0x6a */
  0x00, 0x80, 0x80, 0x90, 0xa0, 0xc0, 0xa0, 0x90, 0x00, 0x00, /* 0x6b */
  0x00, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00, 0x00, /* 0x6c */
  0x00, 0x00, 0x00, 0xf0, 0xa8, 0xa8, 0xa8, 0xa8, 0x00, 0x00, /* 0x6d */
  0x00, 0x00, 0x00, 0xe0, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0x6e */
  0x00, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0x6f */
  0x00, 0x00, 0x00, 0xe0, 0x90, 0x90, 0x90, 0xe0, 0x80, 0x80, /* 0x70 */
  0x00, 0x00, 0x00, 0x70, 0x90, 0x90, 0x90, 0x70, 0x10, 0x10, /* 0x71 */
  0x00, 0x00, 0x00, 0xb0, 0xc0, 0x80, 0x80, 0x80, 0x00, 0x00, /* 0x72 */
  0x00, 0x00, 0x00, 0x70, 0x80, 0x60, 0x10, 0xe0, 0x00, 0x00, /* 0x73 */
  0x00, 0x40, 0x40, 0xf0, 0x40, 0x40, 0x40, 0x30, 0x00, 0x00, /* 0x74 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0x75 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0xa0, 0xc0, 0x00, 0x00, /* 0x76 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0xd0, 0xb0, 0x90, 0x00, 0x00, /* 0x77 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0x60, 0x90, 0x90, 0x00, 0x00, /* 0x78 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x10, 0x60, /* 0x79 */
  0x00, 0x00, 0x00, 0xf0, 0x20, 0x40, 0x80, 0xf0, 0x00, 0x00, /* 0x7a */
  0x30, 0x40, 0x40, 0x40, 0x80, 0x40, 0x40, 0x40, 0x30, 0x00, /* 0x7b */
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, /* 0x7c */
  0xc0, 0x20, 0x20, 0x20, 0x10, 0x20, 0x20, 0x20, 0xc0, 0x00, /* 0x7d */
  0x00, 0x50, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x7e */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x81 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x82 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x83 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x84 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x85 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x86 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x87 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x89 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8a */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8b */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8c */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8d */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8e */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x8f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x91 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x92 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x93 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x94 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x95 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x96 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x97 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x99 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9a */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9b */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9c */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9d */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9e */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x9f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa0 */
  0x00, 0x20, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, /* 0xa1 */
  0x00, 0x20, 0x20, 0x70, 0xa0, 0xa0, 0xa0, 0x70, 0x20, 0x00, /* 0xa2 */
  0x00, 0x30, 0x40, 0x40, 0xf0, 0x40, 0x40, 0xb0, 0x00, 0x00, /* 0xa3 */
  0x00, 0x00, 0x90, 0x60, 0x60, 0x60, 0x90, 0x00, 0x00, 0x00, /* 0xa4 */
  0x00, 0x90, 0xa0, 0x40, 0xf0, 0x40, 0xf0, 0x40, 0x00, 0x00, /* 0xa5 */
  0x20, 0x20, 0x20, 0x20, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, /* 0xa6 */
  0x60, 0x90, 0x80, 0x60, 0x90, 0x60, 0x10, 0x90, 0x60, 0x00, /* 0xa7 */
  0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa8 */
  0x00, 0xf0, 0x90, 0x60, 0x40, 0x60, 0x90, 0xf0, 0x00, 0x00, /* 0xa9 */
  0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0xf0, 0x00, 0x00, /* 0xaa */
  0x00, 0x10, 0x20, 0x50, 0xa0, 0x50, 0x20, 0x10, 0x00, 0x00, /* 0xab */
  0x00, 0x00, 0x00, 0x00, 0xf0, 0x10, 0x10, 0x00, 0x00, 0x00, /* 0xac */
  0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xad */
  0x00, 0xf0, 0x90, 0x60, 0x40, 0x40, 0x90, 0xf0, 0x00, 0x00, /* 0xae */
  0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xaf */
  0x60, 0x90, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb0 */
  0x00, 0x40, 0x40, 0xf0, 0x40, 0x40, 0x00, 0xf0, 0x00, 0x00, /* 0xb1 */
  0xc0, 0x20, 0x40, 0x80, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb2 */
  0xe0, 0x20, 0x40, 0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb3 */
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb4 */
  0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0xb0, 0xd0, 0x80, 0x80, /* 0xb5 */
  0x00, 0xf0, 0xd0, 0xd0, 0xd0, 0x50, 0x50, 0x50, 0x00, 0x00, /* 0xb6 */
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb7 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x40, /* 0xb8 */
  0x40, 0xc0, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb9 */
  0x60, 0x90, 0x90, 0x60, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, /* 0xba */
  0x00, 0x80, 0x40, 0xa0, 0x50, 0xa0, 0x40, 0x80, 0x00, 0x00, /* 0xbb */
  0x40, 0xc0, 0x40, 0x40, 0x30, 0x50, 0x70, 0x10, 0x00, 0x00, /* 0xbc */
  0x40, 0xc0, 0x40, 0x40, 0x70, 0x10, 0x20, 0x70, 0x00, 0x00, /* 0xbd */
  0xe0, 0x40, 0x20, 0xc0, 0x30, 0x50, 0x70, 0x10, 0x00, 0x00, /* 0xbe */
  0x00, 0x20, 0x00, 0x20, 0x40, 0x80, 0x80, 0x70, 0x00, 0x00, /* 0xbf */
  0x20, 0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc0 */
  0x40, 0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc1 */
  0x90, 0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc2 */
  0xa0, 0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc3 */
  0x50, 0x00, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc4 */
  0x90, 0x90, 0x60, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00, 0x00, /* 0xc5 */
  0x00, 0xf0, 0xa0, 0xa0, 0xf0, 0xa0, 0xa0, 0xb0, 0x00, 0x00, /* 0xc6 */
  0x00, 0x60, 0x90, 0x80, 0x80, 0x80, 0x80, 0x70, 0x20, 0x40, /* 0xc7 */
  0x20, 0x00, 0xf0, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0xc8 */
  0x40, 0x00, 0xf0, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0xc9 */
  0x90, 0x00, 0xf0, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0xca */
  0x50, 0x00, 0xf0, 0x80, 0xe0, 0x80, 0x80, 0xf0, 0x00, 0x00, /* 0xcb */
  0x10, 0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xcc */
  0x20, 0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xcd */
  0x90, 0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xce */
  0x50, 0x00, 0x70, 0x20, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xcf */
  0x00, 0xe0, 0x50, 0x50, 0xf0, 0x50, 0x50, 0xe0, 0x00, 0x00, /* 0xd0 */
  0xa0, 0x00, 0x90, 0xd0, 0xb0, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0xd1 */
  0x20, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd2 */
  0x40, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd3 */
  0x90, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd4 */
  0xa0, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd5 */
  0x50, 0x00, 0x60, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd6 */
  0x00, 0x00, 0x90, 0x90, 0x60, 0x90, 0x90, 0x00, 0x00, 0x00, /* 0xd7 */
  0x10, 0x60, 0x90, 0xb0, 0xd0, 0x90, 0x90, 0x60, 0x80, 0x00, /* 0xd8 */
  0x20, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xd9 */
  0x40, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xda */
  0x90, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xdb */
  0x50, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xdc */
  0x40, 0x00, 0x90, 0x90, 0xa0, 0x40, 0x40, 0x40, 0x00, 0x00, /* 0xdd */
  0x80, 0xe0, 0x90, 0x90, 0x90, 0xe0, 0x80, 0x80, 0x00, 0x00, /* 0xde */
  0x00, 0x60, 0x90, 0x90, 0xe0, 0x90, 0x90, 0xe0, 0x80, 0x80, /* 0xdf */
  0x20, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe0 */
  0x40, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe1 */
  0x90, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe2 */
  0xa0, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe3 */
  0x50, 0x00, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe4 */
  0x90, 0x60, 0x00, 0x60, 0x10, 0x70, 0x90, 0x70, 0x00, 0x00, /* 0xe5 */
  0x00, 0x00, 0x00, 0xf0, 0x20, 0x70, 0xa0, 0xf0, 0x00, 0x00, /* 0xe6 */
  0x00, 0x00, 0x00, 0x60, 0x90, 0x80, 0x80, 0x70, 0x20, 0x40, /* 0xe7 */
  0x20, 0x00, 0x00, 0x60, 0x90, 0xf0, 0x80, 0x70, 0x00, 0x00, /* 0xe8 */
  0x40, 0x00, 0x00, 0x60, 0x90, 0xf0, 0x80, 0x70, 0x00, 0x00, /* 0xe9 */
  0x90, 0x00, 0x00, 0x60, 0x90, 0xf0, 0x80, 0x70, 0x00, 0x00, /* 0xea */
  0x50, 0x00, 0x00, 0x60, 0x90, 0xf0, 0x80, 0x70, 0x00, 0x00, /* 0xeb */
  0x10, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xec */
  0x20, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xed */
  0x90, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xee */
  0x50, 0x00, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00, 0x00, /* 0xef */
  0x70, 0xc0, 0x20, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf0 */
  0xa0, 0x00, 0x00, 0xe0, 0x90, 0x90, 0x90, 0x90, 0x00, 0x00, /* 0xf1 */
  0x20, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf2 */
  0x40, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf3 */
  0x90, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf4 */
  0xa0, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf5 */
  0x50, 0x00, 0x00, 0x60, 0x90, 0x90, 0x90, 0x60, 0x00, 0x00, /* 0xf6 */
  0x00, 0x40, 0x40, 0x00, 0xf0, 0x00, 0x40, 0x40, 0x00, 0x00, /* 0xf7 */
  0x00, 0x00, 0x10, 0x60, 0x90, 0xf0, 0x90, 0x60, 0x80, 0x00, /* 0xf8 */
  0x20, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0xf9 */
  0x40, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0xfa */
  0x90, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0xfb */
  0x50, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x00, 0x00, /* 0xfc */
  0x40, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x10, 0x60, /* 0xfd */
  0x00, 0x80, 0x80, 0xe0, 0x90, 0x90, 0x90, 0xe0, 0x80, 0x80, /* 0xfe */
  0x50, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x70, 0x10, 0x60, /* 0xff */
};

static VALUE wavspa_module;
static VALUE fb_klass;

static const char* opts_keys[] = {
  "column_step",
  "margin_x",
  "margin_y",
  "ceil",
  "floor",
};

static ID opts_ids[N(opts_keys)];

static void
rb_fb_mark(void* _ptr)
{
  rb_fb_t* ptr;

  ptr = (rb_fb_t*)_ptr;

  if (ptr->buf != Qnil) {
    rb_gc_mark(ptr->buf);
  }
}

static void
rb_fb_free(void* _ptr)
{
  ((rb_fb_t*)_ptr)->buf = Qnil;
}

static size_t
rb_fb_size(const void* _ptr)
{
  size_t ret;
  rb_fb_t* ptr;

  ptr = (rb_fb_t*)_ptr;
  ret = sizeof(*ptr) + ((ptr->buf != Qnil)? RSTRING_LEN(ptr->buf): 0);

  return ret;
}

static const struct rb_data_type_struct fb_data_type = {
  "Simple frame buffer for WAV file spectrum analyzer",
  {rb_fb_mark, rb_fb_free, rb_fb_size, {NULL, NULL}},
  NULL,
  NULL,
};

static VALUE
rb_fb_alloc(VALUE self)
{
  rb_fb_t* ptr;

  ptr = ALLOC(rb_fb_t);
  memset(ptr, 0, sizeof(*ptr));

  ptr->width    = -1;
  ptr->height   = -1;
  ptr->step     = 1;
  ptr->margin_x = 0;
  ptr->margin_y = 0;
  ptr->ceil     = -10.0;
  ptr->floor    = -90.0;
  ptr->range    = ptr->ceil - ptr->floor;
  ptr->size     = -1;
  ptr->buf      = Qnil;

  return TypedData_Wrap_Struct(fb_klass, &fb_data_type, ptr);
}

static VALUE
rb_fb_initialize(int argc, VALUE* argv, VALUE self)
{
  rb_fb_t* ptr;
  VALUE width;
  VALUE height;
  VALUE opt;
  VALUE opts[N(opts_ids)];

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  /*
   * parse argument
   */
  rb_scan_args(argc, argv, "21", &width, &height, &opt);

  /*
   * eval argument
   */
  Check_Type(width, T_FIXNUM);
  Check_Type(height, T_FIXNUM);

  if (FIX2INT(width) < 1 || FIX2INT(height) < 1){
    ARGUMENT_ERROR("too small");
  }

  if (opt != Qnil) {
    Check_Type(opt, T_HASH);
    rb_get_kwargs(opt, opts_ids, 0, N(opts_ids), opts);

    // :column_step
    if (opts[0] != Qundef) ptr->step = FIX2INT(opts[0]);

    // :margin_x
    if (opts[1] != Qundef) ptr->margin_x = FIX2INT(opts[1]);

    // :margin_y
    if (opts[2] != Qundef) ptr->margin_y = FIX2INT(opts[2]);

    // :ceil
    if (opts[3] != Qundef) ptr->ceil = NUM2DBL(opts[3]);

    // :floor
    if (opts[4] != Qundef) ptr->floor = NUM2DBL(opts[4]);
  }

  ptr->width  = FIX2INT(width);
  ptr->height = FIX2INT(height);

  ptr->stride = (ptr->margin_x + (ptr->width * ptr->step)) * 3;
  ptr->size   = ptr->stride * (ptr->height + ptr->margin_y);
  ptr->buf    = rb_str_buf_new(ptr->size);

  ptr->range  = ptr->ceil - ptr->floor;

  /*
   * clear buffer
   */
  rb_str_set_len(ptr->buf, ptr->size);
  memset(RSTRING_PTR(ptr->buf), 0, RSTRING_LEN(ptr->buf));

  return self;
}

static VALUE
rb_fb_width(VALUE self)
{
  rb_fb_t* ptr;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  return INT2FIX(ptr->margin_x + (ptr->width * ptr->step));
}

static VALUE
rb_fb_height(VALUE self)
{
  rb_fb_t* ptr;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  return INT2FIX(ptr->height + ptr->margin_y);
}

static VALUE
rb_fb_to_s(VALUE self)
{
  rb_fb_t* ptr;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  return ptr->buf;
}

static VALUE
rb_fb_draw_power(VALUE self, VALUE col, VALUE dat)
{
  rb_fb_t* ptr;
  uint8_t* p;
  uint8_t* src;
  double x;
  int v;
  int i;
  int j;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  /*
   * check argument
   */
  Check_Type(col, T_FIXNUM);
  Check_Type(dat, T_STRING);

  if (FIX2INT(col) < 0 || FIX2INT(col) >= ptr->width) {
    ARGUMENT_ERROR("invalid column number");
  }

  if (RSTRING_LEN(dat) != (int)((sizeof(double) * ptr->height))) {
    ARGUMENT_ERROR("invalid data length");
  }

  /*
   * put pixel data
   */
  p   = (uint8_t*)RSTRING_PTR(ptr->buf) +
              ((ptr->margin_x + (FIX2INT(col) * ptr->step)) * 3);
  src = (uint8_t*)RSTRING_PTR(dat) + (sizeof(double) * (ptr->height - 1));

  for (i = 0; i < ptr->height; i++) {
    memcpy(&x, src, sizeof(double));

    v = round(x * 3.5);

    if (v < 0) {
      v = 0;

    } else if (v > 255.0) {
      v = 255;
    }

    for (j = 0; j < ptr->step; j++) {
      p[0] = v / 3;
      p[1] = v;
      p[2] = v / 2;

      p += 3;
    }

    p   += (ptr->stride - (j * 3));
    src -= sizeof(double);
  }

  return self;
}

static VALUE
rb_fb_draw_amplitude(VALUE self, VALUE col, VALUE dat)
{
  rb_fb_t* ptr;
  uint8_t* p;
  uint8_t* src;
  double x;
  int v;
  int i;
  int j;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  /*
   * check argument
   */
  Check_Type(col, T_FIXNUM);
  Check_Type(dat, T_STRING);

  if (FIX2INT(col) < 0 || FIX2INT(col) >= ptr->width) {
    ARGUMENT_ERROR("invalid column number");
  }

  if (RSTRING_LEN(dat) != (int)((sizeof(double) * ptr->height))) {
    ARGUMENT_ERROR("invalid data length");
  }

  /*
   * put pixel data
   */
  p   = (uint8_t*)RSTRING_PTR(ptr->buf) +
              ((ptr->margin_x + (FIX2INT(col) * ptr->step)) * 3);
  src = (uint8_t*)RSTRING_PTR(dat) + (sizeof(double) * (ptr->height - 1));

  for (i = 0; i < ptr->height; i++) {
    memcpy(&x, src, sizeof(double));

    if (x >= ptr->ceil) {
      v = 255;

    } else if (x <= ptr->floor) {
      v = 0;

    } else {
      v = (uint8_t)((255.0 * (x - ptr->floor)) / ptr->range);
    }

    for (j = 0; j < ptr->step; j++) {
      p[0] = v / 3;
      p[1] = v;
      p[2] = v / 2;

      p += 3;
    }

    p   += (ptr->stride - (j * 3));
    src -= sizeof(double);
  }

  return self;
}

static void
put_string(rb_fb_t* ptr, int row, int col,
           VALUE rstr, uint8_t r, uint8_t g, uint8_t b)
{
  uint8_t* head;
  uint8_t* tail;
  uint8_t* p0;
  char* str;
  int len;
  int w;
  int h;

  uint8_t* p;
  const uint8_t* gl; // as Glyph
  int i;
  int j;
  int k;
  uint8_t m;

  head = (uint8_t*)RSTRING_PTR(ptr->buf);
  tail = head + RSTRING_LEN(ptr->buf);
  p0   = head + ((row * ptr->stride) + (col * 3));
  str  = RSTRING_PTR(rstr);
  len  = RSTRING_LEN(rstr);
  w    = ptr->width + ptr->margin_x;
  h    = ptr->height + ptr->margin_y;

  for (i = 0; i < len; i++) {
    p  = p0 + (i * 6 * 3);
    gl = font + (str[i] * 10);

    for (j = 0; j < 10; j++) {
      for (k = 0; k < 5; k++) {
        do {
          if ((col + k <  0) || (row + j <  0)) break;
          if ((col + k >= w) || (row + j >= h)) break;
          if (!(gl[j] & (0x80 >> k))) break;

          p[0] = r;
          p[1] = g;
          p[2] = b;
        } while (0);

        p += 3;
      }

      p += (ptr->stride - (5 * 3));
    }

    col += 6;
  }
}

static VALUE
rb_fb_hline(VALUE self, VALUE row, VALUE label)
{
  rb_fb_t* ptr;
  uint8_t* p;
  int i;
  int j;
  int w;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  /*
   * check argument
   */
  Check_Type(row, T_FIXNUM);
  Check_Type(label, T_STRING);

  if (FIX2INT(row) < 0 || FIX2INT(row) >= ptr->height) {
    ARGUMENT_ERROR("invalid column number");
  }

  /*
   * put line
   */
  p = (uint8_t*)RSTRING_PTR(ptr->buf) + (FIX2INT(row) * ptr->stride);
  w = ptr->margin_x + (ptr->width * ptr->step);

  for (i = 0; i < w; i++) {
    for (j = 0; j < ptr->step; j++) {
      int v;
      v = p[0] + 0xff;
      p[0] = (v <= 0xff)? v: 0xff;

      v = p[1] + 0x00;
      p[1] = (v <= 0xff)? v: 0xff;

      v = p[2] + 0x00;
      p[2] = (v <= 0xff)? v: 0xff;
    }

    p += 3;
  }

  /*
   * put label
   */
  put_string(ptr, FIX2INT(row) - 11, 4, label, 0xff, 0x00, 0x00);

  return self;
}

static VALUE
rb_fb_vline(VALUE self, VALUE col, VALUE label)
{
  rb_fb_t* ptr;
  uint8_t* p;
  int i;
  int h;

  /*
   * extract context data
   */
  TypedData_Get_Struct(self, rb_fb_t, &fb_data_type, ptr);

  /*
   * check argument
   */
  Check_Type(col, T_FIXNUM);
  Check_Type(label, T_STRING);

  if (FIX2INT(col) < 0 || FIX2INT(col) >= ptr->width) {
    ARGUMENT_ERROR("invalid column number");
  }

  /*
   * put line
   */
  p = (uint8_t*)RSTRING_PTR(ptr->buf) +
          ((ptr->margin_x + (FIX2INT(col) * ptr->step)) * 3);
  h = ptr->margin_y + ptr->height;

  for (i = 0; i < h; i++) {
    int v;
    v = p[0] + 0x40;
    p[0] = (v <= 0xff)? v: 0xff;

    v = p[1] + 0x40;
    p[1] = (v <= 0xff)? v: 0xff;

    v = p[2] + 0xff;
    p[2] = (v <= 0xff)? v: 0xff;

    p += ptr->stride;
  }

  /*
   * put label
   */
  put_string(ptr,
             ptr->height + 14,
             ptr->margin_x + (FIX2INT(col) * ptr->step) + 4,
             label,
             0x80,
             0x80,
             0xff);

  return self;
}

void
Init_fb()
{
  int i;

  wavspa_module = rb_define_module("WavSpectrumAnalyzer");
  fb_klass      = rb_define_class_under(wavspa_module,
                                        "FrameBuffer", rb_cObject);

  rb_define_alloc_func(fb_klass, rb_fb_alloc);
  rb_define_method(fb_klass, "initialize", rb_fb_initialize, -1);
  rb_define_method(fb_klass, "width", rb_fb_width, 0);
  rb_define_method(fb_klass, "height", rb_fb_height, 0);
  rb_define_method(fb_klass, "to_s", rb_fb_to_s, 0);
  rb_define_method(fb_klass, "draw_power", rb_fb_draw_power, 2);
  rb_define_method(fb_klass, "draw_amplitude", rb_fb_draw_amplitude, 2);
  rb_define_method(fb_klass, "hline", rb_fb_hline, 2);
  rb_define_method(fb_klass, "vline", rb_fb_vline, 2);

  for (i = 0; i < (int)N(opts_keys); i++) {
    opts_ids[i] = rb_intern_const(opts_keys[i]);
  }
}
