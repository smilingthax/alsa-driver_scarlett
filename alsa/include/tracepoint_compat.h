#ifndef __TRACEPOINT_COMPAT_H
#define __TRACEPOINT_COMPAT_H

#define DECLARE_EVENT_CLASS(name, proto, args, str, assign, print)
#define DECLARE_TRACE(name, proto, args) \
	static inline void trace_##name(proto) {}
#define DEFINE_EVENT(temp, name, proto, args) \
	DECLARE_TRACE(name, proto, args)

#endif /* __TRACEPOINT_COMPAT_H */
