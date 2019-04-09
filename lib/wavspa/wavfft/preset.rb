#! /usr/bin/env ruby
# coding: utf-8

#
# Spectrum analyzer for WAV file
#
#   Copyright (C) 2019 Hiroshi Kuwagata <kgt9221@gmail.com>
#

module WavSpectrumAnalyzer
  module FFTApp
    PRESET_TABLE = {
      "default" => {
        :unit_time       => 10,
        :fft_size        => 16384,
        :output_width    => 240,
        :window_function => :FLAT_TOP,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :range           => [200, 8000],
        :ceil            => -10.0,
        :floor           => -90.0,
        :luminance       => 3.5,
        :col_step        => 1,
      },

      "32k" => {
        :unit_time       => 5,
        :fft_size        => 32768,
        :output_width    => 360,
        :window_function => :FLAT_TOP,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :range           => [200, 16000],
        :ceil            => -10.0,
        :floor           => -90.0,
        :luminance       => 3.5,
        :col_step        => 1,
      },

      "cd" => {
        :unit_time       => 5,
        :fft_size        => 32768,
        :output_width    => 480,
        :window_function => :FLAT_TOP,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :range           => [50, 22000],
        :ceil            => -10.0,
        :floor           => -90.0,
        :luminance       => 3.5,
        :col_step        => 1,
      },

      "highreso" => {
        :unit_time       => 5,
        :fft_size        => 131072,
        :window_function => :FLAT_TOP,
        :output_width    => 640,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :range           => [200, 32000],
        :ceil            => -10.0,
        :floor           => -90.0,
        :luminance       => 3.5,
        :col_step        => 1,
      },
    }
  end
end
