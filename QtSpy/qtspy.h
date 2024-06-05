#pragma once

//#include "qtspy_global.h"
#ifndef QT_SPY_LIB
#pragma comment(lib, "QtSpy.lib")
#endif
#include "TimeKeeper.h"
class /*QTSPY_EXPORT*/ QtSpy
{
public:
    QtSpy(void* parent = nullptr);
    ~QtSpy();
private:
    void StartSpy(void* parent = nullptr);
    void StopSpy();
};
