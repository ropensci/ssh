#' Execute Remote Command
#'
#' Run a command or script on the host while streaming stdout and stderr directly
#' to the client.
#'
#' The [ssh_exec_wait()] function is the remote equivalent of the local [sys::exec_wait()].
#' It runs a command or script on the ssh server and streams stdout and stderr to the client
#' to a file or connection. When done it returns the exit status for the remotely executed command.
#'
#' Similarly [ssh_exec_internal()] is a small wrapper analogous to [sys::exec_internal()].
#' It buffers all stdout and stderr output into a raw vector and returns it in a list along with
#' the exit status. By default this function raises an error if the remote command was unsuccessful.
#'
#' @export
#' @rdname ssh_exec
#' @name ssh_exec
#' @family ssh
#' @useDynLib ssh C_ssh_exec
#' @inheritParams ssh_connect
#' @param command The command or script to execute
#' @param std_out callback function, filename, or connection object to handle stdout stream
#' @param std_err callback function, filename, or connection object to handle stderr stream
#' @examples \dontrun{
#' session <- ssh_connect("dev.opencpu.org")
#' ssh_exec_wait(command = c(
#'   'curl -O https://cran.r-project.org/src/contrib/jsonlite_1.5.tar.gz',
#'   'R CMD check jsonlite_1.5.tar.gz',
#'   'rm -f jsonlite_1.5.tar.gz'
#' ))}
ssh_exec_wait <- function(session = ssh_connect(), command = "whoami", std_out = stdout(), std_err = stderr()) {
  assert_session(session)
  stopifnot(is.character(command))
  command <- paste(command, collapse = "\n")

  # Convert TRUE or filepath into connection objects
  std_out <- if(isTRUE(std_out) || identical(std_out, "")){
    stdout()
  } else if(is.character(std_out)){
    file(normalizePath(std_out, mustWork = FALSE))
  } else std_out

  std_err <- if(isTRUE(std_err) || identical(std_err, "")){
    stderr()
  } else if(is.character(std_err)){
    std_err <- file(normalizePath(std_err, mustWork = FALSE))
  } else std_err

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
  if(is.na(status))
    return(invisible())
  status
}


#' @export
#' @param error automatically raise an error if the exit status is non-zero
#' @rdname ssh_exec
#' @useDynLib ssh C_ssh_exec
ssh_exec_internal <- function(session = ssh_connect(), command = "whoami", error = TRUE){
  outcon <- rawConnection(raw(0), "r+")
  on.exit(close(outcon), add = TRUE)
  errcon <- rawConnection(raw(0), "r+")
  on.exit(close(errcon), add = TRUE)
  status <- ssh_exec_wait(session, command, std_out = outcon, std_err = errcon)
  if (isTRUE(error) && !identical(status, 0L))
    stop(sprintf("Executing '%s' failed with status %d",
                 command, status))
  list(status = status, stdout = rawConnectionValue(outcon),
       stderr = rawConnectionValue(errcon))
}
