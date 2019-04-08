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
        @transform_mode = param[:transform_mode]
        @unit_time      = param[:unit_time]
        @fft_size       = param[:fft_size]
        @output_width   = param[:output_width]
        @win_func       = param[:window_function]
                       
        @freq_range     = param[:range]
        @ceil           = param[:ceil]
        @floor          = param[:floor]
        @col_step       = param[:col_step]
                       
        @scale_mode     = param[:scale_mode]
        @logscale       = (param[:scale_mode] == :LOGSCALE)
        @basis_freq     = param[:basis_freq] || 440.0 
        @grid_step      = param[:grid_step] || ((@logscale)? 2.0: 2000.0)
                       
        @lo_freq        = @freq_range[0]
        @hi_freq        = @freq_range[1]
        @freq_width     = (@hi_freq - @lo_freq)
                       
        @log_step       = (@hi_freq / @lo_freq) ** (1.0 / @output_width)
        @log_base       = Math.log(@log_step)
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

        rows  = 0
        usize = (wav.sample_rate / 100) * @unit_time
        nblk  = (wav.data_size / (wav.sample_size / 8)) / usize

        fb    = FrameBuffer.new(nblk,
                                @output_width,
                                :column_step => @col_step,
                                :margin_x => ($draw_freq_line)? 50:0,
                                :margin_y => ($draw_time_line)? 30:0,
                                :ceil => @ceil,
                                :floor => @floor)

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
                plot mode :  #{@transform_mode}
                ceil:        #{@ceil}
                floor:       #{@floor}

          EOT
        end

        if wav.channel_num >= 2
          error("error: multi chanel data is not supported " \
                "(support only monoral data).")
        end
       
        until rows >= nblk
          STDERR.printf("\rtransform #{rows + 1}/#{nblk}", rows) if $verbose

          fft << wav.read(usize)

          if @transform_mode == :POWER
            fb.draw_power(rows, fft.power)
          else
            fb.draw_amplitude(rows, fft.amplitude)
          end

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
