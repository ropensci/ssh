context("scp")

content <- function(x){
  unname(tools::md5sum(x))
}

test_that("uploading a directory", {
  ssh <- ssh_connect()
  ssh_exec_internal(ssh, command = "rm -Rf ~/testdir")
  source_dir <- 'testdir'
  scp_upload(ssh, files = source_dir, to = "~", verbose = FALSE)
  scp_download(ssh, files = source_dir, to = tempdir(), verbose = FALSE)
  target_dir <- file.path(tempdir(), 'testdir')
  expect_equal(list.files(source_dir, recursive = TRUE), list.files(target_dir, recursive = TRUE))
  v1 <- list.files(source_dir, full.names = TRUE, recursive = TRUE)
  v2 <- list.files(target_dir, full.names = TRUE, recursive = TRUE)
  expect_equal(content(v1), content(v2))
  unlink(target_dir, recursive = TRUE)
})
