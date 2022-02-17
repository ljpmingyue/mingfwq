#ifndef __MINGFWQ_UTIL_H__
#define __MINGFWQ_UTIL_H__

#include <string>
#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#       define MINGFWQ_LICKLY(x)        __builtin_expect(!!(x),1)
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#       define MINGFWQ_UNLICKLY(x)      __builtin_expect(!!(x),0)
#else
#       define MINGFWQ_LICKLY(x)        (x)
#       define MINGFWQ_UNLICKLY(x)      (x)
#endif

//断言宏 ASSERT
#define MINGFWQ_ASSERT(x) \
{\
        if(MINGFWQ_LICKLY(!(x))) {  \
            MINGFWQ_LOG_ERROR(MINGFWQ_LOG_ROOT() ) << "ASSERTON: "#x  \
                << "\nbacktrace:\n" \
                << mingfwq::BacktraceToString(100, 2, "    "); \
                assert(x); \
        }\
}

#define MINGFWQ_ASSERT2(x, w)\
{\
        if(MINGFWQ_LICKLY(!(x))) {\
            MINGFWQ_LOG_ERROR(MINGFWQ_LOG_ROOT() ) << "ASSERTON: "#x \
                << "\n" << w\
                << "\nbacktrace:\n"  \
                << mingfwq::BacktraceToString(100, 2, "    ");  \
                assert(x);\
        }\
}




#endif