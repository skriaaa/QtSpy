编译：

1、在utils.cmake里配置Qt版本和路径
2、build目录，执行   windows：build.bat/built_64.bat； linux：linux_build.sh



引入(静态库)：

1、在Qt工程.pro中添加
```c++
#  QT_SPY_INCLUE是CMakeLists.txt所在文件夹
QT += testlib
INCLUDEPATH += $(QT_SPY_INCLUDE) 
LIBS += -L$(QT_SPY_INCLUDE) -lQtSpy
```
2、引用头文件
```c++
#include "QtSpyInterface.h"
```
3、调起窗口
```c++
QtSpy::initSpy();
```
