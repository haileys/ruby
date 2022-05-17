#ifndef INTERNAL_REF_H
#define INTERNAL_REF_H

#include "ruby/ruby.h"

/* ref.c */
VALUE rb_ref_local(VALUE env, size_t index);
VALUE rb_ref_ivar(VALUE self, ID var);
VALUE rb_ref_cvar(VALUE self, ID var);
VALUE rb_ref_global(ID var);

#endif /* INTERNAL_REF_H */
