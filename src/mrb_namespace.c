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
#include <sys/mount.h>
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

static int mrb_namespace_setns(mrb_state *mrb, int fd, int nstype) {
  return setns(fd, nstype);
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

static int mrb_namespace_pid_to_nsfile(mrb_state *mrb, char** procpath, pid_t _pid, int flag) {
  const char *procpath_fmt = "/proc/%i/ns/%s";
  int ret = 0;
  int pid = (int)_pid;

  switch (flag) {
  case CLONE_NEWNS:
    ret = asprintf(procpath, procpath_fmt, pid, "mnt");
    break;
  case CLONE_NEWUTS:
    ret = asprintf(procpath, procpath_fmt, pid, "uts");
    break;
  case CLONE_NEWIPC:
    ret = asprintf(procpath, procpath_fmt, pid, "ipc");
    break;
  case CLONE_NEWUSER:
    ret = asprintf(procpath, procpath_fmt, pid, "user");
    break;
  case CLONE_NEWPID:
    ret = asprintf(procpath, procpath_fmt, pid, "pid");
    break;
  case CLONE_NEWNET:
    ret = asprintf(procpath, procpath_fmt, pid, "net");
    break;
#ifdef CLONE_NEWCGROUP
  case CLONE_NEWCGROUP:
    ret = asprintf(procpath, procpath_fmt, pid, "cgroup");
    break;
#endif
  default:
    mrb_raise(mrb, NULL, "invalid namespace id. check that flag is or'ed?");
    return -1;
  }

  return ret;
}

static mrb_value mrb_namespace_setns_by_pid(mrb_state *mrb, mrb_value self)
{
  int pid, nsflag, fileno, ret;
  int ns_count = 0;
  char *procpath;

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

    if(mrb_namespace_pid_to_nsfile(mrb, &procpath, (pid_t)pid, curns) < 0) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "ns file detection failed");
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

static mrb_value mrb_namespace_persist_ns(mrb_state *mrb, mrb_value self)
{
  mrb_int pid, flag;
  char *dest;

  mrb_get_args(mrb, "iiz", &pid, &flag, &dest);

  if(!pid) {
    pid = (mrb_int)getpid();
  }

  char *procpath;
  if(mrb_namespace_pid_to_nsfile(mrb, &procpath, (pid_t)pid, (int)flag) < 0) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "ns file detection failed");
  }

  int dest_fd;
  if((dest_fd = open(dest, O_WRONLY|O_CREAT, 0660)) < 0){
    mrb_sys_fail(mrb, "open dest file failed");
  }
  if(futimens(dest_fd, NULL) < 0){
    mrb_sys_fail(mrb, "futimens failed");
  }
  close(dest_fd);

  if(mount(procpath, dest, "none", MS_BIND, NULL) < 0){
    mrb_sys_fail(mrb, "mount failed - cannot bind ns file to other location");
  }
  return mrb_fixnum_value(0);
}

struct mrb_clone_params {
  mrb_state *mrb;
  mrb_value block;
};
static volatile struct mrb_clone_params* clone_params = NULL;

static int mrb_clone_childfunc(void *params)
{
  if(!clone_params) {
    printf("[BUG] No clone params...\n");
    abort();
  }

  mrb_state *mrb = clone_params->mrb;
  mrb_value ret = mrb_yield_with_class(mrb, clone_params->block, 0, NULL, mrb_nil_value(), mrb->object_class);
  /* mrb_funcall(mrb, mrb_obj_value(mrb->object_class), "puts", 1, mrb_str_new_lit(mrb, "Hello from clone")); */

  _exit(0);
  return 0;
}

#define STACK_SIZE 4096

static mrb_value mrb_namespace_clone(mrb_state *mrb, mrb_value self)
{
  struct mrb_clone_params *p;
  mrb_int flag;
  mrb_value block;
  char *stack, *stack_top;
  int pid;

  mrb_get_args(mrb, "i&", &flag, &block);

  if (!mrb_nil_p(block)) {
    p = (struct mrb_clone_params *)malloc(sizeof(struct mrb_clone_params));
    p->mrb = mrb;
    p->block = block;

    stack = malloc(STACK_SIZE);
    if (p == NULL || stack == NULL)
      mrb_sys_fail(mrb, "malloc failed");

    stack_top = stack + STACK_SIZE;

    clone_params = p;
    pid = clone(
      mrb_clone_childfunc,
      stack_top,
      SIGCHLD|flag,
      NULL);
    if(pid < 0) {
      perror("clone");
      mrb_sys_fail(mrb, "clone failed");
    }

    return mrb_fixnum_value(pid);
  }

  mrb_raise(mrb, E_ARGUMENT_ERROR, "block must be passed.");
  return mrb_nil_value();
}

void mrb_mruby_linux_namespace_gem_init(mrb_state *mrb)
{
  struct RClass *namespace;
  namespace = mrb_define_module(mrb, "Namespace");
  mrb_define_class_method(mrb, namespace, "do_unshare", mrb_namespace_unshare, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, namespace, "setns_by_fd", mrb_namespace_setns_by_fd, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, namespace, "setns_by_pid", mrb_namespace_setns_by_pid, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, namespace, "persist_ns", mrb_namespace_persist_ns, MRB_ARGS_REQ(3));
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
