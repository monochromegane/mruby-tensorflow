MRuby::Gem::Specification.new('mruby-tensorflow') do |spec|
  spec.license = 'MIT'
  spec.authors = 'monochromegane'

  require 'open3'

  tensorflow_dir = "#{build_dir}/tensorflow"
  tensorflow_make_dir = "#{tensorflow_dir}/tensorflow/contrib/makefile"

  def run_command env, command
    STDOUT.sync = true
    puts "build: [exec] #{command}"
    Open3.popen2e(env, command) do |stdin, stdout, thread|
      print stdout.read
      fail "#{command} failed" if thread.value != 0
    end
  end

  FileUtils.mkdir_p build_dir

  unless File.exist? tensorflow_dir
    Dir.chdir(build_dir) do
      e = {}
      run_command e, 'git clone --recurse-submodules https://github.com/tensorflow/tensorflow.git'
      run_command e, "git --git-dir=#{tensorflow_dir}/.git --work-tree=#{tensorflow_dir} checkout v1.0.0"
      if `uname` =~ /Darwin/
        # Use host protoc
        run_command e, "ruby -i -pe '$_.gsub!(\"LDFLAGS += -all_load\", \"LDFLAGS += -all_load\nifeq ($(HAS_GEN_HOST_PROTOC),true)\n        LIBFLAGS += -L$(MAKEFILE_DIR)/gen/protobuf-host/lib\nendif\")' #{tensorflow_make_dir}/Makefile"
      end
    end
  end

  unless File.exist? "#{tensorflow_make_dir}/gen/lib/libtensorflow-core.a"
    Dir.chdir(tensorflow_make_dir) do
      e = {}
      run_command e, './build_all_linux.sh'
    end
  end

  spec.cxx.flags << "-std=c++11"

  if `uname` =~ /Darwin/
    spec.linker.flags << "-all_load"
  else
    spec.linker.flags << "-Wl,--allow-multiple-definition"
    spec.linker.flags << "-Wl,--whole-archive"
  end

  spec.cxx.include_paths << tensorflow_dir
  spec.cxx.include_paths << "#{tensorflow_make_dir}/downloads/eigen"
  spec.cxx.include_paths << "#{tensorflow_make_dir}/gen/proto"
  spec.cxx.include_paths << "#{tensorflow_make_dir}/gen/protobuf/include"

  spec.linker.flags_before_libraries << "#{tensorflow_make_dir}/gen/lib/libtensorflow-core.a"
  spec.linker.flags_before_libraries << "#{tensorflow_make_dir}/gen/protobuf/lib/libprotobuf.a"
  spec.linker.flags_before_libraries << "#{tensorflow_make_dir}/gen/protobuf/lib/libprotoc.a"

  spec.add_dependency "mruby-io", :github => 'iij/mruby-io'
end
