#' Create SSH tunnel
#'
#' Opens a port on your machine and tunnel all traffic to a custom target host via the
#' SSH server, for example to connect with a database server behind a firewall.
#'
#' This function blocks while the tunnel is active. Use the tunnel by connecting to
#' `localhost:5555` from a separate process. Each tunnel can only be used once and will
#' automatically be closed when the client disconnects. It is intended to tunnel a single
#' connection, not as a long running proxy server.
#'
#' @export
#' @rdname ssh_tunnel
#' @family ssh
#' @useDynLib ssh C_blocking_tunnel
#' @inheritParams ssh_connect
#' @param port integer of local port on which to listen for incoming connections
#' @param target string with target host and port to connect to via ssh tunnel
ssh_tunnel <- function(session, port = 5555, target = "towel.blinkenlights.nl:23") {
  assert_session(session)
  stopifnot(is.numeric(port))
  target <- parse_host(target)
  .Call(C_blocking_tunnel, session, as.integer(port), target$host, target$port)
  invisible()
}
