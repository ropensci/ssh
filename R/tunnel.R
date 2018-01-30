#' SSH tunnel
#'
#' Create an SSH tunnel
#'
#' @export
#' @rdname ssh
#' @useDynLib ssh C_blocking_tunnel
#' @param session existing ssh connnection created with [ssh]
#' @param port integer of local port on which to listen for incoming connections
#' @param target string with target host and port to connnect to via ssh tunnel
tunnel <- function(session = ssh(), port = 5555, target = "ds043942.mongolab.com:43942") {
  assert_session(session)
  stopifnot(is.numeric(port))
  target <- parse_host(target)
  .Call(C_blocking_tunnel, session, as.integer(port), target$host, target$port)
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
  Sys.info()[["user"]]
}

askpass <- function(prompt = "Please enter your password: "){
  FUN <- getOption("askpass", readline)
  FUN(prompt)
}

assert_session <- function(x){
  if(!inherits(x, "ssh_session"))
    stop('Argument "session" must be an ssh session', call. = FALSE)
}
