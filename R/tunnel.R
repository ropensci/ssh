#' SSH tunnel
#'
#' Create an SSH tunnel
#'
#' @export
#' @rdname ssh
#' @useDynLib tunnel C_blocking_tunnel
#' @param host a hostname
#' @param port a number
#' @param user the username to authenticate with
#' @param passwd string or callback function to supply password
blocking_tunnel <- function(host = "dev.opencpu.org", port = 22, user = me(), passwd = askpass) {
  stopifnot(is.character(host))
  stopifnot(is.numeric(port))
  stopifnot(is.character(user))
  stopifnot(is.character(passwd) || is.function(passwd))
  .Call(C_blocking_tunnel, host, port, user, passwd)
}

#' @export
#' @rdname ssh
me <- function(){
  Sys.info()[["user"]]
}

askpass <- function(prompt = "Please enter your password: "){
  FUN <- getOption("askpass", readline)
  FUN(prompt)
}
