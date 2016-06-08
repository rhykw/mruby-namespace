/*
** mrb_namespace.c - For linux namespace
**
** Copyright (c) Russel Hunter Yukawa 2016
**
** See Copyright Notice in LICENSE
*/

#define _GNU_SOURCE 1

#include <sched.h>
#include <unistd.h>
#include <sys/types.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mrb_namespace.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  char *str;
  int len;
} mrb_namespace_data;

static const struct mrb_data_type mrb_namespace_data_type = {
  "mrb_namespace_data", mrb_free,
};

static mrb_value mrb_namespace_init(mrb_state *mrb, mrb_value self)
{
  mrb_namespace_data *data;
  /*char *str;
  int len;*/

  data = (mrb_namespace_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_namespace_data_type;
  DATA_PTR(self) = NULL;

  DATA_PTR(self) = data;

  return self;
}


static mrb_value mrb_namespace_getuid(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)getuid());
}

static mrb_value mrb_namespace_getgid(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)getgid());
}

static mrb_value mrb_namespace_getpid(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)getpid());
}

static mrb_value mrb_namespace_unshare(mrb_state *mrb, mrb_value self)
{
  int unshare_flags = 0;
  mrb_int arg;

  mrb_get_args(mrb, "i", &arg);
  unshare_flags = (int)arg;

  return mrb_fixnum_value((mrb_int) unshare(unshare_flags));
}


void mrb_mruby_namespace_gem_init(mrb_state *mrb)
{
    struct RClass *namespace;
    namespace = mrb_define_class(mrb, "Namespace", mrb->object_class);
    mrb_define_method(mrb, namespace, "initialize", mrb_namespace_init, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, namespace, "unshare", mrb_namespace_unshare, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, namespace, "getuid", mrb_namespace_getuid, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, namespace, "getgid", mrb_namespace_getgid, MRB_ARGS_NONE());
    mrb_define_class_method(mrb, namespace, "getpid", mrb_namespace_getpid, MRB_ARGS_NONE());

    mrb_define_const(mrb, namespace, "CLONE_VM",                  mrb_fixnum_value(CLONE_VM));
    mrb_define_const(mrb, namespace, "CLONE_FS",                  mrb_fixnum_value(CLONE_FS));
    mrb_define_const(mrb, namespace, "CLONE_FILES",               mrb_fixnum_value(CLONE_FILES));
    mrb_define_const(mrb, namespace, "CLONE_SIGHAND",             mrb_fixnum_value(CLONE_SIGHAND));
    mrb_define_const(mrb, namespace, "CLONE_PTRACE",              mrb_fixnum_value(CLONE_PTRACE));
    mrb_define_const(mrb, namespace, "CLONE_VFORK",               mrb_fixnum_value(CLONE_VFORK));
    mrb_define_const(mrb, namespace, "CLONE_PARENT",              mrb_fixnum_value(CLONE_PARENT));
    mrb_define_const(mrb, namespace, "CLONE_THREAD",              mrb_fixnum_value(CLONE_THREAD));
    mrb_define_const(mrb, namespace, "CLONE_NEWNS",               mrb_fixnum_value(CLONE_NEWNS));
    mrb_define_const(mrb, namespace, "CLONE_SYSVSEM",             mrb_fixnum_value(CLONE_SYSVSEM));
    mrb_define_const(mrb, namespace, "CLONE_SETTLS",              mrb_fixnum_value(CLONE_SETTLS));
    mrb_define_const(mrb, namespace, "CLONE_PARENT_SETTID",       mrb_fixnum_value(CLONE_PARENT_SETTID));
    mrb_define_const(mrb, namespace, "CLONE_CHILD_CLEARTID",      mrb_fixnum_value(CLONE_CHILD_CLEARTID));
    mrb_define_const(mrb, namespace, "CLONE_DETACHED",            mrb_fixnum_value(CLONE_DETACHED));
    mrb_define_const(mrb, namespace, "CLONE_UNTRACED",            mrb_fixnum_value(CLONE_UNTRACED));
    mrb_define_const(mrb, namespace, "CLONE_CHILD_SETTID",        mrb_fixnum_value(CLONE_CHILD_SETTID));
    mrb_define_const(mrb, namespace, "CLONE_NEWUTS",              mrb_fixnum_value(CLONE_NEWUTS));
    mrb_define_const(mrb, namespace, "CLONE_NEWIPC",              mrb_fixnum_value(CLONE_NEWIPC));
    mrb_define_const(mrb, namespace, "CLONE_NEWUSER",             mrb_fixnum_value(CLONE_NEWUSER));
    mrb_define_const(mrb, namespace, "CLONE_NEWPID",              mrb_fixnum_value(CLONE_NEWPID));
    mrb_define_const(mrb, namespace, "CLONE_NEWNET",              mrb_fixnum_value(CLONE_NEWNET));
    mrb_define_const(mrb, namespace, "CLONE_IO",                  mrb_fixnum_value(CLONE_IO));

    DONE;
}

void mrb_mruby_namespace_gem_final(mrb_state *mrb)
{
}
