context("ssh-exec")

ssh <- ssh_connect('dev.opencpu.org')

test_that("Execute a command", {
  out <- ssh_exec_internal(ssh, 'whoami')
  expect_equal(out$status, 0)
  expect_equal(sys::as_text(out$stdout), 'jeroen')
})
ssh_disconnect(ssh)
