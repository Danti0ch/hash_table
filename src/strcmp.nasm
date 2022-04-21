section .text

global fstrcmp
fstrcmp:
    push rbp
    mov rbp, rsp
    
    xor rcx, rcx

symb_cmp_loop:
    
    cmp byte [rsi], byte [rdi]
    jb l_lower
    ja r_lower

    cmp byte [rsi], 0
    je l_check_len

    cmp byte [rdi], 0
    je r_lower

    je symb_cmp_loop

l_check_len:
    cmp byte [rdi], 0
    jne l_lower

    pop rbp
    mov rax, 0
    ret

l_lower:
    pop rbp
    mov rax, -1
    ret

r_lower:
    pop rbp
    mov rax, 1
    ret
