#ifdef LOG_TRACE
#define TRACE_CORO(expr) (std::cout << expr << std::endl)
#define TRACE_PARSE(expr) (std::cout << expr << std::endl)
#else
#define TRACE_CORO(expr) (static_cast<void>(0))
#define TRACE_PARSE(expr) (static_cast<void>(0))
#endif
