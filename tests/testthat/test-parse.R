test_that("parsing hostname works", {
  expect_equal(parse_host("test.server.com", 22),
               list(user = me(), host = "test.server.com", port = 22))
  expect_equal(parse_host("jerry@server.com", 22),
               list(user = 'jerry', host = "server.com", port = 22))
  expect_equal(parse_host("jerry@gmail.com@server.com", 22),
               list(user = 'jerry@gmail.com', host = "server.com", port = 22))
})
