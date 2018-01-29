#' @export
#' @rdname ssh
#' @useDynLib ssh C_ssh_exec
#' @param command The command to execute (e.g. `ls ~/ -al | grep -i reports`).
exec <- function(session = ssh(), command = "whoami") {
  assert_session(session)
  stopifnot(is.character(command))
  .Call(C_ssh_exec, session, command)
}
