MRuby::Gem::Specification.new('mruby-uri-parser') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'URI parser for mruby'
  spec.add_dependency 'mruby-errno'
  spec.add_conflict 'mruby-uri'

  if (/mswin|mingw|win32/ =~ RUBY_PLATFORM) then
    spec.linker.libraries << 'wsock32'
    spec.linker.libraries << 'ws2_32'
  end
end
