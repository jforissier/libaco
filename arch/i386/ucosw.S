.text
.globl _ucosw
.type  _ucosw, @function
.intel_syntax noprefix
_ucosw:
/* void _ucosw(struct uco *from_co, struct uco *to_co); */ 
    mov     eax,DWORD PTR [esp+0x4]     // from_co
    mov     edx,DWORD PTR [esp]         // retaddr
    lea     ecx,[esp+0x4]               // esp
    mov     DWORD PTR [eax+0x8],ebp     //<ebp
    mov     DWORD PTR [eax+0x4],ecx     //<esp
    mov     DWORD PTR [eax+0x0],edx     //<retaddr
    mov     DWORD PTR [eax+0xc],edi     //<edi
    mov     ecx,DWORD PTR [esp+0x8]     // to_co
    mov     DWORD PTR [eax+0x10],esi    //<esi
    mov     DWORD PTR [eax+0x14],ebx    //<ebx
    mov     edx,DWORD PTR [ecx+0x4]     //>esp
    mov     ebp,DWORD PTR [ecx+0x8]     //>ebp
    mov     eax,DWORD PTR [ecx+0x0]     //>retaddr
    mov     edi,DWORD PTR [ecx+0xc]     //>edi
    mov     esi,DWORD PTR [ecx+0x10]    //>esi
    mov     ebx,DWORD PTR [ecx+0x14]    //>ebx
    xor     ecx,ecx
    mov     esp,edx
    xor     edx,edx
    jmp     eax
