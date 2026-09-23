require 'fileutils'

MRuby::Gem::Specification.new('mruby-uri-parser') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'WHATWG-compliant URI parser for mruby (ada-url)'
  spec.add_conflict 'mruby-uri'
  spec.add_dependency 'mruby-c-ext-helpers'

  # A build that already asks for C++20 or later keeps its -std: the
  # last -std on the line wins, and a later one here would take away
  # what the build chose, -freflection's C++26 among it.
  cxx20_or_later = spec.cxx.flags.flatten.any? do |flag|
    version = flag.to_s[%r{\A[-/]std[:=](?:c|gnu)\+\+(\w+)\z}, 1]
    version == 'latest' || version.to_s.match?(/\A2[0-9a-z]\z/)
  end
  spec.cxx.flags << (spec.for_windows? ? '/std:c++20' : '-std=c++20') unless cxx20_or_later
end
