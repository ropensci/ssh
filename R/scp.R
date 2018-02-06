#' SCP (Secure Copy)
#'
#' Upload and download files to/from the SSH server via the scp protocol.
#' Directories in the `files` argument are automatically trasversed and
#' uploaded / downloaded recursively.
#'
#' Default path `to = "."` means that files get downloaded to the current
#' working directory and uploaded to the user home directory on the server.
#'
#' @export
#' @rdname scp
#' @name scp
#' @useDynLib ssh C_scp_download_recursive
#' @param to existing directory on the destination where `files` will be copied into
#' @param verbose print progress while copying files
#' @param files path to files or directory to transfer
#' @inheritParams ssh_connect
scp_download <- function(session = ssh_connect(), files, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(files))
  stopifnot(is.character(to))
  if(length(files) != 1)
    stop("For scp_download(), the 'files' parameter should be a single file or directory")
  cb <- function(data, filepath){
    target <- do.call(file.path, as.list(c(to, filepath)))
    if(verbose)
      cat(sprintf("%10d %s\n", length(data), target))
    if(is.null(data))
      return(dir.create(target, recursive = TRUE, showWarnings = FALSE))
    writeBin(data, target)
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
