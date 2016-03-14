#ifndef UW_FFI_PIPE_H
#define UW_FFI_PIPE_H

// track which ends have been closed on urweb side
//
typedef int uw_System_pipe[4]; // 0,1 are pipe ids, 2,3 is zero if pipe is closed

#define UW_SYSTEM_PIPE_INIT(x) { x[2] = 0; x[3]=0; }
#define UW_SYSTEM_PIPE_CLOSE_IN(x)  { close(x[1]); x[3] = 0; }
#define UW_SYSTEM_PIPE_CLOSE_OUT(x) { close(x[0]); x[2] = 0; }
#define UW_SYSTEM_PIPE_CLOSE(x) { \
  if (x[2]) { close(x[0]); x[2] = 0; } \
  if (x[3]) { close(x[1]); x[3] = 0; } \
}
#define UW_SYSTEM_PIPE_CREATE_OR_URWEB_ERROR(x) {               \
  if (pipe(x) >= 0){                                            \
      x[2] = 1;                                                 \
      x[3] = 1;                                                 \
  } else {                                                      \
    uw_error(ctx, FATAL, "failed creating pipe %s", cmd);       \
  }                                                             \
}                                                               \

#endif /* end of include guard: UW_FFI_PIPE_H */
