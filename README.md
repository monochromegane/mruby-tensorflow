# mruby-tensorflow   [![Build Status](https://travis-ci.org/monochromegane/mruby-tensorflow.svg?branch=master)](https://travis-ci.org/monochromegane/mruby-tensorflow)
TensorFlow class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'monochromegane/mruby-tensorflow'
end
```
## example
```ruby
p TensorFlow.hi
#=> "hi!!"
t = TensorFlow.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the MIT License:
- see LICENSE file
