#' SSH tunnel
#'
#' Create an SSH tunnel
#'
#' @export
#' @rdname ssh
#' @useDynLib tunnel C_blocking_tunnel
#' @param ssh an ssh server string of the form `[user@]hostname[:@port]`
#' @param target target sever string of the form `hostname[:port]`
#' @param listen port on which to listen for incoming connections
#' @param passwd string or callback function to supply ssh password
tunnel <- function(ssh = "dev.opencpu.org:22", target = "ds043942.mongolab.com:43942", listen = 5555, passwd = askpass) {
  stopifnot(is.character(ssh))
  stopifnot(is.character(target))
  stopifnot(is.character(passwd) || is.function(passwd))
  ssh <- parse_host(ssh, default_port = 22)
  target <- parse_host(target)
  .Call(C_blocking_tunnel, ssh$host, ssh$port, ssh$user, target$host, target$port, listen, passwd)
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
