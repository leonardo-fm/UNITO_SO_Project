#include <string.h>

#define printConsole(S) (printf("%s\n", S))

#ifdef DEBUG
#define debugId(S, ID) (printf("[Debug][%d] %s:%d %s\n", ID, __FILE__, __LINE__, S))
#define debug(S) (printf("[Debug] %s:%d %s\n", __FILE__, __LINE__, S))
#else
#define debugId(S, ID)
#define debug(S)
#endif

#define handleErrnoId(MSG, ID) (printf("[Error][Entity:%d] %s:%d %s, %s (%d)\n", ID, __FILE__, __LINE__, MSG, strerror(errno), errno))
#define handleErrno(MSG) (printf("[Error] %s:%d %s, %s (%d)\n", __FILE__, __LINE__, MSG, strerror(errno), errno))

#define handleErrorId(MSG, ID) (printf("[Error][Entity:%d] %s:%d %s\n", ID, __FILE__, __LINE__, MSG))
#define handleError(MSG) (printf("[Error] %s:%d %s\n", __FILE__, __LINE__, MSG))