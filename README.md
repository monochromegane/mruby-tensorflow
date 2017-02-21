# mruby-tensorflow   [![Build Status](https://travis-ci.org/monochromegane/mruby-tensorflow.svg?branch=master)](https://travis-ci.org/monochromegane/mruby-tensorflow)

TensorFlow class for mruby.

[TensorFlow](https://www.tensorflow.org/) is a library for Machine Intelligence. This mrbgem provides mruby bindings for the library.

## example

```ruby
a, x, b = 1.0, 2.0, 3.0
a_tensor = TensorFlow::Tensor.new([a], [1])
x_tensor = TensorFlow::Tensor.new([x], [1])
b_tensor = TensorFlow::Tensor.new([b], [1])

# Load linear fucntion graph that provides 'y = ax + b'.
graph = TensorFlow::GraphDef.new
graph.load('test/linear_function_graph.pb')
session = TensorFlow::Session.new
session.create(graph)

tensors = session.run([['a:0', a_tensor], ['x:0', x_tensor], ['b:0', b_tensor]], ['y:0'])
session.close

p tensors[0].flat[0]
#=> 5.0
```

### ngx_mruby

```rb
# mruby_init
userdata = Userdata.new "tensorflow_data_key"
graph = TensorFlow::GraphDef.new
graph.load('test/linear_function_graph.pb')
userdata.graph = graph
```

```rb
# mruby_content_handler
class Linear
  def call(env)
    userdata = Userdata.new "tensorflow_data_key"
    graph = userdata.graph
    return not_found unless graph

    session = TensorFlow::Session.new
    session.create(graph)

    params = env['QUERY_STRING'].split('&').map {|kv| kv.split('=') }.to_h

    a_tensor = TensorFlow::Tensor.new([params['a']], [1])
    x_tensor = TensorFlow::Tensor.new([params['x']], [1])
    b_tensor = TensorFlow::Tensor.new([params['b']], [1])

    tensors = session.run([['a:0', a_tensor], ['x:0', x_tensor], ['b:0', b_tensor]], ['y:0'])

    [200, { "Content-Type" => "application/json;charset=utf-8" }, [tensors[0].flat]]
  ensure
    session.close if session
  end

  private

  def not_found
    return [404, { "Content-Type" => "application/json;charset=utf-8" }, [{"error" => "not_found"}.to_json]]
  end
end

run Linear.new
```

## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

  # Avoid build failures when C and C++ mrbgems exist.
  conf.disable_cxx_exception
  conf.linker do |linker|
      linker.libraries = %w(stdc++ m pthread dl z)
  end

  require 'open3'

  def run_command env, command
    STDOUT.sync = true
    puts "build: [exec] #{command}"
    Open3.popen2e(env, command) do |stdin, stdout, thread|
      print stdout.read
      fail "#{command} failed" if thread.value != 0
    end
  end

  # Avoid confict libmruby_core and libmruby when linker use -all_load option.
  e = {}
  run_command e, "ruby -i -pe '$_.gsub!(\"file exec => [driver_obj, mlib, mrbtest_lib, libmruby_core, libmruby]\", \"file exec => [driver_obj, mlib, mrbtest_lib, libmruby]\")' mrbgems/mruby-test/mrbgem.rake"

  # ... (snip) ...

  conf.gem :github => 'monochromegane/mruby-tensorflow'
end
```

#### NOTE

- `conf.disable_cxx_exception` must be written above `conf.gem` section.
- This mrbgem build a TensorFlow static library (libtensorflow-core.a). So, the process needs a lot of memory.
- If you install `protobuf` by OSX homebrew, you must uninstall the package.

## License

[MIT](https://github.com/monochromegane/mruby-tensorflow/blob/master/LICENSE)

## Author

[monochromegane](https://github.com/monochromegane)
