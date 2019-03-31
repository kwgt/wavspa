require 'mkmf'

$CFLAGS="-DUSE_CDFT_PTHREADS -DCDFT_THREADS_BEGIN_N=512 -DCDFT_4THREADS_BEGIN_N=1024"

have_library( "m")
create_makefile( "wavspa/fft")
