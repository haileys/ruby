#include <sys/mman.h>

#include "vm_core.h"
#include "internal.h"
#include "insns.inc"
#include "insns_info.inc"

#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"

typedef struct {
    rb_iseq_t *iseq;
    dasm_State *dasm_state;
    size_t i;
}
jit_ctx_t;

typedef struct {
    size_t size;
    char mem[];
}
jit_code_t;

static VALUE
JITError;

static VALUE
insn_at(jit_ctx_t *ctx, size_t idx)
{
    return ctx->iseq->iseq[idx];
}

static VALUE
next(jit_ctx_t *ctx)
{
    return insn_at(ctx, ctx->i++);
}

static void
free_ctx(jit_ctx_t *ctx)
{
    dasm_free(&ctx->dasm_state);
}

#define Dst (&ctx->dasm_state)
#include "jit.inc"
#undef Dst

VALUE
rb_iseq_jit(VALUE iseqval)
{
    jit_ctx_t ctx;
    int dasm_status;
    size_t size;
    VALUE code_val;

    GetISeqPtr(iseqval, ctx.iseq);
    dasm_init(&ctx.dasm_state, 1);
    dasm_setup(&ctx.dasm_state, dasm_actionlist);

    for (ctx.i = 0; ctx.i < ctx.iseq->iseq_size;) {
	jit_instruction(&ctx);
    }

    dasm_status = dasm_link(&ctx.dasm_state, &size);
    if (dasm_status) {
	free_ctx(&ctx);
	return Qfalse;
    }

    /*mem = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);*/

    code_val = rb_str_buf_new(size);
    RSTRING(code_val)->as.heap.len = size;
    dasm_encode(&ctx.dasm_state, RSTRING_PTR(code_val));

    free_ctx(&ctx);

    return code_val;
}

void
Init_jit()
{
    JITError = rb_define_class("JITError", rb_eStandardError);
    rb_define_method(rb_cISeq, "jit", rb_iseq_jit, 0);
}
