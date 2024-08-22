#pragma once

//#include "qtspy_global.h"
class /*QTSPY_EXPORT*/ QtSpy
{
public:
    QtSpy(void* parent = nullptr);
    ~QtSpy();
private:
    void StartSpy(void* parent = nullptr);
    void StopSpy();
};
