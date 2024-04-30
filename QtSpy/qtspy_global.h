#pragma once

#include <QtCore/qglobal.h>
#define BUILD_STATIC
#ifndef BUILD_STATIC
# if defined(QTSPY_LIB)
#  define QTSPY_EXPORT Q_DECL_EXPORT
# else
#  define QTSPY_EXPORT Q_DECL_IMPORT
# endif
#else
# define QTSPY_EXPORT
#endif
