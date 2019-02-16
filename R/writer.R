# Borrowed from the 'curl' package
file_writer <- function(path){
  path <- normalizePath(path, mustWork = FALSE)
  fp <- new_file_writer(path)
  structure(function(data = raw(), close = FALSE){
    stopifnot(is.raw(data))
    write_file_writer(fp, data, as.logical(close))
  }, class = "file_writer")
}

#' @useDynLib ssh R_ssh_new_file_writer
new_file_writer <- function(path){
  .Call(R_ssh_new_file_writer, path)
}

#' @useDynLib ssh R_ssh_write_file_writer
write_file_writer <- function(fp, data, close){
  .Call(R_ssh_write_file_writer, fp, data, close)
}

#' @useDynLib ssh R_ssh_total_writers
total_writers <- function(){
  .Call(R_ssh_total_writers)
}
