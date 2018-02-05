#' Upload / Download via SCP
#'
#' Upload / download files and directories to/from the SSH server.
#'
#' @export
#' @rdname scp
#' @name scp
#' @useDynLib ssh C_scp_download_recursive
#' @param to path to an existing location on the local system (for download) or server
#' system (for upload) where the target `files` will be copied into.
#' downloaded files. Default copies everything into working directory for downloads,
#' and home directory for uploads.
#' @param verbose print file paths and sizes while copying
#' @param files path to directory or file to upload or download
#' @inheritParams ssh_tunnel
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
#' @useDynLib ssh C_scp_write_recursive
scp_upload <- function(session = ssh_connect(), files, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(files))
  stopifnot(is.character(to))
  info <- list_all(files)
  .Call(C_scp_write_recursive, session, info$local, info$size, info$path, to, verbose)
}

list_all <- function(files, hidden = FALSE){
  dfs <- lapply(files, function(path){
    if(!file.exists(path))
      stop(sprintf("File or directory does not exist: %s", path))
    if(file.info(path)$isdir){
      rel <- list.files(path, recursive = TRUE, all.files = hidden, include.dirs = TRUE)
      abs <- file.path(path, rel)
      target <- file.path(basename(normalizePath(path, mustWork = TRUE)), rel)
    } else {
      target <- basename(path)
      abs <- path
    }
    data.frame(local = normalizePath(abs), target = target, stringsAsFactors = FALSE)
  })
  df <- do.call(rbind.data.frame, c(dfs, make.row.names = FALSE, stringsAsFactors = FALSE, deparse.level = 0))
  df <- cbind(file.info(df$local), df)
  path <- strsplit(df$target, "/")
  path[df$isdir] <- lapply(path[df$isdir], append, NA_character_)
  df$path = path
  df
}

#' @useDynLib ssh C_scp_write_file
scp_write_file <- function(session = ssh_connect(), path, data){
  assert_session(session)
  stopifnot(is.character(path))
  if(is.character(data))
    data <- charToRaw(paste(data, collapse = "\n"))
  stopifnot(is.raw(data))
  .Call(C_scp_write_file, session, path, data)
}

#' @useDynLib ssh C_scp_read_file
scp_read_file <- function(session = ssh_connect(), path){
  assert_session(session)
  stopifnot(is.character(path))
  .Call(C_scp_read_file, session, path)
}
