# SSH

[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](http://www.repostatus.org/badges/latest/active.svg)](http://www.repostatus.org/#active)
[![Build Status](https://travis-ci.org/ropensci/ssh.svg?branch=master)](https://travis-ci.org/ropensci/ssh)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/ropensci/ssh?branch=master&svg=true)](https://ci.appveyor.com/project/jeroen/ssh)
[![Coverage Status](https://codecov.io/github/ropensci/ssh/coverage.svg?branch=master)](https://codecov.io/github/ropensci/ssh?branch=master)
[![CRAN_Status_Badge](http://www.r-pkg.org/badges/version/ssh)](http://cran.r-project.org/package=ssh)
[![CRAN RStudio mirror downloads](http://cranlogs.r-pkg.org/badges/ssh)](http://cran.r-project.org/web/packages/ssh/index.html)

> Secure Shell (SSH) Client for R

## Installation

This package is available on CRAN and can be installed via:

```r
install.packages('ssh')
```

Alternatively it can be installed from source using `devtools`:

```r
remotes::install_github('ropensci/ssh')
```

Installation from source on MacOS or Linux requires [`libssh`](https://www.libssh.org/) (the original `libssh`, __not__ the unrelated `libssh2` library). On __Debian__ or __Ubuntu__ use [libssh-dev](https://packages.debian.org/testing/libssh-dev):

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

```{r}
session <- ssh_connect("jeroen@dev.opencpu.org")
```

You can use the session in subsequent ssh functions below.

### Run a command

Run a command or script on the host while streaming stdout and stderr directly to the client.

```{r}
ssh_exec_wait(session, command = c(
  'curl -fOL https://cloud.r-project.org/src/contrib/Archive/jsonlite/jsonlite_1.5.tar.gz',
  'R CMD check jsonlite_1.5.tar.gz',
  'rm -f jsonlite_1.5.tar.gz'
))
```

If you want to capture the stdout/stderr:

```r
out <- ssh_exec_internal(session, "R -e 'rnorm(100)'")
cat(rawToChar(out$stdout))
```
#### Using 'sudo'

Note that the exec functions are non interactive so they cannot prompt for a sudo password. A trick is to use `-S` which reads the password from stdin:

```
out <- ssh_exec_wait(session, 'echo "mypassword!" | sudo -s -S apt-get update -y')
```

Be very careful with hardcoding passwords!

### Uploading and Downloading via SCP

Upload and download files via SCP. Directories are automatically traversed as in `scp -r`.

```r
# Upload a file to the server
file_path <- R.home("COPYING")
scp_upload(session, file_path)
```

```r
# Download the file back and verify it is the same
scp_download(session, "COPYING", to = tempdir())
tools::md5sum(file_path)
tools::md5sum(file.path(tempdir(), "COPYING"))
```

### Create a Tunnel

Opens a port on your machine and tunnel all traffic to a custom target host via the SSH server.

```r
ssh_tunnel(session, port = 5555,target = "ds043942.mongolab.com:43942")
```

This function blocks while the tunnel is active. Use the tunnel by connecting to `localhost:5555` from a separate process. The tunnel can only be used once and will automatically be closed when the client disconnects.

### Disconnect


When you are done with the session you should disconnect:

```r
ssh_disconnect(session)
```

If you forgot to disconnect, the garbage collector will do so for you (with a warning).

