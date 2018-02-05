context("scp")

content <- function(x){
  unname(tools::md5sum(x))
}

compare_dir <- function(ssh, source_dir){
  res <- ssh_exec_internal(ssh, paste('ls', source_dir))
  lst <- strsplit(rawToChar(res$stdout), "\\s+")[[1]]
  expect_equal(lst, list.files(source_dir))
}

test_that("Upload and download a directory", {
  ssh <- ssh_connect()
  expect_equal(ssh_exec_internal(ssh, command = "rm -Rf ~/testdir")$status, 0)
  source_dir <- 'testdir'
  scp_upload(ssh, files = source_dir, to = "~", verbose = FALSE)
  compare_dir(ssh, source_dir)
  compare_dir(ssh, file.path(source_dir, 'subdir', 'subsubdir'))
  scp_download(ssh, files = source_dir, to = tempdir(), verbose = FALSE)
  target_dir <- file.path(tempdir(), 'testdir')
  expect_equal(list.files(source_dir, recursive = TRUE), list.files(target_dir, recursive = TRUE))
  v1 <- list.files(source_dir, full.names = TRUE, recursive = TRUE)
  v2 <- list.files(target_dir, full.names = TRUE, recursive = TRUE)
  expect_equal(content(v1), content(v2))
  unlink(target_dir, recursive = TRUE)
})
