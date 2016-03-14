type process_result (* C internal type *)

(* run command using /bin/sh returning stdout. (Other programming languages call this system or process ..
* urs timeout (deadline) is honored
*)
val exec : string -> blob -> int -> transaction process_result (* cmd, stdin, bufsize *)

(* access result *)
val status: process_result -> int
val blob: process_result -> blob
val buf_full: process_result -> bool (* true indicates that the whole buffer was filled - then the process was killed *)
(* blob_to_str replaces zero bytes by \n because 0 can't be represented as in C strings *)
val blobText: blob -> string
