// Copyright 2026 Husarion sp. z o.o.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// One heap for libc and FreeRTOS: pvPortMalloc() is newlib malloc(), which
// grows from the end of .bss up to the main (ISR) stack. Same layout the
// STM32FreeRTOS library used (heap_useNewlib_ST), so the free RAM budget
// does not change with the build system.

#include <errno.h>
#include <malloc.h>
#include <reent.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

extern char end;              // linker script: first byte after .bss
extern char _estack;          // linker script: top of RAM
extern char _Min_Stack_Size;  // linker script: main stack reserve

static char* s_heap_end = &end;

static int schedulerRunning(void) {
  return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED;
}

void* _sbrk_r(struct _reent* r, ptrdiff_t incr) {
  char* limit = &_estack - (uintptr_t)&_Min_Stack_Size;
  // setup() runs on the main stack and may go deeper than its 1 KB
  // reserve; until the scheduler takes over, never hand out RAM below SP.
  char* const sp = (char*)__builtin_frame_address(0);
  if (!schedulerRunning() && sp < limit) limit = sp;
  if (s_heap_end + incr > limit) {
    r->_errno = ENOMEM;
    return (void*)-1;
  }
  char* const prev = s_heap_end;
  s_heap_end += incr;
  return prev;
}

void* _sbrk(ptrdiff_t incr) { return _sbrk_r(_impure_ptr, incr); }

// newlib's malloc and environ are not reentrant on their own. Suspending
// the scheduler is enough as long as no ISR allocates; vTaskSuspendAll()
// from an ISR would corrupt the kernel, so that resets instead.
void __malloc_lock(struct _reent* r) {
  (void)r;
  configASSERT(!xPortIsInsideInterrupt());
  if (schedulerRunning()) vTaskSuspendAll();
}

void __malloc_unlock(struct _reent* r) {
  (void)r;
  if (schedulerRunning()) (void)xTaskResumeAll();
}

void __env_lock(struct _reent* r) { __malloc_lock(r); }
void __env_unlock(struct _reent* r) { __malloc_unlock(r); }

void* pvPortMalloc(size_t size) {
  void* p = malloc(size);
#if configUSE_MALLOC_FAILED_HOOK == 1
  if (p == NULL) {
    extern void vApplicationMallocFailedHook(void);
    vApplicationMallocFailedHook();
  }
#endif
  return p;
}

void vPortFree(void* p) { free(p); }

size_t xPortGetFreeHeapSize(void) {
  const struct mallinfo mi = mallinfo();
  const char* const limit = &_estack - (uintptr_t)&_Min_Stack_Size;
  return (size_t)mi.fordblks + (size_t)(limit - s_heap_end);
}
