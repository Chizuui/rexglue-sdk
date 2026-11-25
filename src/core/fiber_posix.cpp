/**
 * @file        rex/core/fiber_posix.cpp
 * @brief       POSIX backend for rex::thread::Fiber (makecontext/swapcontext)
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#if defined(__APPLE__) && !defined(_XOPEN_SOURCE)
// Darwin hides the deprecated ucontext API unless a POSIX/XSI feature level
// is requested before <ucontext.h> is included. fiber.h includes ucontext.h,
// so the feature macro must be set in this translation unit first.
#define _XOPEN_SOURCE 700
#endif

#include <rex/platform.h>
#if REX_PLATFORM_LINUX || REX_PLATFORM_MAC

#include <rex/thread/fiber.h>

#include <cassert>

#if REX_PLATFORM_ANDROID && defined(__aarch64__)
// Bionic has no ucontext functions; minimal implementations are defined at
// the bottom of this file. Declare them here so the Fiber code can call them.
extern "C" int getcontext(ucontext_t* uc);
extern "C" int setcontext(const ucontext_t* uc);
extern "C" int swapcontext(ucontext_t* ouc, const ucontext_t* uc);
extern "C" void makecontext(ucontext_t* uc, void (*func)(), int argc, ...);
#endif

namespace rex::thread {

thread_local Fiber* Fiber::tls_current_ = nullptr;

Fiber* Fiber::ConvertCurrentThread() {
  auto* f = new Fiber();
  if (getcontext(&f->context_) == -1) {
    delete f;
    return nullptr;
  }
  f->is_thread_fiber_ = true;
  tls_current_ = f;
  return f;
}

Fiber* Fiber::Create(size_t stack_size, void (*entry)(void*), void* arg) {
  auto* f = new Fiber();
  f->entry_ = entry;
  f->arg_ = arg;
  f->stack_.resize(stack_size);

  if (getcontext(&f->context_) == -1) {
    delete f;
    return nullptr;
  }
  f->context_.uc_stack.ss_sp = f->stack_.data();
  f->context_.uc_stack.ss_size = f->stack_.size();
  f->context_.uc_link = nullptr;
  // Trampoline reads entry_/arg_ from tls_current_ — no pointer splitting needed.
  makecontext(&f->context_, &Fiber::Trampoline, 0);
  return f;
}

/*static*/ void Fiber::Trampoline() {
  // tls_current_ was updated by SwitchTo before swapcontext returned here.
  Fiber* f = tls_current_;
  f->entry_(f->arg_);
}

void Fiber::SwitchTo(Fiber* target) {
  Fiber* from = tls_current_;
  tls_current_ = target;
  swapcontext(&from->context_, &target->context_);
}

void Fiber::Destroy() {
  // Thread fibers are destroyed from the owning thread itself.
  if (is_thread_fiber_) {
    tls_current_ = nullptr;
  } else {
    assert(this != tls_current_ && "Destroy called on the currently running fiber");
  }
  // No POSIX equivalent of ConvertFiberToThread; stack_ is freed by the vector destructor.
  delete this;
}

}  // namespace rex::thread

#endif  // REX_PLATFORM_LINUX || REX_PLATFORM_MAC

#if REX_PLATFORM_ANDROID && defined(__aarch64__)
// Bionic ships ucontext_t but no getcontext/makecontext/swapcontext/
// setcontext, so the fiber backend above has nothing to call. Minimal
// aarch64 implementations over the kernel sigcontext layout
// (uc_mcontext.regs[0..30] = x0..x30, .sp, .pc; __reserved[] carries the
// FPSIMD context). Signal masks are left untouched: fibers never depend on
// per-fiber masks here. Modeled on the musl aarch64 ucontext approach.
// Only fiber_posix.cpp uses these, and only on Android.
#include <ucontext.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

// Offsets into ucontext_t (from the NDK headers).
constexpr size_t kMcOffset = offsetof(ucontext_t, uc_mcontext);
constexpr size_t kRegsOffset = kMcOffset + offsetof(mcontext_t, regs);
constexpr size_t kSpOffset = kMcOffset + offsetof(mcontext_t, sp);
constexpr size_t kPcOffset = kMcOffset + offsetof(mcontext_t, pc);
constexpr size_t kReservedOffset = kMcOffset + offsetof(mcontext_t, __reserved);

}  // namespace

extern "C" __attribute__((naked)) int getcontext(ucontext_t* /*uc*/) {
  __asm__ volatile(
      // x0 = uc. Save callee-saved GPRs x19-x30.
      "add x1, x0, %[regs]\n"
      "stp x19, x20, [x1, #152]\n"
      "stp x21, x22, [x1, #168]\n"
      "stp x23, x24, [x1, #184]\n"
      "stp x25, x26, [x1, #200]\n"
      "stp x27, x28, [x1, #216]\n"
      "stp x29, x30, [x1, #232]\n"
      // Save SP and PC (LR = return address).
      "mov x2, sp\n"
      "str x2, [x0, %[sp]]\n"
      "str x30, [x0, %[pc]]\n"
      // FPSIMD context in __reserved: header + fpsr/fpcr + v0-v31.
      "add x1, x0, %[res]\n"
      "movz w2, #0x8001\n"
      "movk w2, #0x4650, lsl #16\n"
      "str w2, [x1, #0]\n"
      "mov w2, #528\n"
      "str w2, [x1, #4]\n"
      "mrs x2, fpsr\n"
      "str w2, [x1, #8]\n"
      "mrs x2, fpcr\n"
      "str w2, [x1, #12]\n"
      "add x1, x1, #16\n"
      "stp q0, q1, [x1, #0]\n"
      "stp q2, q3, [x1, #32]\n"
      "stp q4, q5, [x1, #64]\n"
      "stp q6, q7, [x1, #96]\n"
      "stp q8, q9, [x1, #128]\n"
      "stp q10, q11, [x1, #160]\n"
      "stp q12, q13, [x1, #192]\n"
      "stp q14, q15, [x1, #224]\n"
      "stp q16, q17, [x1, #256]\n"
      "stp q18, q19, [x1, #288]\n"
      "stp q20, q21, [x1, #320]\n"
      "stp q22, q23, [x1, #352]\n"
      "stp q24, q25, [x1, #384]\n"
      "stp q26, q27, [x1, #416]\n"
      "stp q28, q29, [x1, #448]\n"
      "stp q30, q31, [x1, #480]\n"
      "mov x0, #0\n"
      "ret\n"
      :
      : [regs] "i"(kRegsOffset), [sp] "i"(kSpOffset), [pc] "i"(kPcOffset),
        [res] "i"(kReservedOffset)
      : "x1", "x2", "memory");
}

extern "C" __attribute__((naked)) int swapcontext(ucontext_t* /*ouc*/,
                                                   const ucontext_t* /*uc*/) {
  __asm__ volatile(
      // x0 = ouc, x1 = uc. Save current context into ouc.
      "add x2, x0, %[regs]\n"
      "stp x19, x20, [x2, #152]\n"
      "stp x21, x22, [x2, #168]\n"
      "stp x23, x24, [x2, #184]\n"
      "stp x25, x26, [x2, #200]\n"
      "stp x27, x28, [x2, #216]\n"
      "stp x29, x30, [x2, #232]\n"
      "mov x3, sp\n"
      "str x3, [x0, %[sp]]\n"
      "str x30, [x0, %[pc]]\n"
      "add x2, x0, %[res]\n"
      "movz w3, #0x8001\n"
      "movk w3, #0x4650, lsl #16\n"
      "str w3, [x2, #0]\n"
      "mov w3, #528\n"
      "str w3, [x2, #4]\n"
      "mrs x3, fpsr\n"
      "str w3, [x2, #8]\n"
      "mrs x3, fpcr\n"
      "str w3, [x2, #12]\n"
      "add x2, x2, #16\n"
      "stp q0, q1, [x2, #0]\n"
      "stp q2, q3, [x2, #32]\n"
      "stp q4, q5, [x2, #64]\n"
      "stp q6, q7, [x2, #96]\n"
      "stp q8, q9, [x2, #128]\n"
      "stp q10, q11, [x2, #160]\n"
      "stp q12, q13, [x2, #192]\n"
      "stp q14, q15, [x2, #224]\n"
      "stp q16, q17, [x2, #256]\n"
      "stp q18, q19, [x2, #288]\n"
      "stp q20, q21, [x2, #320]\n"
      "stp q22, q23, [x2, #352]\n"
      "stp q24, q25, [x2, #384]\n"
      "stp q26, q27, [x2, #416]\n"
      "stp q28, q29, [x2, #448]\n"
      "stp q30, q31, [x2, #480]\n"
      // Restore FPSIMD state from uc.
      "add x2, x1, %[res]\n"
      "ldr w3, [x2, #8]\n"
      "msr fpsr, x3\n"
      "ldr w3, [x2, #12]\n"
      "msr fpcr, x3\n"
      "add x2, x2, #16\n"
      "ldp q0, q1, [x2, #0]\n"
      "ldp q2, q3, [x2, #32]\n"
      "ldp q4, q5, [x2, #64]\n"
      "ldp q6, q7, [x2, #96]\n"
      "ldp q8, q9, [x2, #128]\n"
      "ldp q10, q11, [x2, #160]\n"
      "ldp q12, q13, [x2, #192]\n"
      "ldp q14, q15, [x2, #224]\n"
      "ldp q16, q17, [x2, #256]\n"
      "ldp q18, q19, [x2, #288]\n"
      "ldp q20, q21, [x2, #320]\n"
      "ldp q22, q23, [x2, #352]\n"
      "ldp q24, q25, [x2, #384]\n"
      "ldp q26, q27, [x2, #416]\n"
      "ldp q28, q29, [x2, #448]\n"
      "ldp q30, q31, [x2, #480]\n"
      // Restore callee-saved GPRs, SP, then jump to the saved PC.
      "add x2, x1, %[regs]\n"
      "ldp x19, x20, [x2, #152]\n"
      "ldp x21, x22, [x2, #168]\n"
      "ldp x23, x24, [x2, #184]\n"
      "ldp x25, x26, [x2, #200]\n"
      "ldp x27, x28, [x2, #216]\n"
      "ldp x29, x30, [x2, #232]\n"
      "ldr x2, [x1, %[sp]]\n"
      "mov sp, x2\n"
      "ldr x30, [x1, %[pc]]\n"
      "mov x0, #0\n"
      "ret\n"
      :
      : [regs] "i"(kRegsOffset), [sp] "i"(kSpOffset), [pc] "i"(kPcOffset),
        [res] "i"(kReservedOffset)
      : "x2", "x3", "memory");
}

extern "C" __attribute__((naked)) int setcontext(const ucontext_t* /*uc*/) {
  __asm__ volatile(
      // x0 = uc. Restore FPSIMD state, GPRs, SP, then jump to the saved PC.
      "add x1, x0, %[res]\n"
      "ldr w2, [x1, #8]\n"
      "msr fpsr, x2\n"
      "ldr w2, [x1, #12]\n"
      "msr fpcr, x2\n"
      "add x1, x1, #16\n"
      "ldp q0, q1, [x1, #0]\n"
      "ldp q2, q3, [x1, #32]\n"
      "ldp q4, q5, [x1, #64]\n"
      "ldp q6, q7, [x1, #96]\n"
      "ldp q8, q9, [x1, #128]\n"
      "ldp q10, q11, [x1, #160]\n"
      "ldp q12, q13, [x1, #192]\n"
      "ldp q14, q15, [x1, #224]\n"
      "ldp q16, q17, [x1, #256]\n"
      "ldp q18, q19, [x1, #288]\n"
      "ldp q20, q21, [x1, #320]\n"
      "ldp q22, q23, [x1, #352]\n"
      "ldp q24, q25, [x1, #384]\n"
      "ldp q26, q27, [x1, #416]\n"
      "ldp q28, q29, [x1, #448]\n"
      "ldp q30, q31, [x1, #480]\n"
      "add x1, x0, %[regs]\n"
      "ldp x19, x20, [x1, #152]\n"
      "ldp x21, x22, [x1, #168]\n"
      "ldp x23, x24, [x1, #184]\n"
      "ldp x25, x26, [x1, #200]\n"
      "ldp x27, x28, [x1, #216]\n"
      "ldp x29, x30, [x1, #232]\n"
      "ldr x1, [x0, %[sp]]\n"
      "mov sp, x1\n"
      "ldr x30, [x0, %[pc]]\n"
      "mov x0, #0\n"
      "ret\n"
      :
      : [regs] "i"(kRegsOffset), [sp] "i"(kSpOffset), [pc] "i"(kPcOffset),
        [res] "i"(kReservedOffset)
      : "x1", "x2", "memory");
}

extern "C" void makecontext(ucontext_t* uc, void (*func)(), int argc, ...) {
  // Only argc == 0 is supported; fiber_posix.cpp always passes 0.
  // (Variadic args are ignored.)
  (void)argc;
  // Fresh FP state: valid empty FPSIMD context.
  auto* fp = reinterpret_cast<fpsimd_context*>(reinterpret_cast<char*>(uc) +
                                               kReservedOffset);
  fp->head.magic = FPSIMD_MAGIC;
  fp->head.size = sizeof(fpsimd_context);
  fp->fpsr = 0;
  fp->fpcr = 0;
  std::memset(fp->vregs, 0, sizeof(fp->vregs));
  // Fresh GPR state; SP = top of the caller-provided stack (16-byte aligned),
  // PC/LR = entry point.
  std::memset(&uc->uc_mcontext.regs, 0, sizeof(uc->uc_mcontext.regs));
  uintptr_t sp = reinterpret_cast<uintptr_t>(uc->uc_stack.ss_sp) +
                 uc->uc_stack.ss_size;
  sp &= ~uintptr_t(15);
  uc->uc_mcontext.sp = sp;
  uc->uc_mcontext.pc = reinterpret_cast<uint64_t>(func);
  uc->uc_mcontext.regs[30] = reinterpret_cast<uint64_t>(func);
}
#endif  // REX_PLATFORM_ANDROID && defined(__aarch64__)
