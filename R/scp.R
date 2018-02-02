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

#' @export
#' @rdname scp
#' @name scp
#' @useDynLib ssh C_scp_download_recursive
#' @param to either a path to a local directory or a callback function to handle
#' downloaded files. Default copies everything to current working directory.
#' @param verbose print file paths and sizes while copying
scp_read_recursive <- function(session = ssh_connect(), path, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(path))
  cb <- if(is.function(to)){
    function(data, filepath){
      target <- do.call(file.path, as.list(filepath))
      if(verbose)
        cat(sprintf("%10d %s\n", length(data), target))
      to(data, target);
    }
  } else if(is.character(to)){
    function(data, filepath){
      target <- do.call(file.path, as.list(c(to, filepath)))
      if(verbose)
        cat(sprintf("%10d %s\n", length(data), target))
      if(!identical(basename(target), target) && !dir.exists(dirname(target)))
        dir.create(dirname(target), recursive = TRUE)
      writeBin(data, target)
    }
  } else {
    stop("Parameter 'to' must be a filepath or callback function")
  }
  .Call(C_scp_download_recursive, session, path, cb)
}

#' @rdname scp
#' @export
#' @useDynLib ssh C_scp_write_file
#' @param data string or raw vector with data to write to the file
scp_write_file <- function(session = ssh_connect(), path, data){
  assert_session(session)
  stopifnot(is.character(path))
  if(is.character(data))
    data <- charToRaw(paste(data, collapse = "\n"))
  stopifnot(is.raw(data))
  .Call(C_scp_write_file, session, path, data)
}
