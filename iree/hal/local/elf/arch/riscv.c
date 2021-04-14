// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "iree/base/target_platform.h"
#include "iree/hal/local/elf/arch.h"

#if defined(IREE_ARCH_RISCV_32) || defined(IREE_ARCH_RISCV_64)

// Documentation:
// https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md

//==============================================================================
// ELF machine type/ABI
//==============================================================================

bool iree_elf_arch_is_valid(const iree_elf_ehdr_t* ehdr) {
  return ehdr->e_machine == 0xF3;  // EM_RISCV / 243
}

//==============================================================================
// ELF relocations
//==============================================================================

enum {
  IREE_ELF_R_RISCV_NONE = 0,
  IREE_ELF_R_RISCV_32 = 1,
  IREE_ELF_R_RISCV_64 = 2,
  IREE_ELF_R_RISCV_RELATIVE = 3,
  IREE_ELF_R_RISCV_COPY = 4,
  IREE_ELF_R_RISCV_JUMP_SLOT = 5,
};

#if defined(IREE_ARCH_RISCV_32)
static iree_status_t iree_elf_arch_riscv_apply_rela(
    iree_elf_relocation_state_t* state, iree_host_size_t rela_count,
    const iree_elf_rela_t* rela_table) {
  for (iree_host_size_t i = 0; i < rela_count; ++i) {
    const iree_elf_rela_t* rela = &rela_table[i];
    uint32_t type = IREE_ELF_R_TYPE(rela->r_info);
    if (type == 0) continue;

    // TODO(benvanik): support imports by resolving from the import table.
    iree_elf_addr_t sym_addr = 0;
    if (IREE_ELF_R_SYM(rela->r_info) != 0) {
      return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                              "symbol-relative relocations not implemented");
    }

    iree_elf_addr_t instr_ptr =
        (iree_elf_addr_t)state->vaddr_bias + rela->r_offset;
    switch (type) {
      case IREE_ELF_R_RISCV_NONE:
        break;
      case IREE_ELF_R_RISCV_32:
        *(uint32_t*)instr_ptr = (uint32_t)(sym_addr + rela->r_addend);
        break;
      case IREE_ELF_R_RISCV_JUMP_SLOT:
        *(uint32_t*)instr_ptr = (uint32_t)sym_addr;
        break;
      case IREE_ELF_R_RISCV_RELATIVE:
        *(uint32_t*)instr_ptr = (uint32_t)(state->vaddr_bias + rela->r_addend);
        break;
      default:
        return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                                "unimplemented riscv32 relocation type %08X",
                                type);
    }
  }
  return iree_ok_status();
}
#else
static iree_status_t iree_elf_arch_riscv_apply_rela(
    iree_elf_relocation_state_t* state, iree_host_size_t rela_count,
    const iree_elf_rela_t* rela_table) {
  for (iree_host_size_t i = 0; i < rela_count; ++i) {
    const iree_elf_rela_t* rela = &rela_table[i];
    uint32_t type = IREE_ELF_R_TYPE(rela->r_info);
    if (type == 0) continue;

    // TODO(benvanik): support imports by resolving from the import table.
    iree_elf_addr_t sym_addr = 0;
    if (IREE_ELF_R_SYM(rela->r_info) != 0) {
      return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                              "symbol-relative relocations not implemented");
    }

    iree_elf_addr_t instr_ptr =
        (iree_elf_addr_t)state->vaddr_bias + rela->r_offset;
    switch (type) {
      case IREE_ELF_R_RISCV_NONE:
        break;
      case IREE_ELF_R_RISCV_32:
        *(uint32_t*)instr_ptr = (uint32_t)(sym_addr + rela->r_addend);
        break;
      case IREE_ELF_R_RISCV_64:
        *(uint64_t*)instr_ptr = (uint64_t)(sym_addr + rela->r_addend);
        break;
      case IREE_ELF_R_RISCV_JUMP_SLOT:
        *(uint64_t*)instr_ptr = (uint64_t)sym_addr;
        break;
      case IREE_ELF_R_RISCV_RELATIVE:
        *(uint64_t*)instr_ptr = (uint64_t)(state->vaddr_bias + rela->r_addend);
        break;
      default:
        return iree_make_status(IREE_STATUS_UNIMPLEMENTED,
                                "unimplemented riscv64 relocation type %08X",
                                type);
    }
  }
  return iree_ok_status();
}
#endif  // IREE_ARCH_RISCV_*

iree_status_t iree_elf_arch_apply_relocations(
    iree_elf_relocation_state_t* state) {
  // Gather the relevant relocation tables.
  iree_host_size_t rela_count = 0;
  const iree_elf_rela_t* rela_table = NULL;
  for (iree_host_size_t i = 0; i < state->dyn_table_count; ++i) {
    const iree_elf_dyn_t* dyn = &state->dyn_table[i];
    switch (dyn->d_tag) {
      case IREE_ELF_DT_RELA:
        rela_table =
            (const iree_elf_rela_t*)(state->vaddr_bias + dyn->d_un.d_ptr);
        break;
      case IREE_ELF_DT_RELASZ:
        rela_count = dyn->d_un.d_val / sizeof(iree_elf_rela_t);
        break;

      case IREE_ELF_DT_REL:
      case IREE_ELF_DT_RELSZ:
        return iree_make_status(IREE_STATUS_FAILED_PRECONDITION,
                                "unsupported DT_REL relocations");
      default:
        // Ignored.
        break;
    }
  }
  if (!rela_table) rela_count = 0;

  if (rela_count > 0) {
    IREE_RETURN_IF_ERROR(
        iree_elf_arch_riscv_apply_rela(state, rela_count, rela_table));
  }

  return iree_ok_status();
}

//==============================================================================
// Cross-ABI function calls
//==============================================================================

void iree_elf_call_v_v(const void* symbol_ptr) {
  typedef void (*ptr_t)(void);
  ((ptr_t)symbol_ptr)();
}

void* iree_elf_call_p_i(const void* symbol_ptr, int a0) {
  typedef void* (*ptr_t)(int);
  return ((ptr_t)symbol_ptr)(a0);
}

int iree_elf_call_i_pp(const void* symbol_ptr, void* a0, void* a1) {
  typedef int (*ptr_t)(void*, void*);
  return ((ptr_t)symbol_ptr)(a0, a1);
}

#endif  // IREE_ARCH_RISCV_*
