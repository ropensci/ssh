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
blocking_tunnel <- function(host = "dev.opencpu.org", port = 22, user = me()) {
  stopifnot(is.character(host))
  stopifnot(is.numeric(port))
  stopifnot(is.character(user))
  .Call(C_blocking_tunnel, host, port, user)
}

#' @export
#' @rdname ssh
me <- function(){
  Sys.info()[["user"]]
}
