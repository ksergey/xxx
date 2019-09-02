// ------------------------------------------------------------
// Copyright 2019-present Sergey Kovalevich <inndie@gmail.com>
// ------------------------------------------------------------

#ifndef KSERGEY_compiler_300819143652
#define KSERGEY_compiler_300819143652

#ifndef XXX_ALWAYS_INLINE
#define XXX_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#ifndef XXX_LIKELY
#define XXX_LIKELY(x) __builtin_expect(static_cast<bool>(x), true)
#endif

#ifndef XXX_UNLIKELY
#define XXX_UNLIKELY(x) __builtin_expect(static_cast<bool>(x), false)
#endif

#endif /* KSERGEY_compiler_300819143652 */
