#' @export
#' @rdname ssh
#' @useDynLib ssh C_ssh_exec
#' @param command The command to execute (e.g. `ls ~/ -al | grep -i reports`).
#' @param std_out callback function or connection to handle stdout stream from host
#' @param std_err callback function or connection to handle stderr stream from host
ssh_exec <- function(session = ssh_connect(), command = "whoami", std_out = stdout(), std_err = stderr()) {
  assert_session(session)
  stopifnot(is.character(command))
  command <- paste(command, collapse = "\n")
  outfun <- if(inherits(std_out, "connection")){
    if(!isOpen(std_out)){
      open(std_out, "wb")
      on.exit(close(std_out), add = TRUE)
    }
    if(identical(summary(std_out)$text, "text")){
      function(x){
        cat(rawToChar(x), file = std_out)
        flush(std_out)
      }
    } else {
      function(x){
        writeBin(x, con = std_out)
        flush(std_out)
      }
    }
  }
  errfun <- if(inherits(std_err, "connection")){
    if(!isOpen(std_err)){
      open(std_err, "wb")
      on.exit(close(std_err), add = TRUE)
    }
    if(identical(summary(std_err)$text, "text")){
      function(x){
        cat(rawToChar(x), file = std_err)
        flush(std_err)
      }
    } else {
      function(x){
        writeBin(x, con = std_err)
        flush(std_err)
      }
    }
  }
  status <- .Call(C_ssh_exec, session, command, outfun, errfun)
  if(is.na(out))
    return(invisible())
  status
}
