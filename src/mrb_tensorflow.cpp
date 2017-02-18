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

#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/graph.pb.h"

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

extern "C" {
void mrb_mruby_tensorflow_gem_init(mrb_state *mrb)
{
  struct RClass *tensorflow_module;
  tensorflow_module = mrb_define_module(mrb, "TensorFlow");

  struct RClass *graph_def = mrb_define_class_under(mrb, tensorflow_module, "GraphDef", mrb->object_class);
  MRB_SET_INSTANCE_TT(graph_def, MRB_TT_DATA);
  mrb_define_method(mrb, graph_def, "load", mrb_tensorflow_graph_def_load, MRB_ARGS_REQ(1));
  DONE;
}

void mrb_mruby_tensorflow_gem_final(mrb_state *mrb)
{
}
}
