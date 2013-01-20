#include "ruby/ruby.h"
#include "vm_core.h"
#include "node.h"
#include "gc.h"

VALUE rb_cRef;

static VALUE
sym_local,
sym_ivar,
sym_cvar,
sym_global;

typedef enum {
    REF_NONE,
    REF_LOCAL,
    REF_IVAR,
    REF_CVAR,
    REF_GVAR
}
rb_ref_type_t;

typedef struct {
    rb_ref_type_t type;
    union {
	struct {
	    VALUE env;
	    size_t index;
	} local;
	struct {
	    VALUE self;
	    ID var;
	} ivar, cvar;
	struct rb_global_entry *global;
    } as;
}
rb_ref_t;

static void
ref_mark(void *ptr)
{
    rb_ref_t *ref = ptr;
    RUBY_MARK_ENTER("ref");
    if(ref) {
        switch(ref->type) {
	  case REF_NONE:
	    break;
          case REF_LOCAL:
	    RUBY_MARK_UNLESS_NULL(ref->as.local.env);
            break;
          case REF_IVAR:
          case REF_CVAR:
            RUBY_MARK_UNLESS_NULL(ref->as.ivar.self);
	    break;
          case REF_GVAR:
	    break;
	}
    }
    RUBY_MARK_LEAVE("ref");
}

static void
ref_free(void* ptr)
{
    if(ptr) {
	ruby_xfree(ptr);
    }
}

static size_t
ref_memsize(const void* ptr)
{
    return ptr ? sizeof(rb_ref_t) : 0;
}

static const rb_data_type_t
ref_data_type = {
    "ref",
    {
	ref_mark,
	ref_free,
	ref_memsize
    }
};

static VALUE
ref_alloc(VALUE klass)
{
    VALUE obj;
    rb_ref_t* ref = NULL;
    obj = TypedData_Make_Struct(klass, rb_ref_t, &ref_data_type, ref);
    ref->type = REF_NONE;
    return obj;
}

VALUE
rb_ref_local(VALUE env, size_t idx)
{
    VALUE refv = ref_alloc(rb_cRef);
    rb_ref_t* ref;
    TypedData_Get_Struct(refv, rb_ref_t, &ref_data_type, ref);
    ref->type = REF_LOCAL;
    ref->as.local.env = env;
    ref->as.local.index = idx;
    return refv;
}

VALUE
rb_ref_ivar(VALUE self, ID var)
{
    VALUE refv = ref_alloc(rb_cRef);
    rb_ref_t* ref;
    TypedData_Get_Struct(refv, rb_ref_t, &ref_data_type, ref);
    ref->type = REF_IVAR;
    ref->as.ivar.self = self;
    ref->as.ivar.var = var;
    return refv;
}

VALUE
rb_ref_cvar(VALUE self, ID var)
{
    VALUE refv = ref_alloc(rb_cRef);
    rb_ref_t* ref;
    TypedData_Get_Struct(refv, rb_ref_t, &ref_data_type, ref);
    ref->type = REF_CVAR;
    ref->as.ivar.self = self;
    ref->as.ivar.var = var;
    return refv;
}

VALUE
rb_ref_global(struct rb_global_entry* ge)
{
    VALUE refv = ref_alloc(rb_cRef);
    rb_ref_t* ref;
    TypedData_Get_Struct(refv, rb_ref_t, &ref_data_type, ref);
    ref->type = REF_GVAR;
    ref->as.global = ge;
    return refv;
}

static VALUE*
local_ref(rb_ref_t* ref)
{
    rb_env_t* env;
    GetEnvPtr(ref->as.local.env, env);
    return env->env + env->local_size - ref->as.local.index;
}

static VALUE
ref_value(VALUE self)
{
    rb_ref_t* ref;
    TypedData_Get_Struct(self, rb_ref_t, &ref_data_type, ref);
    switch(ref->type) {
      case REF_NONE:
        rb_raise(rb_eTypeError, "Can't get value of uninitialized reference");
      case REF_LOCAL:
	return *local_ref(ref);
      case REF_IVAR:
        return rb_ivar_get(ref->as.ivar.self, ref->as.ivar.var);
      case REF_CVAR:
        return rb_cvar_get(ref->as.cvar.self, ref->as.cvar.var);
      case REF_GVAR:
        return rb_gvar_get(ref->as.global);
    }
    return Qundef;
}

static VALUE
ref_value_set(VALUE self, VALUE val)
{
    rb_ref_t* ref;
    TypedData_Get_Struct(self, rb_ref_t, &ref_data_type, ref);
    switch(ref->type) {
      case REF_NONE:
        rb_raise(rb_eTypeError, "Can't get value of uninitialized reference");
	break;
      case REF_LOCAL:
	*local_ref(ref) = val;
	break;
      case REF_IVAR:
        rb_ivar_set(ref->as.ivar.self, ref->as.ivar.var, val);
	break;
      case REF_CVAR:
        rb_cvar_set(ref->as.cvar.self, ref->as.cvar.var, val);
	break;
      case REF_GVAR:
        rb_gvar_set(ref->as.global, val);
	break;
    }
    return val;
}

static VALUE
ref_inspect(VALUE self)
{
    rb_ref_t* ref;
    TypedData_Get_Struct(self, rb_ref_t, &ref_data_type, ref);
    switch(ref->type) {
	case REF_NONE:
	    return rb_str_new_cstr("#<Ref:(uninitialized)>");
	case REF_LOCAL:
	    return rb_str_new_cstr("#<Ref:(local)>");
	case REF_IVAR:
	case REF_CVAR:
	    return rb_sprintf("#<Ref:%"PRIsVALUE" %s>", ref->as.ivar.self, rb_id2name(ref->as.ivar.var));
	case REF_GVAR:
	    return rb_sprintf("#<Ref:%s>", rb_id2name(ref->as.global->id));
	default:
	    rb_bug("unknown ref->type");
    }
}

static VALUE
ref_type(VALUE self)
{
    rb_ref_t* ref;
    TypedData_Get_Struct(self, rb_ref_t, &ref_data_type, ref);
    switch(ref->type) {
	case REF_NONE:
	    return Qnil;
	case REF_LOCAL:
	    return sym_local;
	case REF_IVAR:
	    return sym_ivar;
	case REF_CVAR:
	    return sym_cvar;
	case REF_GVAR:
	    return sym_global;
	default:
	    rb_bug("unknown ref->type");
    }
}

void
Init_ref()
{
    rb_cRef = rb_define_class("Ref", rb_cObject);
    rb_define_alloc_func(rb_cRef, ref_alloc);
    rb_define_method(rb_cRef, "value", ref_value, 0);
    rb_define_method(rb_cRef, "value=", ref_value_set, 1);
    rb_define_method(rb_cRef, "inspect", ref_inspect, 0);
    rb_define_method(rb_cRef, "type", ref_type, 0);

    sym_local  = ID2SYM(rb_intern("local"));
    sym_ivar   = ID2SYM(rb_intern("ivar"));
    sym_cvar   = ID2SYM(rb_intern("cvar"));
    sym_global = ID2SYM(rb_intern("global"));
}

