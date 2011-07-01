
/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2011 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	attribute.cpp                      			*/
/* Author:	Micah Shennum   					*/
/* Creation:	Jun 29 2011     					*/
/* Description: contains the actual function code for the class of the same name */
/************************************************************************/

#include <iostream>
#include <string.h>
#include "objdrv.hpp"

//constructors!

Attribute::Attribute(int type,pObjData value){
        Type=type;
        Value = new ObjData;
        memcpy(Value,value,sizeof(ObjData));
}

Attribute::Attribute(std::string value){
        Type=DATA_T_STRING;
        Value= new ObjData;
        bzero(Value,sizeof(ObjData));
        Value->String=(char *)strdup(value.c_str());
}

Attribute::Attribute(int value){
        Type=DATA_T_INTEGER;
        Value= new ObjData;
        bzero(Value,sizeof(ObjData));
        Value->Integer=value;
}

Attribute::Attribute(pDateTime value){
        Type=DATA_T_DATETIME;
        Value= new ObjData;
        bzero(Value,sizeof(ObjData));
        if(!value){
            //default to now
            pDateTime dt = new DateTime;
            objCurrentDate(dt);
            value=dt;
        }
        Value->DateTime=value;
}

//clean up after ourself
Attribute::~Attribute(){
    delete Value;
}

//stream operator
std::ostream &operator <<(std::ostream &out,Attribute *att){
    if(!att){
        out << "NULL";
        return out;
    }
    switch(att->Type){
        case DATA_T_STRING:
            out<<att->Value->String;
            break;
        case DATA_T_INTEGER:
            out<<att->Value->Integer;
            break;
        case DATA_T_DOUBLE:
            out<<att->Value->Double;
            break;
        //the following cases brazenly stollen from test_obj.c
        case DATA_T_MONEY:
            out<<objDataToStringTmp(DATA_T_MONEY, att->Value, 0);
            break;
        case DATA_T_BINARY:
            out << att->Value->Binary.Size << " bytes: "<< std::hex;
            for(int i=0;i<att->Value->Binary.Size;i++)
                out << att->Value->Binary.Data[i];
            //return to decimal
            out << std::dec;
            break;
        case DATA_T_DATETIME:
            //and it's like "WOMP"
            out<<att->Value->DateTime->Part.Month+1<<"/"
                    <<att->Value->DateTime->Part.Day+1<<"/"
                    <<att->Value->DateTime->Part.Year+1900<<" "
                    <<att->Value->DateTime->Part.Hour<<":"
                    <<att->Value->DateTime->Part.Minute<<":"
                    <<att->Value->DateTime->Part.Second;
            break;
        default:
            out<<att->Value;
            break;
    }//end switch
    return out;
}//end operator
