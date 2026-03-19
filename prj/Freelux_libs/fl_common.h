/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_common.h
 *Created on		: Jul 9, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#ifndef FREELUX_LIBS_FL_COMMON_H_
#define FREELUX_LIBS_FL_COMMON_H_

#include "plog.h"
#include "queue_fifo.h"
#include "fl_sys_datetime.h"

inline int MSB_BIT_SET(u8 *_array, int _size) {
    for (int i = _size - 1; i >= 0; --i) {
        if (_array[i]) {
            for (int b = 7; b >= 0; --b) {
                if (_array[i] & (1 << b)) {
                    return i * 8 + b;
                }
            }
        }
    }
    return -1;
}

inline int LSB_BIT_SET(u8 *_array, int _size) {
    for (int i = 0; i < _size; ++i) {
        if (_array[i]) {
            for (int b = 0; b < 8; ++b) {
                if (_array[i] & (1 << b)) {
                    return i * 8 + b;
                }
            }
        }
    }
    return -1; //
}
inline void SET_BIT_InARRAY(uint8_t *arr, int _size, int _bitPos) {
    int index  = _bitPos / 8;
    int offset = _bitPos % 8;

    if (index < 0 || index >= _size) return;
    arr[index] |=  (1U << offset);

//        arr[index] &= ~(1U << offset);  //clear
}
inline void CLEAR_BIT_InARRAY(uint8_t *arr, int _size, int _bitPos) {
    int index  = _bitPos / 8;
    int offset = _bitPos % 8;
    if (index < 0 || index >= _size) return;
    arr[index] &= ~(1U << offset);  //clear
}

inline int GET_BIT_InARRAY(uint8_t *arr, int _size, int _bitPos) {
    int index  = _bitPos / 8;
    int offset = _bitPos % 8;

    if (index < 0 || index >= _size) return 0;
    return (arr[index] >> offset) & 1U;
}


#endif /* FREELUX_LIBS_FL_COMMON_H_ */
