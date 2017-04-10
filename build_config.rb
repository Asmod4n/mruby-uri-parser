MRuby::Build.new do |conf|
  toolchain :gcc
  enable_debug
  conf.cc.defines = %w(ENABLE_DEBUG)
  conf.enable_test
  conf.gembox 'full-core'
  conf.cc.flags << '-fsanitize=address'
  conf.linker.flags << '-fsanitize=address'
  conf.gem File.expand_path(File.dirname(__FILE__))
end
