/*
** mrb_tensorflow.cpp - TensorFlow class
**
** Copyright (c) monochromegane 2017
**
** See Copyright Notice in LICENSE
*/

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/string.h"
#include "mruby/array.h"

#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/framework/ops.h"

using std::vector;
using std::string;

#define DONE mrb_gc_arena_restore(mrb, 0);

// TensorFlow::GraphDef
static void tensorflow_graph_def_free(mrb_state *mrb, void *ptr)
{
  tensorflow::GraphDef* graph_def = static_cast<tensorflow::GraphDef*>(ptr);
  delete graph_def;
}

static struct mrb_data_type tensorflow_graph_def_type = { "GraphDef", tensorflow_graph_def_free };

static mrb_value mrb_tensorflow_graph_def_load(mrb_state *mrb, mrb_value self)
{
  DATA_TYPE(self) = &tensorflow_graph_def_type;
  DATA_PTR(self) = NULL;

  mrb_value filename;
  int len;
  mrb_get_args(mrb, "S", &filename, &len);

  tensorflow::GraphDef *graph_def = new tensorflow::GraphDef();
  tensorflow::Status status = ReadBinaryProto(tensorflow::Env::Default(), mrb_str_to_cstr(mrb, filename), graph_def);

  if (!status.ok()) {
    mrb_raisef(mrb, mrb_class_get(mrb, "NotFoundError"), "Failed to load compute graph at '%S'", filename);
  }
  DATA_PTR(self) = graph_def;

  return self;
}

// TensorFlow::Tensor
static void tensorflow_tensor_free(mrb_state *mrb, void *ptr)
{
  tensorflow::Tensor* tensor = static_cast<tensorflow::Tensor*>(ptr);
  delete tensor;
}

static struct mrb_data_type tensorflow_tensor_type = { "Tensor", tensorflow_tensor_free };

static mrb_value tensorflow_tensor_init(mrb_state *mrb, tensorflow::Tensor *tensor)
{
  RClass *tensor_class = mrb_class_get_under(mrb, mrb_module_get(mrb, "TensorFlow"), "Tensor");

  mrb_value dummy_input = mrb_ary_new(mrb);
  mrb_value dummy_shape = mrb_ary_new(mrb);
  mrb_ary_push(mrb, dummy_shape, mrb_float_value(mrb, 1.0f));

  mrb_value args[2];
  args[0] = dummy_input;
  args[1] = dummy_shape;

  mrb_value mrb_tensor = mrb_class_new_instance(mrb, 2, args, tensor_class);

  DATA_TYPE(mrb_tensor) = &tensorflow_tensor_type;
  DATA_PTR(mrb_tensor) = NULL;
  DATA_PTR(mrb_tensor) = tensor;

  return mrb_tensor;
}

static mrb_value mrb_tensorflow_tensor_init(mrb_state *mrb, mrb_value self)
{
  DATA_TYPE(self) = &tensorflow_tensor_type;
  DATA_PTR(self) = NULL;

  mrb_value value_ary, shape_ary;
  mrb_get_args(mrb, "AA", &value_ary, &shape_ary);

  // shape
  int shape_len = RARRAY_LEN(shape_ary);
  vector<long long int> shape(shape_len);
  for (int i = 0; i < shape_len; ++i){
    shape[i] = mrb_fixnum(mrb_ary_ref(mrb, shape_ary, i));
  }

  // tensor
  tensorflow::Tensor *tensor = new tensorflow::Tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape(shape));

  int value_len = RARRAY_LEN(value_ary);
  for (int i = 0; i < value_len; ++i){
    tensor->flat<float>()(i) = mrb_float(mrb_ary_ref(mrb, value_ary, i));
  }

  DATA_PTR(self) = tensor;

  return self;
}

static mrb_value mrb_tensorflow_tensor_flat(mrb_state *mrb, mrb_value self)
{
  tensorflow::Tensor* tensor = static_cast<tensorflow::Tensor*>(mrb_get_datatype(mrb, self, &tensorflow_tensor_type));
  auto values = tensor->flat<float>();
  mrb_value ary = mrb_ary_new(mrb);
  for (int i = 0; i < values.size(); ++i) {
    mrb_ary_push(mrb, ary, mrb_float_value(mrb, values(i)));
  }
  return ary;
}

// TensorFlow::Session
static void tensorflow_session_free(mrb_state *mrb, void *ptr)
{
  tensorflow::Session* session = static_cast<tensorflow::Session*>(ptr);
  delete session;
}

static struct mrb_data_type tensorflow_session_type = { "Session", tensorflow_session_free };

static mrb_value mrb_tensorflow_session_init(mrb_state *mrb, mrb_value self)
{
  DATA_TYPE(self) = &tensorflow_session_type;
  DATA_PTR(self) = NULL;

  tensorflow::Session* session = tensorflow::NewSession(tensorflow::SessionOptions());

  DATA_PTR(self) = session;

  return self;
}

static mrb_value mrb_tensorflow_session_create(mrb_state *mrb, mrb_value self)
{
  mrb_value graph;
  mrb_get_args(mrb, "o", &graph);

  tensorflow::Session* session = static_cast<tensorflow::Session*>(mrb_get_datatype(mrb, self, &tensorflow_session_type));
  tensorflow::GraphDef* graph_def = static_cast<tensorflow::GraphDef*>(mrb_get_datatype(mrb, graph, &tensorflow_graph_def_type));

  tensorflow::Status status = session->Create(*graph_def);
  if (!status.ok()) {
    mrb_raise(mrb, mrb_class_get(mrb, "SessionError"), status.error_message().c_str());
  }
  return self;
}

static mrb_value mrb_tensorflow_session_close(mrb_state *mrb, mrb_value self)
{
  tensorflow::Session* session = static_cast<tensorflow::Session*>(mrb_get_datatype(mrb, self, &tensorflow_session_type));
  tensorflow::Status status = session->Close();
  if (!status.ok()) {
    mrb_raise(mrb, mrb_class_get(mrb, "SessionError"), status.error_message().c_str());
  }
  return self;
}

typedef std::pair<string, tensorflow::Tensor> input_pair;

static mrb_value mrb_tensorflow_session_run(mrb_state *mrb, mrb_value self)
{
  mrb_value input_ary, output_ary;
  mrb_get_args(mrb, "AA", &input_ary, &output_ary);

  // input
  int input_len = RARRAY_LEN(input_ary);
  vector<input_pair> inputs(input_len);
  for (int i = 0; i < input_len; ++i){
    mrb_value input = mrb_ary_ref(mrb, input_ary, i);
    string name = mrb_str_to_cstr(mrb, mrb_ary_ref(mrb, input, 0));
    mrb_value mrb_tensor = mrb_ary_ref(mrb, input, 1);
    tensorflow::Tensor* tensor = static_cast<tensorflow::Tensor*>(mrb_get_datatype(mrb, mrb_tensor, &tensorflow_tensor_type));
    inputs[i] = input_pair(name, *tensor);
  }

  // output
  int output_len = RARRAY_LEN(output_ary);
  vector<string> output_tensor_names(output_len);
  for (int i = 0; i < output_len; ++i){
    output_tensor_names[i] = mrb_str_to_cstr(mrb, mrb_ary_ref(mrb, output_ary, i));
  }

  vector<tensorflow::Tensor> out_tensors;

  tensorflow::Session* session = static_cast<tensorflow::Session*>(mrb_get_datatype(mrb, self, &tensorflow_session_type));

  tensorflow::Status status = session->Run(inputs, output_tensor_names, {}, &out_tensors);
  if (!status.ok()) {
    mrb_raise(mrb, mrb_class_get(mrb, "SessionError"), status.error_message().c_str());
  }

  mrb_value mrb_tensors = mrb_ary_new(mrb);
  for (auto t: out_tensors){
    tensorflow::Tensor *tensor = new tensorflow::Tensor(t);
    mrb_ary_push(mrb, mrb_tensors, tensorflow_tensor_init(mrb, tensor));
  }
  return mrb_tensors;
}

extern "C" {
void mrb_mruby_tensorflow_gem_init(mrb_state *mrb)
{
  struct RClass *tensorflow_module;
  tensorflow_module = mrb_define_module(mrb, "TensorFlow");

  struct RClass *graph_def = mrb_define_class_under(mrb, tensorflow_module, "GraphDef", mrb->object_class);
  MRB_SET_INSTANCE_TT(graph_def, MRB_TT_DATA);
  mrb_define_method(mrb, graph_def, "load", mrb_tensorflow_graph_def_load, MRB_ARGS_REQ(1));

  struct RClass *tensor = mrb_define_class_under(mrb, tensorflow_module, "Tensor", mrb->object_class);
  MRB_SET_INSTANCE_TT(tensor, MRB_TT_DATA);
  mrb_define_method(mrb, tensor, "initialize", mrb_tensorflow_tensor_init, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, tensor, "flat",       mrb_tensorflow_tensor_flat, MRB_ARGS_NONE());

  struct RClass *session = mrb_define_class_under(mrb, tensorflow_module, "Session", mrb->object_class);
  MRB_SET_INSTANCE_TT(session, MRB_TT_DATA);
  mrb_define_method(mrb, session, "initialize", mrb_tensorflow_session_init,   MRB_ARGS_NONE());
  mrb_define_method(mrb, session, "create",     mrb_tensorflow_session_create, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, session, "run",        mrb_tensorflow_session_run,    MRB_ARGS_REQ(2));
  mrb_define_method(mrb, session, "close",      mrb_tensorflow_session_close,  MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_tensorflow_gem_final(mrb_state *mrb)
{
}
}
