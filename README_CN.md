# nodev

[![Build status](https://api.travis-ci.com/toyobayashi/nodev.svg?branch=master&status=passed)](https://www.travis-ci.com/github/toyobayashi/nodev/)

用 C++ 写的 Node.js 版本管理器。

特性：

* 单个可执行文件

* 切换 Node.js 版本时共享全局安装的包

[下载](https://github.com/toyobayashi/nodev/releases)

## 快速开始

### Windows

1. 卸载掉用官方安装包安装好的 Node.js 并删掉 `C:\Users\USERNAME\AppData\Roaming\npm`。

2. `mkdir somewhere-you-like`

3. 手动把 somewhere-you-like 加到环境变量 `%PATH%` 里面。

4. 把 `nodev.exe` 放进 somewhere-you-like。

5. 开始玩耍。

    ``` bat
    > nodev use 12.16.2
    > node -v
    > npm -v
    ```

### Linux （mac 上类似）

1. 卸载掉系统包管理器安装的 Node.js。

2. `mkdir -p ~/app/nodev`

3. 把 `~/app/nodev` 和 `~/app/nodev/bin` 加到环境变量 `$PATH` 里面。

    ``` bash
    $ echo "export PATH=\$PATH:~/app/nodev:~/app/nodev/bin" >> ~/.bashrc
    $ source ~/.bashrc
    ```

4. 把 `nodev` 放进 `~/app/nodev`.

5. 配置 node 可执行文件目录（相对于 `nodev` 可执行文件路径）

    ``` bash
    $ nodev root bin
    ```

6. 开始玩耍。

    ``` bash
    $ nodev use 12.16.2
    $ node -v
    $ npm -v
    ```

配置文件是 `~/nodev.config.json`

例子：

``` json
{
  // 相对于 nodev 可执行文件路径，也可以是绝对路径
  "root": "bin",

  "node": {
    "mirror": "https://npm.taobao.org/mirrors/node",

    // "x86" 只有在 Windows 上可用
    "arch": "x64", 

    // 相对于 nodev 可执行文件路径，也可以是绝对路径
    "cacheDir": "cache/node" 
  },
  "npm": {
    "mirror": "https://npm.taobao.org/mirrors/npm",

    // 相对于 nodev 可执行文件路径，也可以是绝对路径
    "cacheDir": "cache/npm"
  }
}
```

## 构建

先拉代码。

``` bash
$ git clone https://github.com/toyobayashi/nodev.git
$ cd ./nodev
$ git submodule init
$ git submodule update
```

### Windows

环境要求：

* VC++ 环境

* CMake

启动 Visual Studio x86 Native Tools Command Prompt。

可以设置 VCVER 变量指定 VS 版本。

14: VS 2015  
15: VS 2017  
16: VS 2019

``` bat
> set VCVER=16
> scripts\buildzlib.bat
> scripts\buildcurl.bat
> build.bat win32 Release static
```

输出：`dist\win\Win32\bin\nodev.exe`

### Linux

环境要求：

* gcc & g++

* CMake

``` bash
$ chmod +x ./scripts/buildzlib.sh ./build.sh
$ ./scripts/buildzlib.sh
$ ./scripts/build.sh Release
```

输出：`./dist/linux/bin/nodev-linux`

### macOS

环境要求：

* clang & clang++

* CMake

``` bash
$ chmod +x ./scripts/buildzlib.sh ./scripts/buildcurl.sh ./build.sh
$ ./scripts/buildzlib.sh
$ ./scripts/buildcurl.sh
$ ./scripts/build.sh Release
```

输出：`./dist/darwin/bin/nodev-darwin`
