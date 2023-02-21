#include <string.h>

#define printConsole(S) (printf("%s\n", S))

#define debug(S) (printf("[Debug] %s:%d %s\n", __FILE__, __LINE__, S))

#define handleErrno(MSG) (printf("[Error] %s:%d %s, %s (%d)\n", __FILE__, __LINE__, MSG, strerror(errno), errno))
#define handleError(MSG) (printf("[Error] %s:%d %s\n", __FILE__, __LINE__, MSG))