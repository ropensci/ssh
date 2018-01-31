# SSH

[![Build Status](https://travis-ci.org/ropensci/ssh.svg?branch=master)](https://travis-ci.org/ropensci/ssh)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/ropensci/ssh?branch=master&svg=true)](https://ci.appveyor.com/project/jeroen/ssh)
[![Coverage Status](https://codecov.io/github/ropensci/ssh/coverage.svg?branch=master)](https://codecov.io/github/ropensci/ssh?branch=master)
[![CRAN_Status_Badge](http://www.r-pkg.org/badges/version/ssh)](http://cran.r-project.org/package=ssh)
[![CRAN RStudio mirror downloads](http://cranlogs.r-pkg.org/badges/ssh)](http://cran.r-project.org/web/packages/ssh/index.html)

> Secure Shell (SSH) Client for R

## Installation

Binary packages for __OS-X__ or __Windows__ can be installed directly from CRAN:

```r
install.packages("ssh")
```

Installation from source on Linux requires [`libssh`](https://www.libssh.org/) (which is __not__ `libssh2`). On __Debian__ or __Ubuntu__ use [libssh-dev](https://packages.debian.org/testing/libssh-dev):

```
sudo apt-get install -y libssh-dev
```

On __Fedora__ we need [libssh-devel](https://apps.fedoraproject.org/packages/libssh-devel):

```
sudo yum install libssh-devel
````

On __CentOS / RHEL__ we install [libssh-devel](https://apps.fedoraproject.org/packages/libssh-devel) via EPEL:

```
sudo yum install epel-release
sudo yum install libssh-devel
```

On __OS-X__ use [libssh](https://github.com/Homebrew/homebrew-core/blob/master/Formula/libssh.rb) from Homebrew:

```
brew install libssh
```

## Getting Started

First create an ssh session by connecting to an SSH server. You can either use private key or passphrase authentication: 

```r
session <- ssh_connect("jeroen@dev.opencpu.org")
```

You can use the session in subsequent ssh functions below.

### Run a command

Run a command or script on the host while streaming stdout and stderr directly to the client.

```r
ssh_exec_wait(session, "apt-get update -y && apt-get upgrade -y")
```

If you want to capture the stdout/stderr:

```r
out <- ssh_exec_internal(session, "R -e 'rnorm(100)'")
cat(rawToChar(out$stdout))
```

### SCP Single Files

Simple read a file from the server:

```r
cat(rawToChar(scp_read_file(session, '/etc/passwd')))
```

Or upload a file to the server. Here `data` can be a character or raw (binary) vector:

```r
scp_write_file(session, "hello/helloworld.txt", data = "Hello World!")
```

### Create a Tunnel

Opens a port on your machine and tunnel all traffic to a custom target host via the SSH server.

```r
ssh_tunnel(session, port = 5555,target = "ds043942.mongolab.com:43942")
```

This function blocks while the tunnel is active. Use the tunnel by connecting to `localhost:5555` from a separate process. The tunnel can only be used once and will automatically be closed when the client disconnects.

