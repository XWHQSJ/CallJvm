# CallJvm

Call JVM from C/C++ in ThreadPool Using JNI

### 文件结构 ——

```
.
├── callJvmThreadpool
│   ├── a.out
│   ├── cmake-build-debug
│   │   ├── callJvm.cbp
│   │   ├── callJvmThreadp.cbp
│   │   ├── callJvmThreadpool.cbp
│   │   ├── CMakeCache.txt
│   │   ├── CMakeFiles
│   │   │   ├── 3.14.5
│   │   │   │   ├── CMakeCCompiler.cmake
│   │   │   │   ├── CMakeCXXCompiler.cmake
│   │   │   │   ├── CMakeDetermineCompilerABI_C.bin
│   │   │   │   ├── CMakeDetermineCompilerABI_CXX.bin
│   │   │   │   ├── CMakeSystem.cmake
│   │   │   │   ├── CompilerIdC
│   │   │   │   │   ├── a.out
│   │   │   │   │   ├── CMakeCCompilerId.c
│   │   │   │   │   └── tmp
│   │   │   │   └── CompilerIdCXX
│   │   │   │       ├── a.out
│   │   │   │       ├── CMakeCXXCompilerId.cpp
│   │   │   │       └── tmp
│   │   │   ├── clion-environment.txt
│   │   │   ├── clion-log.txt
│   │   │   ├── cmake.check_cache
│   │   │   ├── CMakeDirectoryInformation.cmake
│   │   │   ├── CMakeOutput.log
│   │   │   ├── CMakeTmp
│   │   │   ├── feature_tests.bin
│   │   │   ├── feature_tests.c
│   │   │   ├── feature_tests.cxx
│   │   │   ├── main.dir
│   │   │   │   ├── build.make
│   │   │   │   ├── cmake_clean.cmake
│   │   │   │   ├── CXX.includecache
│   │   │   │   ├── DependInfo.cmake
│   │   │   │   ├── depend.internal
│   │   │   │   ├── depend.make
│   │   │   │   ├── flags.make
│   │   │   │   ├── link.txt
│   │   │   │   ├── progress.make
│   │   │   │   ├── socketMultithread.cpp.o
│   │   │   │   ├── socketThreadpool.cpp.o
│   │   │   │   └── tpool.cpp.o
│   │   │   ├── Makefile2
│   │   │   ├── Makefile.cmake
│   │   │   ├── progress.marks
│   │   │   └── TargetDirectories.txt
│   │   ├── cmake_install.cmake
│   │   ├── hs_err_pid22583.log
│   │   ├── hs_err_pid22734.log
│   │   ├── hs_err_pid22793.log
│   │   ├── hs_err_pid22856.log
│   │   ├── hs_err_pid22911.log
│   │   ├── main
│   │   └── Makefile
│   ├── CMakeLists.txt
│   ├── jni.h
│   ├── jni_md.h
│   ├── main.cpp
│   ├── pureMultithread.cpp
│   ├── qin_test1.jar
│   ├── qin_test.jar
│   ├── server
│   ├── server.cpp
│   ├── client
│   ├── client.cpp
│   ├── socketMultithread.cpp
│   ├── socketThreadpool.cpp
│   ├── test.cpp
│   ├── tpool.cpp
│   └── tpool.h
└── README.md
```
### 文件解释 ——

···
├── CMakeLists.txt : cmake编译文件
├── jni.h : java JNI接口函数
├── jni_md.h : jni.h调用的必要函数
├── main.cpp : 测试主程序
├── pureMultithread.cpp : 干净的多线程程序
├── qin_test1.jar : 测试的jar包1
├── qin_test.jar : 测试的jar包0
├── server.cpp : socket服务器程序
├── client.cpp : socket客户端程序
├── socketMultithread.cpp : socket服务器+jni调用的多线程程序
├── socketThreadpool.cpp : socket服务器+jni调用的线程池程序
├── test.cpp : 测试程序
├── tpool.cpp : 线程池实现程序
└── tpool.h : 线程池实现头文件
···

### 程序编译

```
cd callJvmThreadpool
cmake ..
./a.out
```

### 程序目的

- 最初目的：使用C++代码调用java编写的代码
- 中间目的：使用多线程方式调用java代码
- 最终目的：使用线程池方式减少多线程带来的资源消耗调用java代码

### 相关内容

- https://www3.ntu.edu.sg/home/ehchua/programming/java/JavaNativeInterface.html
- https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html
