##
## TensorFlow Test
##

assert("TensorFlow::GraphDef#load") do
  graph = TensorFlow::GraphDef.new
  assert_raise(TensorFlow::NotFoundError) { graph.load('not_found') }
end

assert("TensorFlow::Tensor#float") do
  ary = [1.0, 2.0, 3.0]
  tensor = TensorFlow::Tensor.new(ary, [1, 3])
  assert_equal ary, tensor.flat
end

assert("TensorFlow::Session#run") do
  a, x, b = 1.0, 2.0, 3.0
  a_tensor = TensorFlow::Tensor.new([a], [1])
  x_tensor = TensorFlow::Tensor.new([x], [1])
  b_tensor = TensorFlow::Tensor.new([b], [1])

  graph = TensorFlow::GraphDef.new
  graph.load(File.dirname(__FILE__) + '/linear_function_graph.pb')
  session = TensorFlow::Session.new
  session.create(graph)

  tensors = session.run([['a:0', a_tensor], ['x:0', x_tensor], ['b:0', b_tensor]], ['y:0'])
  assert_equal (a*x+b), tensors[0].flat[0]
end
