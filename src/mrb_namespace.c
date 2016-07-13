/*
** mrb_namespace.c - For linux namespace
**
** Copyright (c) Russel Hunter Yukawa / Uchio Kondo 2016
**
** See Copyright Notice in LICENSE
*/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <features.h>
#include <signal.h>
#include <pthread.h>

#include <mruby.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/proc.h>

#include "mrb_namespace.h"

#define DONE mrb_gc_arena_restore(mrb, 0);

#if __GLIBC__ != 2 || __GLIBC_MINOR__ < 14
#include <sys/syscall.h>
#ifndef SYS_setns
#define SYS_setns 308
#endif

static int setns(int fd, int nstype) {
  long ret = syscall(SYS_setns, fd, nstype);
  return (int)ret;
}
#endif

typedef struct {
} mrb_namespace_data;

static const struct mrb_data_type mrb_namespace_data_type = {
  "mrb_namespace_data", mrb_free,
};

static mrb_value mrb_namespace_init(mrb_state *mrb, mrb_value self)
{
  mrb_namespace_data *data;

  data = (mrb_namespace_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_namespace_data_type;
  DATA_PTR(self) = NULL;

  return self;
}

static int mrb_namespace_setns(mrb_state *mrb, int fd, int nstype) {
  return setns(fd, nstype);
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

static mrb_value mrb_namespace_setns_by_fd(mrb_state *mrb, mrb_value self)
{
  int fileno, nstype, ret;

  mrb_get_args(mrb, "ii", &fileno, &nstype);
  ret = mrb_namespace_setns(mrb, fileno, nstype);
  if (ret < 0) {
    mrb_sys_fail(mrb, "setns failed");
  }

  return mrb_fixnum_value(ret);
}

static mrb_value mrb_namespace_setns_by_pid(mrb_state *mrb, mrb_value self)
{
  int pid, nsflag, fileno, ret;
  int ns_count = 0;
  char *procpath;
  const char *procpath_fmt = "/proc/%i/ns/%s";

#ifdef CLONE_NEWCGROUP
  int namespaces[8] = {
    CLONE_NEWNS,
    CLONE_NEWUTS,
    CLONE_NEWIPC,
    CLONE_NEWUSER,
    CLONE_NEWPID,
    CLONE_NEWNET,
    CLONE_NEWCGROUP,
    0,
  };
#else
  int namespaces[7] = {
    CLONE_NEWNS,
    CLONE_NEWUTS,
    CLONE_NEWIPC,
    CLONE_NEWUSER,
    CLONE_NEWPID,
    CLONE_NEWNET,
    0,
  };
#endif

  mrb_get_args(mrb, "ii", &pid, &nsflag);

  for(int i=0; namespaces[i]; ++i) {
    int curns = nsflag & namespaces[i];
    if(!curns) continue;

    switch (curns) {
    case CLONE_NEWNS:
      asprintf(&procpath, procpath_fmt, pid, "mnt");
      break;
    case CLONE_NEWUTS:
      asprintf(&procpath, procpath_fmt, pid, "uts");
      break;
    case CLONE_NEWIPC:
      asprintf(&procpath, procpath_fmt, pid, "ipc");
      break;
    case CLONE_NEWUSER:
      asprintf(&procpath, procpath_fmt, pid, "user");
      break;
    case CLONE_NEWPID:
      asprintf(&procpath, procpath_fmt, pid, "pid");
      break;
    case CLONE_NEWNET:
      asprintf(&procpath, procpath_fmt, pid, "net");
      break;
#ifdef CLONE_NEWCGROUP
    case CLONE_NEWCGROUP:
      asprintf(&procpath, procpath_fmt, pid, "cgroup");
      break;
#endif
    default:
      mrb_raise(mrb, NULL, "invalid namespace id");
      return mrb_fixnum_value(-1);
    }

    fileno = open(procpath, O_RDONLY);

    ret = mrb_namespace_setns(mrb, fileno, curns);
    close(fileno);
    if (ret < 0) {
      mrb_sys_fail(mrb, "setns failed");
    }
    ns_count++;
  }

  return mrb_fixnum_value(ns_count);
}

typedef struct mrb_clone_params {
  mrb_state *mrb;
  struct RProc *block;
} mrb_clone_params;

/* from https://github.com/mattn/mruby-thread/blob/master/src/mrb_thread.c */
static mrb_sym migrate_sym(mrb_state *mrb, mrb_sym sym, mrb_state *mrb2)
{
  mrb_int len;
  const char *p = mrb_sym2name_len(mrb, sym, &len);
  return mrb_intern_static(mrb2, p, len);
}

static void migrate_all_symbols(mrb_state *mrb, mrb_state *mrb2)
{
  mrb_sym i;
  for (i = 1; i < mrb->symidx + 1; i++) {
    migrate_sym(mrb, i, mrb2);
  }
}

static int mrb_clone_childfunc(void *params)
{
  mrb_clone_params *p;
  mrb_state *mrb;
  /* mrb_value result; */

  printf("Cloned...\n");
  p = (mrb_clone_params *)params;
  mrb = p->mrb;
  printf("restore: %p, %p, %p\n", p, mrb, p->block);
  //mrb_yield_with_class(p->mrb, mrb_obj_value(p->block), 0, NULL, mrb_nil_value(), p->mrb->object_class);
  mrb_funcall(mrb, mrb_obj_value(mrb->object_class), "puts", 1, mrb_str_new_lit(mrb, "Hello from clone"));

  _exit(0);
  return 0;
}

#define STACK_SIZE 65535

static mrb_value mrb_namespace_clone(mrb_state *mrb, mrb_value self)
{
  mrb_clone_params *p;
  mrb_int flag;
  mrb_value block;
  char *stack, *stack_top;
  int pid;

  mrb_get_args(mrb, "i&", &flag, &block);

  if (!mrb_nil_p(block)) {
    p = (mrb_clone_params *)malloc(sizeof(mrb_clone_params));
    p->mrb = mrb_open_allocf(mrb->allocf, mrb->allocf_ud);
    migrate_all_symbols(mrb, p->mrb);
    p->block = mrb_proc_new(mrb, mrb_proc_ptr(block)->body.irep);
    p->block->target_class = p->mrb->object_class;

    printf("alloc: %p, %p, %p\n", p, p->mrb, p->block);

    stack = malloc(STACK_SIZE);
    if (p == NULL || stack == NULL)
      mrb_sys_fail(mrb, "malloc failed");

    stack_top = stack + STACK_SIZE;

    pid = clone(
      mrb_clone_childfunc,
      stack_top,
      SIGCHLD|flag,
      (void *)p);
    if(pid < 0) {
      perror("clone");
      mrb_sys_fail(mrb, "clone failed");
    }

    return mrb_fixnum_value(pid);
  }

  mrb_raise(mrb, E_ARGUMENT_ERROR, "block must be passed.");
  return mrb_nil_value();
}

void mrb_mruby_namespace_gem_init(mrb_state *mrb)
{
  struct RClass *namespace;
  namespace = mrb_define_class(mrb, "Namespace", mrb->object_class);
  mrb_define_method(mrb, namespace, "initialize", mrb_namespace_init, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, namespace, "unshare", mrb_namespace_unshare, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, namespace, "setns_by_fd", mrb_namespace_setns_by_fd, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, namespace, "setns_by_pid", mrb_namespace_setns_by_pid, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, namespace, "getuid", mrb_namespace_getuid, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, namespace, "getgid", mrb_namespace_getgid, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, namespace, "getpid", mrb_namespace_getpid, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, namespace, "clone",  mrb_namespace_clone,  (MRB_ARGS_REQ(1)|MRB_ARGS_BLOCK()));

  mrb_define_const(mrb, namespace, "CLONE_VM",             mrb_fixnum_value(CLONE_VM));
  mrb_define_const(mrb, namespace, "CLONE_FS",             mrb_fixnum_value(CLONE_FS));
  mrb_define_const(mrb, namespace, "CLONE_FILES",          mrb_fixnum_value(CLONE_FILES));
  mrb_define_const(mrb, namespace, "CLONE_SIGHAND",        mrb_fixnum_value(CLONE_SIGHAND));
  mrb_define_const(mrb, namespace, "CLONE_PTRACE",         mrb_fixnum_value(CLONE_PTRACE));
  mrb_define_const(mrb, namespace, "CLONE_VFORK",          mrb_fixnum_value(CLONE_VFORK));
  mrb_define_const(mrb, namespace, "CLONE_PARENT",         mrb_fixnum_value(CLONE_PARENT));
  mrb_define_const(mrb, namespace, "CLONE_THREAD",         mrb_fixnum_value(CLONE_THREAD));
  mrb_define_const(mrb, namespace, "CLONE_NEWNS",          mrb_fixnum_value(CLONE_NEWNS));
  mrb_define_const(mrb, namespace, "CLONE_SYSVSEM",        mrb_fixnum_value(CLONE_SYSVSEM));
  mrb_define_const(mrb, namespace, "CLONE_SETTLS",         mrb_fixnum_value(CLONE_SETTLS));
  mrb_define_const(mrb, namespace, "CLONE_PARENT_SETTID",  mrb_fixnum_value(CLONE_PARENT_SETTID));
  mrb_define_const(mrb, namespace, "CLONE_CHILD_CLEARTID", mrb_fixnum_value(CLONE_CHILD_CLEARTID));
  mrb_define_const(mrb, namespace, "CLONE_DETACHED",       mrb_fixnum_value(CLONE_DETACHED));
  mrb_define_const(mrb, namespace, "CLONE_UNTRACED",       mrb_fixnum_value(CLONE_UNTRACED));
  mrb_define_const(mrb, namespace, "CLONE_CHILD_SETTID",   mrb_fixnum_value(CLONE_CHILD_SETTID));
  mrb_define_const(mrb, namespace, "CLONE_NEWUTS",         mrb_fixnum_value(CLONE_NEWUTS));
  mrb_define_const(mrb, namespace, "CLONE_NEWIPC",         mrb_fixnum_value(CLONE_NEWIPC));
  mrb_define_const(mrb, namespace, "CLONE_NEWUSER",        mrb_fixnum_value(CLONE_NEWUSER));
  mrb_define_const(mrb, namespace, "CLONE_NEWPID",         mrb_fixnum_value(CLONE_NEWPID));
  mrb_define_const(mrb, namespace, "CLONE_NEWNET",         mrb_fixnum_value(CLONE_NEWNET));
  mrb_define_const(mrb, namespace, "CLONE_IO",             mrb_fixnum_value(CLONE_IO));
#ifdef CLONE_NEWCGROUP
  mrb_define_const(mrb, namespace, "CLONE_NEWCGROUP",      mrb_fixnum_value(CLONE_NEWCGROUP));
#endif

  DONE;
}

void mrb_mruby_linux_namespace_gem_final(mrb_state *mrb)
{
}
