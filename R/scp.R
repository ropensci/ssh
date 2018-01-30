#' Upload / Download via SCP
#'
#' Upload and download files and directories to/from the SSH server.
#'
#' @export
#' @rdname scp
#' @name scp
#' @useDynLib ssh C_scp_read_file
#' @inheritParams ssh_tunnel
#' @param path path to the file on the server
scp_read_file <- function(session = ssh_connect(), path){
  assert_session(session)
  stopifnot(is.character(path))
  .Call(C_scp_read_file, session, path)
}
