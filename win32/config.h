//--------------------------------------------------------------------
// $Id: config.h 4608 2005-03-21 21:36:38Z cjm $
//--------------------------------------------------------------------
//
//   Visual Binary Diff
//   Copyright 1997-2005 by Christopher J. Madsen
//
//   Include file for standard system include files,
//   or project specific include files that are used frequently,
//   but are changed infrequently
//
//--------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>             // _ASSERT

#define ASSERT _ASSERTE

#define WIN32_CONSOLE 1

#define PACKAGE_VERSION "3.0"   // FIXME

/////////////////////////////////////////////////////////////////////////////
// Local Variables:
// mode: c++
// End: