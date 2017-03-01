# Build against static libraries from rwinlib
if(!file.exists("../windows/libssh-0.7.4/include/libssh/libssh.h")){
  if(getRversion() < "3.3.0") stop("This package requires R 3.3 or newer")
  download.file("https://github.com/rwinlib/libssh/archive/v0.7.4.zip", "lib.zip", quiet = TRUE)
  dir.create("../windows", showWarnings = FALSE)
  unzip("lib.zip", exdir = "../windows")
  unlink("lib.zip")
}
