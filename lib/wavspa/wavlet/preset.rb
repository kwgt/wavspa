#! /usr/bin/env ruby
# coding: utf-8

#
# Spectrum analyzer for WAV file
#
#   Copyright (C) 2019 Hiroshi Kuwagata <kgt9221@gmail.com>
#

module WavSpectrumAnalyzer
  module WaveLetApp
    PRESET_TABLE = {
      "default" => {
        :sigma           => 24.0,
        :unit_time       => 10 * 10,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :output_width    => 240,
        :range           => [200.0, 8000.0],
        :ceil            => -10.0,
        :floor           => -90.0,
        :col_step        => 1,
      },

      "cd" => {
        :sigma           => 24.0,
        :unit_time       => 5 * 10,
        :scale_mode      => :LOGSCALE,
        :transform_mode  => :POWER,
        :output_width    => 480,
        :range           => [50.0, 22050.0],
        :ceil            => -10.0,
        :floor           => -90.0,
        :col_step        => 1,
      },
    }
  end
end
