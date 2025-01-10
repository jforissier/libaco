.text
.globl _ucosw
.type  _ucosw, @function
_ucosw:
/* void _ucosw(struct uco *from_co, struct uco *to_co); */ 
    // x0: from_co
    // x1: to_co 
    // from_co and to_co layout: { pc, sp, x19-x29 }

    // Save context to from_co (x0)
    // AAPCS64 says "A subroutine invocation must preserve the contents of the
    // registers r19-r29 and SP"
    adr x2, 1f  // pc we should use to resume after this function
    mov x3, sp
    stp x2, x3, [x0, #0]  // pc, sp
    stp x19, x20, [x0, #16]
    stp x21, x22, [x0, #32]
    stp x23, x24, [x0, #48]
    stp x25, x26, [x0, #64]
    stp x27, x28, [x0, #80]
    stp x29, x30, [x0, #96]

    // Load new context from to_co (x1)
    ldp x2, x3, [x1, #0]  // pc, sp
    ldp x19, x20, [x1, #16]
    ldp x21, x22, [x1, #32]
    ldp x23, x24, [x1, #48]
    ldp x25, x26, [x1, #64]
    ldp x27, x28, [x1, #80]
    ldp x29, x30, [x1, #96]
    mov sp, x3
    br x2

1:  // Return to the caller
    ret
