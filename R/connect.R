#' SSH Client
#'
#' Create an ssh session using `ssh_connect()`. The session can be used to execute
#' commands, scp files or setup a tunnel.
#'
#' The client first tries to authenticate using a private key, either from ssh-agent
#' or `/.ssh/id_rsa` in the user home directory. If this fails it falls back on
#' challenge-response (interactive) and password auth if allowed by the server. The
#' `passwd` parameter can be used to provide a passphrase or a callback function to
#' ask prompt the user for the passphrase when needed.
#'
#' The session will automatically be disconnected when the session object is removed
#' or when R exits but you can also use [ssh_disconnect()].
#'
#' __Windows users:__ the private key must be in OpenSSH PEM format. If you open it in
#' a text editor the first line must be: `-----BEGIN RSA PRIVATE KEY-----`.
#' To convert a Putty PKK key, open it in the *PuttyGen* utility and go to
#' *Conversions -> Export OpenSSH*.
#'
#' @export
#' @useDynLib ssh C_start_session
#' @rdname ssh
#' @aliases ssh
#' @importFrom askpass askpass
#' @param host an ssh server string of the form `[user@]hostname[:port]`. An ipv6
#' hostname should be wrapped in brackets like this: `[2001:db8::1]:80`.
#' @param passwd either a string or a callback function for password prompt
#' @param keyfile path to private key file. Must be in OpenSSH format (see details)
#' @param verbose either TRUE/FALSE or a value between 0 and 4 indicating log level:
#' 0: no logging, 1: only warnings, 2: protocol, 3: packets or 4: full stack trace.
#' @family ssh
#' @examples \dontrun{
#' session <- ssh_connect("dev.opencpu.org")
#' ssh_exec_wait(session, command = "whoami")
#' ssh_disconnect(session)
#' }
ssh_connect <- function(host, keyfile = NULL, passwd = askpass, verbose = FALSE) {
  if(is.logical(verbose))
    verbose <- 2 * verbose # TRUE == 'protocol'
  stopifnot(verbose %in% 0:4)
  stopifnot(is.character(host))
  stopifnot(is.character(passwd) || is.function(passwd))
  details <- parse_host(host, default_port = 22)
  if(length(keyfile))
    keyfile <- normalizePath(keyfile, mustWork = TRUE)
  .Call(C_start_session, details$host, details$port, details$user, keyfile, passwd, verbose)
}

#' @rdname ssh
#' @export ssh_session_info ssh_info
#' @aliases ssh_info
#' @useDynLib ssh C_ssh_info
ssh_session_info <- function(session){
  assert_session(session)
  out <- .Call(C_ssh_info, session)
  structure(out, names = c("user", "host", "identity", "port", "connected", "sha1"))
}

# For backward compatibility
ssh_info <- ssh_session_info

#' @export
#' @rdname ssh
#' @useDynLib ssh C_disconnect_session
#' @param session ssh connection created with [ssh_connect()]
ssh_disconnect <- function(session){
  assert_session(session)
  .Call(C_disconnect_session, session)
  invisible()
}

parse_host <- function(str, default_port){
  stopifnot(is.character(str) && length(str) == 1)
  str <- sub("^@", "", str)
  str <- sub(":$", "", str)
  x <- strsplit(str, "@", fixed = TRUE)[[1]]
  host <- if(length(x) > 1){
    user <- paste(utils::head(x, -1), collapse = '@')
    utils::tail(x, 1)
  } else {
    user <- me()
    x[1]
  }

  # Check for port
  m <- gregexpr(':[0-9]+$', host, ignore.case = TRUE)
  port <- sub("^:", "", regmatches(host, m)[[1]])
  port <- if(length(port)){
    as.numeric(port)
  } else {
    as.numeric(default_port)
  }
  host <- sub(':[0-9]+$', '', host)
  if(grepl("^\\[.*\\]$", host)){
    host <- sub('\\]$', '', sub('^\\[', "", host))
  } else if(grepl(":", host)){
    stop(sprintf("Invalid hostname '%s'. Use brackets for ipv6 hosts like this: [2001:db8::1]:22", host))
  }

  list(
    user = user,
    host = host,
    port = port
  )
}

me <- function(){
  tolower(Sys.info()[["user"]])
}

assert_session <- function(x){
  if(!inherits(x, "ssh_session"))
    stop('Argument "session" must be an ssh session', call. = FALSE)
}

#' @export
print.ssh_session <- function(x, ...){
  info <- ssh_session_info(x)
  cat(sprintf("<ssh session>\nconnected: %s@%s:%d\nserver: %s\n", info$user, info$host, info$port, info$sha1))
}

#' @export
#' @useDynLib ssh C_libssh_version
#' @rdname ssh
libssh_version <- function(){
  .Call(C_libssh_version)
}
