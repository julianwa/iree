; Copyright 2021 Google LLC
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;      https://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

; Microsoft x64 calling convention:
; https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention
; Arguments:
;   RCX, RDX, R8, R9, [stack]...
; Results:
;   RAX
; Non-volatile:
;   RBX, RBP, RDI, RSI, RSP, R12, R13, R14, R15, and XMM6-XMM15
;
; System V AMD64 ABI (used in IREE):
; https://github.com/hjl-tools/x86-psABI/wiki/x86-64-psABI-1.0.pdf
; Arguments:
;   RDI, RSI, RDX, RCX, R8, R9, [stack]...
; Results:
;   RAX, RDX

_TEXT SEGMENT

; void iree_elf_call_v_v(const void* symbol_ptr)
iree_elf_call_v_v PROC
  push rbp
  mov rbp, rsp
  push rbx
  push rdi
  push rsi
  push r12
  push r13
  push r14
  push r15

  ; RCX = symbol_ptr
  call rcx

  pop r15
  pop r14
  pop r13
  pop r12
  pop rsi
  pop rdi
  pop rbx
  leave
  ret
iree_elf_call_v_v ENDP

; void* iree_elf_call_p_i(const void* symbol_ptr, int a0)
iree_elf_call_p_i PROC
  push rbp
  mov rbp, rsp
  push rbx
  push rdi
  push rsi
  push r12
  push r13
  push r14
  push r15

  ; RCX = symbol_ptr
  ; RDX = a0
  mov rdi, rdx
  call rcx

  pop r15
  pop r14
  pop r13
  pop r12
  pop rsi
  pop rdi
  pop rbx
  leave
  ret
iree_elf_call_p_i ENDP

; int iree_elf_call_i_pp(const void* symbol_ptr, void* a0, void* a1)
iree_elf_call_i_pp PROC
  push rbp
  mov rbp, rsp
  push rbx
  push rdi
  push rsi
  push r12
  push r13
  push r14
  push r15

  ; RCX = symbol_ptr
  ; RDX = a0
  ; R8 = a1
  mov rdi, rdx
  mov rsi, r8
  call rcx

  pop r15
  pop r14
  pop r13
  pop r12
  pop rsi
  pop rdi
  pop rbx
  leave
  ret
iree_elf_call_i_pp ENDP

_TEXT ENDS
END
