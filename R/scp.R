#' SCP (Secure Copy)
#'
#' Upload and download files to/from the SSH server via the scp protocol.
#' Directories in the `files` argument are automatically traversed and
#' uploaded / downloaded recursively.
#'
#' Note that the syntax is slightly different from the `scp` command line
#' tool because the `to` parameter is always a target *directory* where
#' all `files` will be copied __into__. If `to` does not exist, it will be
#' created.
#'
#' The `files` parameter in [scp_upload()] is vectorised hence all files
#' and directories will be recursively uploaded __into__ the `to` directory.
#' For [scp_download()] the `files` parameter must be a single string which
#' may contain wildcards.
#'
#' The default path `to = "."` means that files get downloaded to the current
#' working directory and uploaded to the user home directory on the server.
#'
#' @export
#' @rdname scp
#' @name scp
#' @family ssh
#' @useDynLib ssh C_scp_download_recursive
#' @param to existing directory on the destination where `files` will be copied into
#' @param verbose print progress while copying files
#' @param files path to files or directory to transfer
#' @inheritParams ssh_connect
#' @examples \dontrun{
#' # recursively upload files and directories
#' session <- ssh_connect("dev.opencpu.org")
#' files <- c(R.home("doc"), R.home("COPYING"))
#' scp_upload(session, files, to = "~/target")
#'
#' # download it back
#' scp_download(session, "~/target/*", to = tempdir())
#'
#' # delete it from the server
#' ssh_exec_wait(session, command = "rm -Rf ~/target")
#' ssh_disconnect(session)
#' }
scp_download <- function(session, files, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(files))
  to <- normalizePath(to, mustWork = TRUE)
  if(length(files) != 1)
    stop("For scp_download(), the 'files' parameter should be a single file or directory")
  cb <- function(data, filepath){
    target <- do.call(file.path, as.list(c(to, filepath)))
    if(verbose)
      cat(sprintf("%10.0f %s\n", as.double(length(data)), target))
    if(is.null(data))
      return(dir.create(target, recursive = TRUE, showWarnings = FALSE))
    writer <- file_writer(target)
    writer(data = data, close = TRUE)
  }
  .Call(C_scp_download_recursive, session, files, cb)
}

#' @rdname scp
#' @export
#' @useDynLib ssh C_scp_write_recursive
scp_upload <- function(session, files, to = ".", verbose = TRUE){
  assert_session(session)
  stopifnot(is.character(files))
  stopifnot(is.character(to))
  info <- list_all(files)
  .Call(C_scp_write_recursive, session, info$local, info$size, info$path, to, verbose)
}

list_all <- function(files, hidden = FALSE){
  dfs <- lapply(files, function(path){
    if(!file.exists(path) & !dir.exists(path))
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
scp_write_file <- function(session, path, data){
  assert_session(session)
  stopifnot(is.character(path))
  if(is.character(data))
    data <- charToRaw(paste(data, collapse = "\n"))
  stopifnot(is.raw(data))
  .Call(C_scp_write_file, session, path, data)
}

#' @useDynLib ssh C_scp_read_file
scp_read_file <- function(session, path){
  assert_session(session)
  stopifnot(is.character(path))
  .Call(C_scp_read_file, session, path)
}
