require 'mkmf'
require 'optparse'
require 'rbconfig'

omp_enable = true
omp_path   = nil
omp_name   = "gomp"

OptionParser.new { |opt|
  opt.on("--[no-]openmp") { |flag|
    omp_enable = flag
  }

  opt.on("--with-openmp=PATH", String) { |path|
    omp_enable = true
    omp_path   = path
  }

  opt.on("--omp-name=NAME", String) { |name|
    omp_name = name
  }

  opt.parse!(ARGV)
}

if omp_enable
  $CFLAGS << " -fopenmp"

  if omp_path
    $CFLAGS << " -L#{omp_path}"
    $LDFLAGS << " -L#{omp_path}"

    case RbConfig::CONFIG['arch']
    when /-darwin/
      $LDFLAGS << " -Wl,-rpath,#{omp_path}"

    else
      # nothing
    end
  end

  have_library(omp_name)
end

have_library("m")

create_makefile( "wavspa/wavelet")
