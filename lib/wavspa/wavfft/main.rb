#! /usr/bin/env ruby
# coding: utf-8

#
# Spectrum analyzer for WAV file
#
#   Copyright (C) 2019 Hiroshi Kuwagata <kgt9221@gmail.com>
#

require 'wav'
require 'png'

require 'wavspa/fft'
require 'wavspa/fb'

module WavSpectrumAnalyzer
  module FFTApp
    extend WavSpectrumAnalyzer::Common

    class << self
      def load_param(param)
        @unit_time    = param[:unit_time]
        @fft_size     = param[:fft_size]
        @output_width = param[:output_width]
        @logscale     = (param[:scale_mode] == :LOGSCALE)
        @win_func     = param[:window_function]
        @scale_mode   = param[:scale_mode]
        @col_step     = param[:col_step]

        @freq_range   = param[:range]
        @lo_freq      = @freq_range[0]
        @hi_freq      = @freq_range[1]
        @freq_width   = (@hi_freq - @lo_freq)

        @log_step     = (@hi_freq / @lo_freq) ** (1.0 / @output_width)
        @log_base     = Math.log(@log_step)
        @basis_freq   = 440.0
      end
      private :load_param

      def main(input, param, output)
        load_param(param)

        wav = WavFile.open(input)
        fft = FFT.new("s%dle" % wav.sample_size, @fft_size)

        fft.window     = @win_func
        fft.width      = @output_width
        fft.scale_mode = @scale_mode
        fft.frequency  = @freq_range.clone.unshift(wav.sample_rate)

        line  = []
        rows  = 0
        usize = (wav.sample_rate / 100) * @unit_time
        nblk  = (wav.data_size / (wav.sample_size / 8)) / usize

        fb    = FrameBuffer.new(nblk,
                                @output_width,
                                :column_step => @col_step,
                                :margin_x => ($draw_freq_line)? 50:0,
                                :margin_y => ($draw_time_line)? 30:0)

        if $verbose
          STDERR.print <<~EOT
            - input data
              #{input}
                data size:   #{wav.data_size}
                channel num: #{wav.channel_num}
                sample rate: #{wav.sample_rate} Hz
                sample size: #{wav.sample_size} bits
                data rate:   #{wav.bytes_per_sec} bytes/sec

            - FFT parameter
                FFT size:    #{@fft_size} samples
                unit time:   #{@unit_time} cs
                window func: #{@win_func}

            - OUTPUT
                width:       #{fb.width}px
                height:      #{fb.height}px
                freq range:  #{@lo_freq} - #{@hi_freq}Hz
                scale mode:  #{@scale_mode}

          EOT
        end
       
        until rows >= nblk
          STDERR.printf("\rtransform #{rows + 1}/#{nblk}", rows) if $verbose

          fft << wav.read(usize)

          fft.spectrum.unpack("d*").each {|x|
            begin
              x = (x * 3.5).round 
              x = 0 if x < 0 
              x = 255 if x > 255
            rescue
              x = 0
            end

            line.unshift(x / 3, x, x / 2)
          }

          fb.put_column(rows, line)
          line.clear

          rows += 1
        end

        STDERR.printf(" ... done\n") if $verbose

        STDERR.printf("write to #{output} ... ") if $verbose

        draw_freq_line(fb) if $draw_freq_line
        draw_time_line(fb, wav, usize) if $draw_time_line

        png = PNG.encode(fb.width, fb.height, fb.to_s, :pixel_format => :RGB)
        IO.binwrite(output, png)

        STDERR.printf("done\n") if $verbose
      end
    end
  end
end
