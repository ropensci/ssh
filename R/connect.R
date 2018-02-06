#' SSH Client
#'
#' Create an ssh session using `ssh_connect()`. Then use the other functions to
#' run commands or create a tunnel via this ssh session.
#'
#' The client first tries to authenticate using a private key, if available. When
#' this fails it tries challenge-response (interactive) and password auth. The
#' `passwd` parameter can be used to provide a passphrase or a callback function to
#' ask prompt the user for the passphrase when needed.
#'
#' __Windows users:__ the private key must be in OpenSSH PEM format. If you open it in
#' a text editor it must start with something like `-----BEGIN RSA PRIVATE KEY-----`.
#' To use your Putty PKK key, open it in the *PuttyGen* utility and go to
#' *Conversions->Export OpenSSH*.
#'
#' @export
#' @useDynLib ssh C_start_session
#' @rdname ssh
#' @aliases ssh
#' @param host an ssh server string of the form `[user@]hostname[:@port]`
#' @param passwd either a string or a callback function for password prompt
#' @param keyfile path to private key file. Must be in OpenSSH format (putty PKK files
#' won't work). If `NULL` the default user key (e.g `~/.ssh/id_rsa`) is tried.
#' @family ssh
#' @examples \dontrun{
#' ssh_exec_wait(command = c(
#'   'curl -O https://cran.r-project.org/src/contrib/jsonlite_1.5.tar.gz',
#'   'R CMD check jsonlite_1.5.tar.gz',
#'   'rm -f jsonlite_1.5.tar.gz'
#' ))}
ssh_connect <- function(host = "dev.opencpu.org:22", keyfile = NULL, passwd = askpass) {
  stopifnot(is.character(host))
  stopifnot(is.character(passwd) || is.function(passwd))
  details <- parse_host(host, default_port = 22)
  if(length(keyfile))
    keyfile <- normalizePath(keyfile, mustWork = TRUE)
  .Call(C_start_session, details$host, details$port, details$user, keyfile, passwd)
}

#' @export
#' @rdname ssh
#' @useDynLib ssh C_disconnect_session
#' @param session ssh connection created with [ssh_connect()]
ssh_disconnect <- function(session){
  .Call(C_disconnect_session, session)
  invisible()
}

parse_host <- function(str, default_port){
  stopifnot(is.character(str) && length(str) == 1)
  str <- sub("^@", "", str)
  str <- sub(":$", "", str)
  x <- strsplit(str, "@", fixed = TRUE)[[1]]
  if(length(x) > 2) stop("host string contains multiple '@' characters")
  host <- if(length(x) > 1){
    user <- x[1]
    x[2]
  } else {
    user <- me()
    x[1]
  }
  x <- strsplit(host, ":", fixed = TRUE)[[1]]
  if(length(x) > 2) stop("host string contains multiple ':' characters")
  host <- x[1]
  port <- if(length(x) > 1){
    as.numeric(x[2])
  } else {
    as.numeric(default_port)
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

askpass <- function(prompt = "Please enter your password: "){
  FUN <- getOption("askpass", readline)
  FUN(prompt)
}
