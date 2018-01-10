/*!
 * \file buffer.h
 *
 * String formatting.
 *
 * \author Geoffrey Davis
 * \addtogroup buffer
 *
 * \par Copyright
 * Copyright (C) 1997-2018 Heroes of Kore
 *
 * \par CirlcleMUD copyright
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 *
 * \par DikuMUD copyright
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991
 *
 * \par License
 * All rights reserved. See license.doc for complete information.
 */
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "circle.h"

/*!
 * Prints to a static buffer.
 * \addtogroup buffer
 * \param _Buf the buffer
 * \param _Buflen the length of the specified buffer
 * \param _Bufpos the current offset into the specified buffer
 */
#define BPrintf(_Buf, _Buflen, _Bufpos, ...) \
  if (_Bufpos < _Buflen - 1) { \
    const ssize_t _R = snprintf( \
	_Buf + _Bufpos, \
	_Buflen - _Bufpos, \
	__VA_ARGS__); \
    if (_R >= 0 && _R < _Buflen - _Bufpos) { \
      _Bufpos += _R; \
    } \
    _Buf[_Bufpos] = '\0'; \
  }

#endif /* _BUFFER_H_ */
