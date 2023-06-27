# Download libssh + dependencies
VERSION <- commandArgs(TRUE)
if(!file.exists(sprintf("../windows/libssh-%s/include/libssh/libssh.h", VERSION))){
  download.file(sprintf("https://github.com/rwinlib/libssh/archive/v%s.zip", VERSION), "lib.zip", quiet = TRUE)
  dir.create("../windows", showWarnings = FALSE)
  unzip("lib.zip", exdir = "../windows")
  unlink("lib.zip")
}
