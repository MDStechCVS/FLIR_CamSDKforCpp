// *****************************************************************************
//
//     Copyright (c) 2007, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************
//
// File Name....: PvTypes.h
//
// *****************************************************************************

#ifndef __PVTYPES_H__
#define __PVTYPES_H__

#ifndef PV_NO_STDINT
    #ifdef WIN32
        #if ( _MSC_VER < 1600 || defined _INTSAFE_H_INCLUDED_ )
            #ifndef PT_STDINT
                #define PV_STDINT

                typedef signed char int8_t;
                typedef short int16_t;
                typedef int int32_t;
                typedef __int64 int64_t;

                typedef unsigned char uint8_t;
                typedef unsigned short uint16_t;
                typedef unsigned int uint32_t;
                typedef unsigned __int64 uint64_t;

                #if ( defined _WIN64 || defined __LP64__ || defined _LP64 )
                    typedef unsigned __int64 uintptr_t;
                    typedef __int64 intptr_t;
                #else
                    typedef unsigned int uintptr_t;
                    typedef int intptr_t;
                #endif

            #endif
        #else
            #include <stdint.h>
        #endif
    #else
        #include <stdint.h>
    #endif
#endif 

#ifdef WIN32
    #define PV_DEPRECATED_ALTERNATIVE(A) __declspec(deprecated("This method is deprecated. Consider using "##A))
    #define PV_DEPRECATED __declspec(deprecated)
    #define PV_DEPRECATED_MESSAGE(A) __declspec(deprecated(A))
#else
    #define PV_DEPRECATED_MESSAGE(A)
    #define PV_DEPRECATED_ALTERNATIVE(A)
    #define PV_DEPRECATED
#endif

#endif // __PVTYPES_H__

