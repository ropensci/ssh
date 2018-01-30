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
