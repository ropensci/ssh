context("ssh-tunnel")

test_that("Tunnel: recycle port in process",{
  # Start tunnel twice!
  pid <- sys::r_background(std_out = FALSE, std_err = FALSE, args = c("-e",
    paste0("library(ssh);x=ssh_connect('dev.opencpu.org');ssh_tunnel(x,target='ds043942.mongolab.com:43942');",
    "ssh_tunnel(x,target='ds043942.mongolab.com:43942');ssh_disconnect(x)")))

  # Connect and disconnect twice on the same parent process
  for(i in 1:2){
    Sys.sleep(3)
    expect_equal(sys::exec_status(pid, wait = FALSE), NA_integer_)
    con <- mongolite::mongo("mtcars", url = "mongodb://readwrite:test@localhost:5555/jeroen_test")
    expect_is(con, 'mongo')
    rm(con); gc();
  }

  Sys.sleep(3)
  expect_error(mongolite::mongo("mtcars", url = "mongodb://readwrite:test@localhost:5555/jeroen_test"))
  expect_equal(sys::exec_status(pid, wait = FALSE), 0)
})


test_that("Tunnel: free port on exit", {
  for(i in 1:3){
    pid <- sys::r_background(std_out = interactive(), std_err = interactive(), args = c("-e",
      "x=ssh::ssh_connect('dev.opencpu.org');ssh::ssh_tunnel(x,target='ds043942.mongolab.com:43942');ssh::ssh_disconnect(x)"))
    Sys.sleep(3)
    expect_equal(sys::exec_status(pid, wait = FALSE), NA_integer_)
    Sys.sleep(3)
    con <- mongolite::mongo("mtcars", url = "mongodb://readwrite:test@localhost:5555/jeroen_test")
    expect_is(con, 'mongo')
    if(con$count())
      con$drop()
    con$insert(mtcars)
    out <- con$find()
    expect_equal(out, mtcars)
    con$drop()
    if(i == 2 && Sys.info()[["sysname"]] != "Windows"){
      # Test that process can also be interrupted
      tools::pskill(pid, tools::SIGINT)
    } else {
      con$disconnect()
    }
    expect_equal(sys::exec_status(pid), 0)
    expect_error(con$count())

    #not needed but just in case:
    tools::pskill(pid, tools::SIGKILL)
  }
})
