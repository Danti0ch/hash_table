section .text

global fstrcmp
fstrcmp:
    push rbp
    mov rbp, rsp
    
symb_cmp_loop:
    
    mov dl, byte [rdi]
    mov dh, byte [rsi]

    cmp dl, dh
    jb l_lower
    ja r_lower

    cmp dl, 0
    je l_check_len

    cmp dh, 0
    je r_lower

    inc rsi
    inc rdi
    jmp symb_cmp_loop

l_check_len:
    cmp dh, 0
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
