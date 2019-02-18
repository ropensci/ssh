.onAttach <- function(pkg, lib){
  packageStartupMessage(sprintf("Linking to libssh v%s", ssh::libssh_version()))
}
