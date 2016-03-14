#include <uw_ffi_pipe.h>
#include <uw_process.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <urweb.h>
#include <errno.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/wait.h>

#define UR_SYSTEM_POLL_TIMOUT_MS 100
// #define UR_SYSTEM_DEBUG

void cleanup(uw_Process_result reply){
  UW_SYSTEM_PIPE_CLOSE(reply->cmd_to_ur);
  UW_SYSTEM_PIPE_CLOSE(reply->ur_to_cmd);

  int status;
  if (reply->pid != -1 && reply->status == -1){
    // kill process
    kill(reply->pid, SIGTERM);
    // remove zombie process by getting pid status
   int rc = waitpid(reply->pid, &status, 0);
  }
  free (reply);
}


uw_Process_result uw_Process_exec(
    uw_context ctx,
    uw_Basis_string cmd,
    uw_Basis_blob stdin_,
    uw_Basis_int bufsize
){

  // initialize so that cleanup can be called, and register cleanup function
  // unfortunately can't use uw_malloc here !
  uw_Process_result reply = malloc(sizeof(uw_Process_result_struct));
  UW_SYSTEM_PIPE_INIT(reply->ur_to_cmd);
  UW_SYSTEM_PIPE_INIT(reply->cmd_to_ur);
  reply->pid = -1;
  reply->status = -1;
  reply->blob.size = 0;
  reply->bufsize = bufsize;
  reply->blob.data = (char *) uw_malloc(ctx, bufsize);
  uw_push_cleanup(ctx, (void (*)(void *))cleanup, reply);

  // try creating pipes
  UW_SYSTEM_PIPE_CREATE_OR_URWEB_ERROR(reply->ur_to_cmd);
  UW_SYSTEM_PIPE_CREATE_OR_URWEB_ERROR(reply->cmd_to_ur);

  int ignored;

  int pid = fork(); // local var required? TODO
  reply->pid = pid;
  if (pid == -1) {
    uw_error(ctx, FATAL, "failed forking child %s", cmd);
  } else if (reply->pid == 0) {
    // child - should be closing all fds ? but the ones being used? TODO
    close(reply->ur_to_cmd[1]);
    close(reply->cmd_to_ur[0]);

    // assign stdin
    close(0);
    ignored = dup(reply->ur_to_cmd[0]);
    close(reply->ur_to_cmd[0]);

    // set stdout
    close(1);
    ignored = dup(reply->cmd_to_ur[1]);
    // close(2);
    // ignored = dup(fd_from_cmd[1]);
    close(reply->cmd_to_ur[1]);

    // run command using /bin/sh shell - is there a shorter way to do this?
    char * argv[3];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = cmd;
    argv[3] = 0;
    execv("/bin/sh", argv);
  }

  // parent

  // close pipe ends which are not used:
  UW_SYSTEM_PIPE_CLOSE_IN ( reply->cmd_to_ur );
  UW_SYSTEM_PIPE_CLOSE_OUT( reply->ur_to_cmd );


  // feed stdin and wait for output until
  // - buffer is full
  // - timout occurs
  // - process dies

  int total_written = 0;
  int total_read = 0;

  while (1){
    fd_set rfds, wfds, efds;

    int max_fd = 0;
#define MY_FD_SET_546(x, set) { if (x > max_fd) max_fd = x; FD_SET(x, set); }
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    MY_FD_SET_546( reply->cmd_to_ur[0], &rfds )
    int all_written = total_written == stdin_.size;
    if (!all_written){
      MY_FD_SET_546( reply->ur_to_cmd[1], &wfds )
    }

    struct timeval tv;
    tv.tv_sec  = UR_SYSTEM_POLL_TIMOUT_MS / 1000;
    tv.tv_usec = (UR_SYSTEM_POLL_TIMOUT_MS % 1000) * (1000000/1000);

    int ret = select(max_fd +1, &rfds, &wfds, &efds, &tv);
#ifdef UR_SYSTEM_DEBUG
    printf("select result is %d\n", ret);
#endif
    if (ret < 0)
      uw_error(ctx, FATAL, "select failed while running cmd %s %d %s", cmd, errno, strerror(errno));

    if (FD_ISSET( reply->cmd_to_ur[0], &rfds )){
      ret--;
      size_t read_bytes = read(reply->cmd_to_ur[0], &reply->blob.data[total_read], bufsize - total_read);
#ifdef UR_SYSTEM_DEBUG
      printf("read %d\n", read_bytes);
#endif
      reply->blob.size += read_bytes;
      total_read += read_bytes;
       
      if (read_bytes == 0 || bufsize == total_read){
        // process closed stdout or our buffer is full. kill process and wait for status
        kill(reply->pid, SIGTERM);
        int status;
        int rc = waitpid(pid, &status, 0);
        if (rc == -1){
          // what to do? retry?
          uw_error(ctx, FATAL, "waiting for pid failed, pid: %d", reply->pid);
        } else if (rc == reply->pid){
          // expected
          reply->status = WEXITSTATUS(status);
        } else {
          uw_error(ctx, FATAL, "waitpid unexpected return value %d", rc);
        }
        break;
      }
    }

    // write STDIN to porcess, close stdin if all data has been written
    if (!all_written && FD_ISSET( reply->ur_to_cmd[1], &wfds )){
      ret--;
      size_t written = write(reply->ur_to_cmd[1], &stdin_.data[total_written], stdin_.size - total_written );
#ifdef UR_SYSTEM_DEBUG
      printf("written %d\n", written);
#endif
      total_written += written;
      if (total_written == stdin_.size){
        UW_SYSTEM_PIPE_CLOSE_IN(reply->ur_to_cmd);
      }
    }

    // TIMOUT ?
    uw_check_deadline(ctx);
  }
#undef MY_FD_SET_546

  uw_pop_cleanup(ctx);

  return reply;
}

uw_Basis_int uw_Process_status(uw_context ctx, uw_Process_result reply){
  return reply->status;
}
uw_Basis_blob uw_Process_blob(uw_context ctx, uw_Process_result reply){
  return reply->blob;
}
uw_Basis_bool uw_Process_buf_full(uw_context ctx, uw_Process_result reply){
  return !!(reply->bufsize == reply->blob.size);
}

uw_Basis_string   uw_Process_blobText (uw_context ctx, uw_Basis_blob blob){
  char * c = uw_malloc(ctx, blob.size+1);
  char * write = c;
  int i;
  for (i = 0; i < blob.size; i++) {
    *write =  blob.data[i];
    if (*write == '\0')
      *write = '\n';
    *write++;
  }
  *write=0;
  return c;
}
