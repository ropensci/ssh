#' SSH tunnel
#'
#' Create an SSH tunnel
#'
#' @export
#' @useDynLib tunnel C_blocking_tunnel
blocking_tunnel <- function(host = "dev.opencpu.org", port = 22, user = me()) {
  stopifnot(is.character(host))
  stopifnot(is.numeric(port))
  stopifnot(is.character(user))
  .Call(C_blocking_tunnel, host, port, user)
}

#' @export
me <- function(){
  Sys.info()[["user"]]
}
