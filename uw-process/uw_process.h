#include <urweb.h>
#include "uw_ffi_pipe.h"

typedef struct {
  uw_System_pipe ur_to_cmd;
  uw_System_pipe cmd_to_ur;
  int pid;
  int status;
  int bufsize;
  uw_Basis_blob blob;
} uw_Process_result_struct;


typedef uw_Process_result_struct *uw_Process_result;

uw_Process_result uw_Process_exec( uw_context ctx, uw_Basis_string cmd, uw_Basis_blob stdin_, uw_Basis_int bufsize);
uw_Basis_int      uw_Process_status      (uw_context ctx, uw_Process_result result);
uw_Basis_blob     uw_Process_blob        (uw_context ctx, uw_Process_result result);
uw_Basis_bool     uw_Process_buf_full    (uw_context ctx, uw_Process_result result);
uw_Basis_string   uw_Process_blobText (uw_context ctx, uw_Basis_blob blob);
