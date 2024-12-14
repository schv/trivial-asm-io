section .text
 
 
; Принимает код возврата и завершает текущий процесс
exit:
    xor rax, rax
    add rax, 60
    xor rdi, rdi
    
    syscall 

; - Функции принимают аргументы в `rdi`, `rsi`, `rdx`, `rcx`, `r8` и `r9`.

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax
.loop:
    mov cl, byte [rdi]
    test cl, cl
    jz .end

    inc rax
    inc rdi
    jmp .loop
.end:    
    ret ; -> rax

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string: ; <- rdi
     xor rax, rax
     
     push rdi
     call string_length ; -> rax
     pop rdi

     mov rdx, rax
     mov rax, 1
     mov rsi, rdi
     mov rdi, 1
     syscall

     ret

; Принимает код символа и выводит его в stdout
print_char:
    mov rax, 1
    push rdi
    mov rsi, rsp
    pop rdi
    mov rdi, 1
    mov rdx, 1
    
    syscall
    
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    mov rdi, 0xA
    call print_char
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint: ; <- rdi
    mov rax, rdi
    mov r10, 10
   
    mov rdi, rsp
    dec rdi
    push 0
    push 0
    push 0

.loop:
    xor rdx, rdx
    div r10
    
    dec rdi
    add rdx, 48
    mov BYTE [rdi], dl
   
    test rax, rax
    jz .end
    jmp .loop

.end:
    call print_string

.clear_stack:    
    pop rax
    pop rax
    pop rax
    
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int: ; <- rdi
    cmp rdi, 0
    jge .print_number 

    push rdi
    mov rdi, 45
    call print_char
    pop rdi
    
    neg rdi

.print_number:
    call print_uint
    
    ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals: ; <- rdi, rsi
.loop:
    mov BYTE al, [rdi]
    mov BYTE cl, [rsi]
    
    cmp al, cl
    jne .Fail
    
    test al, cl
    jz .Success
    
    inc rdi
    inc rsi

    jmp .loop

.Success:
    xor rax, rax
    inc rax
    ret

.Fail:
    xor rax, rax
    ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    push 0
    mov rsi, rsp
    
    xor rax, rax
    xor rdi, rdi
    mov rdx, 1
    syscall
    
    pop rax
    
    ret 

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор
read_word:         ; <- rdi (buf), rsi(buf size)
    xor rax, rax
    mov r10, rdi   ; <- buf
    
    xor r8, r8     ; <- cur size
    mov r9, rsi    ; <- buf size
    dec r9

    push rdi

.loop:
    call read_char ; -> rax
    test rax, rax  ; end of stream?
    jz .end
   
    ; check if read a space symbol
    xor rcx, rcx
    cmp al, 0x20
    sete cl
    sal rcx, 8
    cmp al, 0x09
    sete cl
    sal rcx, 8
    cmp al, 0x0A
    sete cl

    test rcx, rcx
    jz .write_to_buf

    test r8, r8
    jnz .end
    jmp .loop

.write_to_buf:
    ; if char is not space and enough place in buf - append to buf
    ; otherwise - rax=0
    
    cmp r8, r9
    je .bad_end
    mov BYTE [r10], al
    inc r8
    inc r10

    jmp .loop

.bad_end:
    pop rdx
    xor rax, rax
    ret

.end:
    mov BYTE [r10], 0
    mov rdx, r8
    pop rax
    
    ret            ; -> rax (buf|0), rdx (word length)

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint: ; <- rdi

.Start:
    xor rax, rax
    xor rcx, rcx
    mov r10, 10
.Num:
    xor r11, r11
    mov BYTE r11b, [rdi]
    sub r11b, 48
    jb .Stop
    cmp r11b, 9
    ja .Stop
    mul r10
    add rax, r11
    inc rcx
    inc rdi
    
    jmp .Num

.Stop:
    mov rdx, rcx
    ret


; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    mov BYTE r9b, [rdi]
    cmp r9b, 45
    jne .uint
    inc rdi

.uint:
    call parse_uint

    cmp r9b, 45
    jne .Stop
    neg rax
    inc rdx

.Stop:
    ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    ; rdi - string
    ; rsi - buf
    ; rdx - buf length
    push rdi
    call string_length ; -> rax
    pop rdi
    
    cmp rdx, rax
    jbe .small_buf
.loop:
    push word [rdi]
    pop word [rsi]
    inc rdi
    inc rsi
    dec rdx
    jbe .end
    jmp .loop

.small_buf:
    xor rax, rax
.end:
    ret ; -> rax
