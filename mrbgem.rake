require 'fileutils'
require 'shellwords'

MRuby::Gem::Specification.new('mruby-uri-parser') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'WHATWG-compliant URI parser for mruby (ada-url)'
  spec.add_conflict 'mruby-uri'
  spec.add_dependency 'mruby-c-ext-helpers'

  msvc     = build.toolchains.include?('visualcpp')
  cxx20_flag = msvc ? '/std:c++20' : '-std=c++20'
  spec.cxx.flags << cxx20_flag unless spec.cxx.flags.include?(cxx20_flag)

  ada_inc = File.join(spec.build_dir, 'include')

  lib_candidates = if msvc
    ['lib', 'lib64'].map { |d| File.join(spec.build_dir, d, 'ada.lib') }
  else
    ['lib', 'lib64'].map { |d| File.join(spec.build_dir, d, 'libada.a') }
  end

  ada_lib = lib_candidates.find { |f| File.exist?(f) }

  unless ada_lib
    warn 'mruby-uri-parser: cannot find libada, building it'

    archiver    = spec.respond_to?(:archiver) && spec.archiver.respond_to?(:command) ?
                  spec.archiver.command : nil
    linker_flags = (spec.linker.flags || []).join(' ')

    cmake_opts  = "-DADA_TESTING=OFF " \
                  "-DADA_TOOLS=OFF " \
                  "-DBUILD_SHARED_LIBS=OFF " \
                  "-DCMAKE_BUILD_TYPE=Release " \
                  "-DCMAKE_INSTALL_PREFIX=#{spec.build_dir.shellescape} " \
                  "-DCMAKE_CXX_COMPILER=#{spec.cxx.command.shellescape} " \
                  "-DCMAKE_CXX_FLAGS=#{spec.cxx.flags.join(' ').shellescape} " \
                  "-DCMAKE_C_COMPILER=#{spec.cc.command.shellescape} " \
                  "-DCMAKE_C_FLAGS=#{spec.cc.flags.join(' ').shellescape} "

    cmake_opts << "-DCMAKE_CXX_COMPILER_AR=#{archiver.shellescape} " if archiver
    cmake_opts << "-DCMAKE_C_COMPILER_AR=#{archiver.shellescape} "   if archiver
    cmake_opts << "-DCMAKE_LINKER=#{spec.linker.command.shellescape} " if spec.linker.command
    cmake_opts << "-DCMAKE_MODULE_LINKER_FLAGS=#{linker_flags.shellescape} " unless linker_flags.empty?

    if build.is_a?(MRuby::CrossBuild)
      cmake_opts << "-DCMAKE_SYSTEM_NAME=#{build.build_target.shellescape} " \
                    "-DCMAKE_HOST_SYSTEM_NAME=#{build.host_target.shellescape} "
    end

    build_dir = File.join(spec.build_dir, 'build')
    FileUtils.mkdir_p(build_dir)
    Dir.chdir(build_dir) do
      sh "cmake #{cmake_opts} #{File.join(spec.dir, 'deps', 'ada').shellescape}"
      sh "cmake --build . --config Release --target install"
    end

    ada_lib = lib_candidates.find { |f| File.exist?(f) } or
      raise "libada not found after build in #{spec.build_dir}"
  end

  [spec.cc, spec.cxx].each { |c| c.include_paths << ada_inc }
  spec.linker.flags_after_libraries << ada_lib
end
