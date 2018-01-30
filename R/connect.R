#' SSH Client
#'
#' Create an ssh session using `ssh_connect()`. Then use the other functions to
#' run commands or create a tunnel via this ssh session.
#'
#' @export
#' @useDynLib ssh C_start_session
#' @aliases ssh
#' @param host an ssh server string of the form `[user@]hostname[:@port]`
#' @param passwd either a string or a callback function for password prompt
#' @param keyfile path to private key. If `NULL` the default user key is tried.
#' @family ssh
#' @examples \dontrun{
#' ssh_exec(command = c(
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
  Sys.info()[["user"]]
}

askpass <- function(prompt = "Please enter your password: "){
  FUN <- getOption("askpass", readline)
  FUN(prompt)
}
