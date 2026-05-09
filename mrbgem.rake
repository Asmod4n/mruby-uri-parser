require 'fileutils'

MRuby::Gem::Specification.new('mruby-uri-parser') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'WHATWG-compliant URI parser for mruby (ada-url)'
  spec.add_conflict 'mruby-uri'
  spec.add_dependency 'mruby-c-ext-helpers'

  msvc       = spec.for_windows?
  cxx20_flag = msvc ? '/std:c++20' : '-std=c++20'
  spec.cxx.flags << cxx20_flag unless spec.cxx.flags.flatten.include?(cxx20_flag)
end
