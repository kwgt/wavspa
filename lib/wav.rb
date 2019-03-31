#!/usr/bin/env ruby
#coding utf-8

#
# WAV file parser
#
#   Copyright (C) 2014 Hiroshi Kuwagata <kgt9221@gmail.com>
#

class WavFile
  class << self
    alias :open :new
  end

  def initialize(file)
    @io = File.open(file, "rb")
    
    tmp = @io.read(12).unpack('a4Va4')

    @magic         = tmp[0]
    @data_size     = tmp[1]
    @type          = tmp[2]

    if not (@magic == "RIFF" and @type == "WAVE")
      raise('Illeagal magic number.')
    end

    if @data_size != (File.size(file) - 8)
      raise('Data size is not match.')
    end

    tmp = @io.read(8).unpack('a4V')

    @id            = tmp[0]

    if @id != 'fmt '
      raise('ID "%s" is not supported.' % @id)
    end

    tmp = @io.read(tmp[1]).unpack('vvVVvv')

    @format_id     = tmp[0]
    @channel_num   = tmp[1]
    @sample_rate   = tmp[2]
    @bytes_per_sec = tmp[3]
    @block_size    = tmp[4]
    @sample_size   = tmp[5]

    loop {
      tmp = @io.read(8).unpack('a4V')
      break if tmp[0] == "data"

      @io.seek(tmp[1], IO::SEEK_CUR)
    }

    @data_offset   = @io.pos
    @data_size     = tmp[1]
  end

  attr_reader :data_size, :channel_num, :sample_rate, :sample_size,
              :bytes_per_sec, :block_size

  def read(n = nil)
    if not n.nil?
      @io.read(@block_size * n)
    else
      @io.read
    end
  end

  def seek(pos)
    @io.seek(@block_size * pos, IO::SEEK_SET)
  end

  def eof?
    @io.eof?
  end

  def close
    @io.close
  end
end

if $0 == __FILE__
  require 'pp'
  pp WavFile.open(ARGV[0])
end
