// ========================================================================== //
// This file is part of DO++, a basic set of libraries in C++ for computer 
// vision.
//
// Copyright (C) 2013 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public 
// License v. 2.0. If a copy of the MPL was not distributed with this file, 
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

#ifndef DO_DEFINES_HPP
#define DO_DEFINES_HPP

#define DO_VERSION "1.0.0"

#ifdef WIN32
# ifdef DO_EXPORTS
#   define DO_EXPORT __declspec(dllexport) /* We are building the libraries */
# elif defined(DO_STATIC)
#   define DO_EXPORT
# else
#   define DO_EXPORT __declspec(dllimport) /* We are using the libraries */
# endif
#else
# define DO_EXPORT
#endif

#ifdef DO_DEPRECATED
# undef DO_DEPRECATED
#endif

#ifdef __GNUC__
# define DO_DEPRECATED __attribute__ ((deprecated))
#elif defined(_MSC_VER)
# define DO_DEPRECATED __declspec(deprecated)
#else
# pragma message("WARNING: You need to implement DO_DEPRECATED for this compiler")
# define DO_DEPRECATED
#endif

#endif /* DO_DEFINES_HPP */
