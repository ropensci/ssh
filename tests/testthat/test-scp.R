context("scp")

ssh <- ssh_connect()

content <- function(x){
  unname(tools::md5sum(x))
}

compare_dir <- function(ssh, source_dir){
  res <- ssh_exec_internal(ssh, paste('ls', source_dir))
  lst <- strsplit(rawToChar(res$stdout), "\\s+")[[1]]
  expect_equal(lst, list.files(source_dir))
}

test_that("Upload and download single files", {
  tmp <- tempfile(fileext = '.csv')
  svn <- R.home("SVN-REVISION")
  write.csv(iris, tmp)
  scp_upload(ssh, c(tmp, svn), verbose = FALSE)
  scp_download(ssh, basename(tmp), verbose = FALSE)
  scp_download(ssh, basename(svn), verbose = FALSE)
  expect_equal(content(tmp), content(basename(tmp)))
  expect_equal(content(svn), content(basename(svn)))
  expect_equal(ssh_exec_internal(ssh, command = paste('rm -f', basename(tmp), basename(svn)))$status, 0)
  unlink(basename(svn))
  unlink(basename(tmp))
})

test_that("Upload and download a directory", {
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
