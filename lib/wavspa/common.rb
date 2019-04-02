require "wavspa/version"

module WavSpectrumAnalyzer
  module Common
    def log_scaler(r)
      return 1.0 - ((Math.log(r / @lo_freq) / @log_base) / @output_width)
    end

    def linear_scaler(r)
      return 1.0 - ((r - @lo_freq) / @freq_width)
    end

    def fpos(r)
      scale = (@logscale)? log_scaler(r): linear_scaler(r)
      return (scale * @output_width).round
    end

    def draw_freq_line(fb)
      #
      # 低周波方向
      #
      frq = @basis_freq / 2
      loop {
        pos  = fpos(frq)
        break if pos >= @output_width

        fb.hline(pos, "#{frq.to_i}Hz")
        frq /= 2
      }
     
      #
      # 高周波方向
      #
      frq = @basis_freq;
      loop {
        pos = fpos(frq)
        break if pos <= 0 

        fb.hline(pos, "#{frq.to_i}Hz")
        frq *= 2
      }
    end

    def time_str(tm)
      m = tm / 60
      s = tm % 60

      if m.zero?
        ret = %q{%d"} % s
      else
        ret = %q{%d'%02d"} % [m, s]
      end

      return ret
    end

    def draw_time_line(fb, wav, usize)
      tc = 0
      tm = 0
      n  = (wav.data_size / (wav.sample_size / 8)) / usize

      fb.vline(0, time_str(tm))

      n.times { |col|
        tc += usize;
        if tc >= wav.sample_rate
          tm += 1
          tc %= wav.sample_rate
          fb.vline(col, time_str(tm)) if (tm % 10).zero?
        end
      }
    end
  end
end
