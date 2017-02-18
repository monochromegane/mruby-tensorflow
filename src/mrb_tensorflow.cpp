/*
** mrb_tensorflow.cpp - TensorFlow class
**
** Copyright (c) monochromegane 2017
**
** See Copyright Notice in LICENSE
*/

#include "mruby.h"
#include "mruby/data.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

extern "C" {
void mrb_mruby_tensorflow_gem_init(mrb_state *mrb)
{
  DONE;
}

void mrb_mruby_tensorflow_gem_final(mrb_state *mrb)
{
}
}
