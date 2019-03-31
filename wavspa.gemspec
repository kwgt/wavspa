
lib = File.expand_path("../lib", __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require "wavspa/version"

Gem::Specification.new do |spec|
  spec.name          = "wavspa"
  spec.version       = WavSpectrumAnalyzer::VERSION
  spec.authors       = ["Hirosho Kuwagata"]
  spec.email         = ["kgt9221@gmail.com"]

  spec.summary       = %q{spectrum analyzer for wav file.}
  spec.description   = %q{spectrum analyzer for wav file.}
  spec.homepage      = "https://github.com/kwgt/wavspa"
  spec.license       = "MIT"

  if spec.respond_to?(:metadata)
    spec.metadata["homepage_uri"] = spec.homepage
  else
    raise "RubyGems 2.0 or newer is required to protect against " \
      "public gem pushes."
  end

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been
  # added into git.
  spec.files         = Dir.chdir(File.expand_path('..', __FILE__)) do
    `git ls-files -z`.split("\x0").reject { |f|
      f.match(%r{^(test|spec|features)/})
    }
  end

  spec.bindir        = "bin"
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.require_paths = ["none"]
  spec.extensions    = %w[
    ext/wavspa/fft/extconf.rb
    ext/wavspa/wavelet/extconf.rb
    ext/wavspa/fb/extconf.rb
  ]

  spec.required_ruby_version = ">= 2.4.0"

  spec.add_development_dependency "bundler", "~> 2.0"
  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_dependency "libpng-ruby", "~> 0.5.2"
end
