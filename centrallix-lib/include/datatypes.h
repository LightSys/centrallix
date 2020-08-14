#ifndef _DATATYPES_H
#define _DATATYPES_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module:      datatypes.h                                             */
/* Author:      Greg Beeley (GRB)                                       */
/* Creation:    October 26, 1999                                        */
/* Description: Provides datatype abstraction and structures for 	*/
/*		managing datatypes.					*/
/************************************************************************/



/** Date/time property structure **/
typedef union _DT
    {
    struct
        {
        unsigned int    Second:6;
        unsigned int    Minute:6;
        unsigned int    Hour:5;
        unsigned int    Day:5;
        unsigned int    Month:4;
        unsigned int    Year:12;
        }
        Part;
    long long		Value;
    }
    DateTime, *pDateTime;

/** Integer Vector structure **/
typedef struct _IV
    {
    unsigned int        nIntegers;
    int*		Integers;
    }
    IntVec, *pIntVec;

/** String Vector structure **/
typedef struct _SV
    {
    unsigned int        nStrings;
    char**              Strings;
    }
    StringVec, *pStringVec;

/** Money structure **/
typedef struct _MN
    {
    long long       Value;     /* Fixed-point representation in 1/10000 of a dollar */
    }
    MoneyType, *pMoneyType;

/** Binary data (counted string) structure **/
typedef struct _BN
    {
    int			Size;
    unsigned char*	Data;
    }
    Binary, *pBinary;

/** Data Types. **/
#define DATA_T_UNAVAILABLE      0
#define DATA_T_ANY		0
#define DATA_T_INTEGER          1
#define DATA_T_STRING           2
#define DATA_T_DOUBLE           3
#define DATA_T_DATETIME         4
#define DATA_T_INTVEC           5
#define DATA_T_STRINGVEC        6
#define DATA_T_MONEY            7
#define DATA_T_ARRAY		8	/* generic array, added for jsvm */
#define DATA_T_CODE		9	/* code; function/expression/etc */
#define DATA_T_BINARY		10

/** structure used to handle data pointers (GetAttrValue, etc) **/
typedef union _POD
    {
    int         Integer;
    char*       String;
    Binary	Binary;
    pMoneyType  Money;
    pDateTime   DateTime;
    double      Double;
    pStringVec	StringVec;
    pIntVec	IntVec;
    void*	Generic;
    }
    ObjData, *pObjData;

#define POD(x)  ((pObjData)(x))


#endif /* _DATATYPES_H */
