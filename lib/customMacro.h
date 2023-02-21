#include <string.h>

#define debug(S) (printf("%s line: %d, message: %s\n", __FILE__, __LINE__, S))

#define handleErrno(MSG) (printf("%s:%d %s, %s (%d)\n", __FILE__, __LINE__, MSG, strerror(errno), errno))
#define handleError(MSG) (printf("%s:%d %s\n", __FILE__, __LINE__, MSG))