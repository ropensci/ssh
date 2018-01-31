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

#' @rdname scp
#' @export
#' @useDynLib ssh C_scp_write_file
#' @param data string or raw vector with data to write to the file
scp_write_file <- function(session = ssh_connect(), path, data){
  assert_session(session)
  stopifnot(is.character(path))
  if(is.character(data))
    data <- charToRaw(data)
  stopifnot(is.raw(data))
  .Call(C_scp_write_file, session, path, data)
}
