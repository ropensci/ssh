.onAttach <- function(libname, pkg){
  packageStartupMessage(sprintf("Linking to libssh v%s", ssh::libssh_version()))
}
