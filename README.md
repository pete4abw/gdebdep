# gdebdep
Get Debian Dependencies -- for non-Debian Systems

## Synopsis
**gdebdep** *debian_package.deb*

## Description

`gdebdep` is a small program that will
* Download the most current Packages.xz from Debian main
* Extract **control** file from target Debian package
* Identify all dependencies from **Depends:** line of control including those with an **OR** dependency.
  e.g. `Depends: package1 | package 2`
* Obtain download locations from FTP mirror
* write and `lftp` command file in a subdirectory off $PWD to download all depdent .deb files

Execute `lftp -f lftp` to download all dependencies in a given directory

## Configuration File
`gdebdep` requires a configuration file, naned **gdebdep.conf** that contains the following elements:
```
# conf file for gdebdep

ARCH=*current architecture to fetch*
MIRROR=*desired Debian FTP mirror*
DEBIANDIR=*Directory under pool to fetch from*
PACKAGELISTDIR=*directory to fetch Packages.xz from, ending with* **/binary-** ($ARCH will be added after - )
PACKAGEFILENAME=*Packages.xz*
```

## Output
Under execution directory, a subdirectory with the basename of the Debian Package will be created. The `lftp` command file will reside there.
e.g. The target Debian deb package is `iproute2_4.20.0-2_amd64.deb`. The directory create will be **`iproute2`**.
## lftp file resembles
```
open ftp.us.debian.org
cd debian
get pool/main/i/iproute2/iproute2_4.20.0-2_amd64.deb
get pool/main/c/c-ares/libc-ares2_1.14.0-1_amd64.deb
get pool/main/g/glibc/libc6_2.28-10_amd64.deb
get pool/main/s/strongswan/libcharon-extra-plugins_5.7.2-1_amd64.deb
get pool/main/c/curl/libcurl4_7.64.0-4+deb10u1_amd64.deb
get pool/main/l/lzo2/liblzo2-2_2.10-0.1_amd64.deb
get pool/main/q/qtbase-opensource-src/libqt5concurrent5_5.11.3+dfsg1-1+deb10u4_amd64.deb
get pool/main/q/qtbase-opensource-src/libqt5core5a_5.11.3+dfsg1-1+deb10u4_amd64.deb
get pool/main/q/qtbase-opensource-src/libqt5gui5_5.11.3+dfsg1-1+deb10u4_amd64.deb
get pool/main/q/qtbase-opensource-src/libqt5network5_5.11.3+dfsg1-1+deb10u4_amd64.deb
get pool/main/q/qtscript-opensource-src/libqt5script5_5.11.3+dfsg-3_amd64.deb
get pool/main/q/qtwebengine-opensource-src/libqt5webengine5_5.11.3+dfsg-2+deb10u1_amd64.deb
get pool/main/q/qtwebengine-opensource-src/libqt5webenginewidgets5_5.11.3+dfsg-2+deb10u1_amd64.deb
get pool/main/q/qtwebkit-opensource-src/libqt5webkit5_5.212.0~alpha2-21_amd64.deb
get pool/main/q/qtbase-opensource-src/libqt5widgets5_5.11.3+dfsg1-1+deb10u4_amd64.deb
get pool/main/o/openssl/libssl1.1_1.1.1d-0+deb10u3_amd64.deb
get pool/main/g/gcc-8/libstdc++6_8.3.0-6_amd64.deb
get pool/main/s/strongswan/libstrongswan-extra-plugins_5.7.2-1_amd64.deb
get pool/main/n/net-tools/net-tools_1.60+git20180626.aebd88e-1_amd64.deb
get pool/main/o/openvpn/openvpn_2.4.7-1_amd64.deb
get pool/main/r/resolvconf/resolvconf_1.79_all.deb
get pool/main/s/strongswan/strongswan_5.7.2-1_all.deb
get pool/main/z/zlib/zlib1g_1.2.11.dfsg-1_amd64.deb
```

# Why?
Some commercial packages only come compiled for Debian systems. In order to run on non-Debian systems, proper libraries with proper compile-time options must be downloaded. Using `LD_LIBRARY_PATH` Debian libraries can be isolated and placed together. The target program can run. By unpacking `.deb` files together, a customized, subset Debian library can be set.

While may common libraries, e.g. `libc6` present no problem and can be excluded, others like libcurl4 present issues. Debian compiles libcurl4 with
`configure --enable-versioned-symbols` which is unique. Using a distro provided libcurl could present an error such as:
`vpn-unlimited-daemon: /usr/lib64/libcurl.so.4: no version information available (required by /usr/lib64/libvpnu_private_sdk.so.1)`

By downloading Debian's libcurl4 and its dependencies, this problem is averted.

## Not all packages required
Some, but maybe not all package dependencies are required (e.g. `gcc`, `libc6`, `debconf`, etc.. Using `ldd library-name`, one may identify other libraries and packages needed. A particular Debian library can be associated with a package by searching: `https://www.debian.org/distrib/packages#search_packages`.
