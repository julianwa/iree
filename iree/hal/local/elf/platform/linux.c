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
#include "iree/base/tracing.h"
#include "iree/hal/local/elf/platform.h"

#if defined(IREE_PLATFORM_ANDROID) || defined(IREE_PLATFORM_LINUX)

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

//==============================================================================
// Memory subsystem information and control
//==============================================================================

void iree_memory_query_info(iree_memory_info_t* out_info) {
  memset(out_info, 0, sizeof(*out_info));

  int page_size = sysconf(_SC_PAGESIZE);
  out_info->normal_page_size = page_size;
  out_info->normal_page_granularity = page_size;

  // Large pages arent't currently used so we aren't introducing the build goo
  // to detect and use them yet.
  // https://linux.die.net/man/3/gethugepagesizes
  // http://manpages.ubuntu.com/manpages/bionic/man3/gethugepagesize.3.html
  // Would be:
  //   #include <hugetlbfs.h>
  //   out_info->large_page_granularity = gethugepagesize();
  out_info->large_page_granularity = page_size;

  out_info->can_allocate_executable_pages = true;
}

void iree_memory_jit_context_begin() {}

void iree_memory_jit_context_end() {}

//==============================================================================
// Virtual address space manipulation
//==============================================================================

static int iree_memory_access_to_prot(iree_memory_access_t access) {
  int prot = 0;
  if (access & IREE_MEMORY_ACCESS_READ) prot |= PROT_READ;
  if (access & IREE_MEMORY_ACCESS_WRITE) prot |= PROT_WRITE;
  if (access & IREE_MEMORY_ACCESS_EXECUTE) prot |= PROT_EXEC;
  return prot;
}

iree_status_t iree_memory_view_reserve(iree_memory_view_flags_t flags,
                                       iree_host_size_t total_length,
                                       void** out_base_address) {
  *out_base_address = NULL;
  IREE_TRACE_ZONE_BEGIN(z0);

  int mmap_prot = PROT_NONE;
  int mmap_flags = MAP_PRIVATE | MAP_ANON | MAP_NORESERVE;

  iree_status_t status = iree_ok_status();
  void* base_address = mmap(NULL, total_length, mmap_prot, mmap_flags, -1, 0);
  if (base_address == MAP_FAILED) {
    status = iree_make_status(iree_status_code_from_errno(errno),
                              "mmap reservation failed");
  }

  *out_base_address = base_address;
  IREE_TRACE_ZONE_END(z0);
  return status;
}

void iree_memory_view_release(void* base_address,
                              iree_host_size_t total_length) {
  IREE_TRACE_ZONE_BEGIN(z0);

  // NOTE: return value ignored as this is a shutdown path.
  munmap(base_address, total_length);

  IREE_TRACE_ZONE_END(z0);
}

iree_status_t iree_memory_view_commit_ranges(
    void* base_address, iree_host_size_t range_count,
    const iree_byte_range_t* ranges, iree_memory_access_t initial_access) {
  IREE_TRACE_ZONE_BEGIN(z0);

  int mmap_prot = iree_memory_access_to_prot(initial_access);
  int mmap_flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;

  iree_status_t status = iree_ok_status();
  for (iree_host_size_t i = 0; i < range_count; ++i) {
    void* range_start = (void*)iree_page_align_start(
        (uintptr_t)base_address + ranges[i].offset, getpagesize());
    void* result =
        mmap(range_start, ranges[i].length, mmap_prot, mmap_flags, -1, 0);
    if (result == MAP_FAILED) {
      status = iree_make_status(iree_status_code_from_errno(errno),
                                "mmap commit failed");
      break;
    }
  }

  IREE_TRACE_ZONE_END(z0);
  return status;
}

iree_status_t iree_memory_view_protect_ranges(void* base_address,
                                              iree_host_size_t range_count,
                                              const iree_byte_range_t* ranges,
                                              iree_memory_access_t new_access) {
  IREE_TRACE_ZONE_BEGIN(z0);

  int mmap_prot = iree_memory_access_to_prot(new_access);

  iree_status_t status = iree_ok_status();
  for (iree_host_size_t i = 0; i < range_count; ++i) {
    void* range_start = (void*)iree_page_align_start(
        (uintptr_t)base_address + ranges[i].offset, getpagesize());
    int ret = mprotect(range_start, ranges[i].length, mmap_prot);
    if (ret != 0) {
      status = iree_make_status(iree_status_code_from_errno(errno),
                                "mprotect failed");
      break;
    }
  }

  IREE_TRACE_ZONE_END(z0);
  return status;
}

void iree_memory_view_flush_icache(void* base_address,
                                   iree_host_size_t length) {
#if defined __has_builtin
#if __has_builtin(__builtin___clear_cache)
#define CLEAR_CACHE(start, end) __builtin___clear_cache(start, end)
#endif
#endif

#if !defined(CLEAR_CACHE)
#if defined(IREE_COMPILER_GCC) || defined(IREE_COMPILER_CLANG)
#define CLEAR_CACHE(start, end) __clear_cache(start, end)
#else
#error "no instruction cache clear implementation"
#endif  // IREE_COMPILER_*
#endif  // CLEAR_CACHE

  CLEAR_CACHE(base_address, base_address + length);

#undef CLEAR_CACHE
}

#endif  // IREE_PLATFORM_*
