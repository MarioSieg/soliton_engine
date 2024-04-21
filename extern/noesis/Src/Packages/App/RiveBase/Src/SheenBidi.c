////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "BuildSettings.h"


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4706 4189)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

typedef int make_iso_compilers_happy;

#ifdef WITH_RIVE_TEXT
#define SB_CONFIG_UNITY
#include "SheenBidi/Source/SheenBidi.c"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
