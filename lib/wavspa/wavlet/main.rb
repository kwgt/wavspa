#! /usr/bin/env ruby
# coding: utf-8

#
# Spectrum analyzer for WAV file
#
#   Copyright (C) 2019 Hiroshi Kuwagata <kgt9221@gmail.com>
#

require 'wav'
require 'png'

require 'wavspa/wavelet'
require 'wavspa/fb'

module WavSpectrumAnalyzer
  module WaveLetApp
    extend WavSpectrumAnalyzer::Common

    class << self
      def load_param(param)
        @unit_time    = param[:unit_time]
        @sigma        = param[:sigma]
        @ceil         = param[:ceil]
        @floor        = param[:floor]
        @tran_mode    = param[:transform_mode]
        @output_width = param[:output_width]
        @logscale     = (param[:scale_mode] == :LOGSCALE)
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
        wl  = Wavelet.new

        wl.frequency  = wav.sample_rate
        wl.sigma      = @sigma
        wl.range      = (@lo_freq .. @hi_freq)
        wl.scale_mode = @scale_mode
        wl.width      = @output_width

        wl.put_in("s%dle" % wav.sample_size, wav.read)

        line  = []
        rows  = 0

        usize = (wav.sample_rate * @unit_time) / 1000
        nblk  = (wav.data_size / wav.block_size) / usize

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
                sigma:       #{@sigma}
                unit time:   #{@unit_time} ms
                ceil:        #{@ceil}
                floor:       #{@floor}
                mode :       #{@tran_mode}

            - OUTPUT
                width:       #{fb.width}px
                height:      #{fb.height}px
                freq range:  #{@lo_freq} - #{@hi_freq}Hz
                scale mode:  #{@scale_mode}

          EOT
        end

        if wav.channel_num >= 2
          STDERR.print("error: multi chanel data is not supported " \
                       "(support only monoral data).\n")
          exit(1)
        end
       
        until rows >= nblk
          STDERR.printf("\rtransform #{rows + 1}/#{nblk}", rows) if $verbose
          wl.transform(rows * usize)

          result = (@transform_mode == :POWER)? wl.power: wl.amplitude

          result.unpack("d*").each {|x|
            if x > @ceil
              x = 255
            elsif x < @floor
              x = 0
            else
              x = ((255 * (x - @floor)) / (@ceil - @floor)).floor
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
