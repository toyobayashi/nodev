# nodev

[![Build status](https://api.travis-ci.com/toyobayashi/nodev.svg?branch=master&status=passed)](https://www.travis-ci.com/github/toyobayashi/nodev/)

[中文 README](README_CN.md)

Nodejs version manager written in C++.

Feature:

* Single exectable

* Share global node modules

[Release Download](https://github.com/toyobayashi/nodev/releases)

## Quick Start

### Windows

1. Uninstall Node.js installed by MSI installer and Remove `C:\Users\USERNAME\AppData\Roaming\npm`.

2. `mkdir somewhere-you-like`

3. Add somewhere-you-like to `%PATH%` manually.

4. Place `nodev.exe` in somewhere-you-like.

5. Enjoy yourself.

    ``` bat
    > nodev use 12.16.2
    > node -v
    > npm -v
    ```

### Linux (similar on mac)

1. Uninstall Node.js installed by system package manager.

2. `mkdir -p ~/app/nodev`

3. Add `~/app/nodev` and `~/app/nodev/bin` to `$PATH`.

    ``` bash
    $ echo "export PATH=\$PATH:~/app/nodev:~/app/nodev/bin" >> ~/.bashrc
    $ source ~/.bashrc
    ```

4. Place `nodev` in `~/app/nodev`.

5. Configure node executable directory (relative to `nodev` executable path)

    ``` bash
    $ nodev root bin
    ```

6. Enjoy yourself.

    ``` bash
    $ nodev use 12.16.2
    $ node -v
    $ npm -v
    ```

Config file is `~/nodev.config.json`

Example:

``` json
{
  // relative to nodev executable path, can be absolute
  "root": "bin",

  "node": {
    "mirror": "https://npm.taobao.org/mirrors/node",

    // "x86" is available on Windows only
    "arch": "x64", 

    // relative to nodev executable path, can be absolute
    "cacheDir": "cache/node" 
  },
  "npm": {
    "mirror": "https://npm.taobao.org/mirrors/npm",

    // relative to nodev executable path, can be absolute
    "cacheDir": "cache/npm"
  }
}
```

## Building

Getting source code.

``` bash
$ git clone https://github.com/toyobayashi/nodev.git
$ cd ./nodev
$ git submodule init
$ git submodule update
```

### Windows

Prerequests:

* VC++ environment

* CMake

Run Visual Studio x86 Native Tools Command Prompt.

Set VCVER variable.

14: VS 2015  
15: VS 2017  
16: VS 2019

``` bat
> set VCVER=16
> scripts\buildzlib.bat
> scripts\buildcurl.bat
> build.bat win32 Release static
```

Output: `dist\win\Win32\bin\nodev.exe`

### Linux

Prerequests:

* gcc & g++

* CMake

``` bash
$ chmod +x ./scripts/buildzlib.sh ./build.sh
$ ./scripts/buildzlib.sh
$ ./scripts/build.sh Release
```

Output: `./dist/linux/bin/nodev-linux`

### macOS

Prerequests:

* clang & clang++

* CMake

``` bash
$ chmod +x ./scripts/buildzlib.sh ./scripts/buildcurl.sh ./build.sh
$ ./scripts/buildzlib.sh
$ ./scripts/buildcurl.sh
$ ./scripts/build.sh Release
```

Output: `./dist/darwin/bin/nodev-darwin`
