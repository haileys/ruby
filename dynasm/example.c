/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.3.0, DynASM x64 version 1.3.0
** DO NOT EDIT! The original file is in "example.dasc".
*/

#if DASM_VERSION != 10300
#error "Version mismatch between DynASM and included encoding engine"
#endif

# 1 "example.dasc"
#include <stdio.h>
#include <stdlib.h>
#include "dasm_x86.h"

//|.arch x64
//|.actionlist actions
static const unsigned char actions[9] = {
  72,199,192,123,0,0,0,195,255
};

# 7 "example.dasc"

#define Dst &state

int main() {
    dasm_State* state;
    initjit(&state, actions);

    //| mov rax, 123
    //| ret
    dasm_put(Dst, 0);
# 16 "example.dasc"

    int (*fptr)() = jitcode(&state);
    fprintf(stderr, "%d\n", fptr());
    free_jitcode(fptr);

    return 0;
}
