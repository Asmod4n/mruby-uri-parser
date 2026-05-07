require 'fileutils'

MRuby::Gem::Specification.new('mruby-uri-parser') do |spec|
  spec.license = 'Apache-2'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'WHATWG-compliant URI parser for mruby (ada-url)'
  spec.add_conflict 'mruby-uri'
  spec.add_dependency 'mruby-c-ext-helpers'

  msvc       = build.for_windows?
  cxx20_flag = msvc ? '/std:c++20' : '-std=c++20'
  spec.cxx.flags << cxx20_flag unless spec.cxx.flags.flatten.include?(cxx20_flag)

  build_type = build.cc.defines.flatten.include?('MRB_DEBUG') ? 'Debug' : 'Release'

  ada_inc = File.join(spec.build_dir, 'include')
  lib_candidates = if msvc
    %w[lib lib64].map { |d| File.join(spec.build_dir, d, 'ada.lib') }
  else
    %w[lib lib64].map { |d| File.join(spec.build_dir, d, 'libada.a') }
  end
  ada_lib = lib_candidates.find { |f| File.exist?(f) }

  unless ada_lib
    warn "mruby-uri-parser: cannot find libada, building it (#{build_type})"

    cmake_args = [
      '-G', 'Ninja',
      "-DCMAKE_BUILD_TYPE=#{build_type}",
      '-DBUILD_SHARED_LIBS=OFF',
      '-DADA_TESTING=OFF',
      '-DADA_TOOLS=OFF',
      "-DCMAKE_INSTALL_PREFIX=#{spec.build_dir}",
      "-DCMAKE_C_COMPILER=#{spec.cc.command}",
      "-DCMAKE_CXX_COMPILER=#{spec.cxx.command}",
    ]

    if build.is_a?(MRuby::CrossBuild)
      cmake_args << "-DCMAKE_SYSTEM_NAME=#{build.build_target}"
      cmake_args << "-DCMAKE_HOST_SYSTEM_NAME=#{build.host_target}"
    end

    src_dir   = File.join(spec.dir, 'deps', 'ada')
    cmake_dir = File.join(spec.build_dir, 'build')
    FileUtils.mkdir_p(cmake_dir)

    Dir.chdir(cmake_dir) do
      sh 'cmake', *cmake_args, '-S', src_dir, '-B', '.'
      sh 'cmake', '--build', '.', '--target', 'install'
    end

    ada_lib = lib_candidates.find { |f| File.exist?(f) } or
      raise "libada not found after build in #{spec.build_dir}"
  end

  [spec.cc, spec.cxx].each { |c| c.include_paths << ada_inc }
  spec.linker.flags_after_libraries << ada_lib
end
