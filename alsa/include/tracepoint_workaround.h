#include_next <linux/tracepoint.h>

#if !defined(DECLARE_EVENT_CLASS) && !defined(TRACE_HEADER_MULTI_READ)
#define HACKED_TRACE_DEFINE_EVENT
#define DECLARE_EVENT_CLASS(name, proto, args, str, assign, print)
#define DEFINE_EVENT(temp, name, proto, args) \
        static inline void trace_##name(proto) {}
#elif defined(HACKED_TRACE_DEFINE_EVENT) && defined(TRACE_HEADER_MULTI_READ)
#undef DEFINE_EVENT
#define DEFINE_EVENT(temp, name, proto, args)
#endif
