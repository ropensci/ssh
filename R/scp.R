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
#' @param from existing directory to copy from
scp_read_recursive <- function(session = ssh_connect(), from, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(from))
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
  .Call(C_scp_download_recursive, session, from, cb)
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


#' @rdname scp
#' @export
#' @useDynLib ssh C_scp_write_recursive
scp_write_recursive <- function(session = ssh_connect(), from, to = ".", verbose = TRUE){
  assert_session(session)
  if(!file.exists(from))
    stop(sprintf("Directory not found: %s", from))
  stopifnot(is.character(to))
  files <- strsplit(list.files(from, recursive = TRUE, all.files = TRUE), "/")
  cb <- function(filevec){
    localpath <- do.call(file.path, as.list(c(from, filevec)))
    target <- do.call(file.path, as.list(c(to, filevec)))
    fsize <- file.info(localpath)$size
    if(verbose)
      cat(sprintf("%10d %s\n", fsize, target))
    readBin(localpath, raw(), fsize)
  }
  .Call(C_scp_write_recursive, session, files, to, cb)
}
