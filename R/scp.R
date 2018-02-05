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
#' @param to path to an existing location on the local system (for download) or server
#' system (for upload) where the target `files` will be copied into.
#' downloaded files. Default copies everything into working directory for downloads,
#' and home directory for uploads.
#' @param verbose print file paths and sizes while copying
#' @param files path to directory or file to upload or download.
scp_download <- function(session = ssh_connect(), files, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(files))
  if(length(files) != 1)
    stop("For scp_download(), the 'files' parameter should be a single file or directory")
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
  .Call(C_scp_download_recursive, session, files, cb)
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
scp_upload <- function(session = ssh_connect(), files, to = ".", verbose = TRUE){
  assert_session(session)
  if(!file.exists(files))
    stop(sprintf("Directory not found: %s", files))
  stopifnot(is.character(to))
  filelist <- strsplit(list.files(files, recursive = TRUE, all.files = TRUE), "/")
  to <- file.path(to, basename(files))
  cb <- function(filevec){
    localpath <- do.call(file.path, as.list(c(files, filevec)))
    target <- do.call(file.path, as.list(c(to, filevec)))
    fsize <- file.info(localpath)$size
    if(verbose)
      cat(sprintf("%10d %s\n", fsize, target))
    readBin(localpath, raw(), fsize)
  }
  .Call(C_scp_write_recursive, session, filelist, to, cb)
}
