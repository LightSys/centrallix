
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
#include <stdlib.h>
#include "objdrv.hpp"

//constructors!

Attribute::Attribute(int type,pObjData value){
    Attribute *tmp;
    switch(type){
        case DATA_T_INTEGER:
            tmp=new Attribute(value->Integer);
            break;
        case DATA_T_DOUBLE:
            tmp=new Attribute(value->Double);
            break;
        case DATA_T_STRING:
            tmp=new Attribute(std::string(value->String));
            break;
        case DATA_T_BINARY:
            tmp=new Attribute(value->Binary);
            break;
        case DATA_T_DATETIME:
            tmp=new Attribute(value->DateTime);
            break;
        case DATA_T_MONEY:
            tmp=new Attribute(value->Money);
            break;
        case DATA_T_INTVEC:
            tmp=new Attribute(value->IntVec);
            break;
        case DATA_T_STRINGVEC:
            tmp=new Attribute(value->StringVec);
            break;
        default:
            Type=type;
            Value = new ObjData;
            //blind copy
            memcpy(Value,value,sizeof(ObjData));
            return;//exit NOW!
    }//end switch
    Type = tmp->Type;
    Value = new ObjData;
    memcpy(Value,tmp->Value,sizeof(ObjData));
}//end constructor

Attribute::Attribute(std::string value){
    std::cerr<<"cpp string is "<<value<<std::endl;
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
            Value->DateTime = new DateTime;
            objCurrentDate(Value->DateTime);
            return;
        }else{
            memcpy(Value->DateTime,value,sizeof(DateTime));
        }//end if now
}//end new Att Date

Attribute::Attribute(pMoneyType value){
        Type=DATA_T_MONEY;
        Value= new ObjData;
        bzero(Value,sizeof(ObjData));
        Value->Money=value;
}

Attribute::Attribute(Binary_t value){
        Type=DATA_T_BINARY;
        Value= new ObjData;
        bzero(Value,sizeof(ObjData));
        Value->Binary.Size=value.Size;
        Value->Binary.Data=value.Data;
}

Attribute::Attribute(pStringVec value){
        Type=DATA_T_STRINGVEC;
        Value= new ObjData;
        memcpy(Value->StringVec,value,sizeof(StringVec));
}

Attribute::Attribute(pIntVec value){
        Type=DATA_T_INTVEC;
        Value= new ObjData;
        memcpy(Value->IntVec,value,sizeof(IntVec_t));
}

//clean up after ourself
Attribute::~Attribute(){
    switch(Type){
        case DATA_T_STRING:
            free(Value->String);
            break;
        case DATA_T_DATETIME:
            delete Value->DateTime;
            break;
    }//end clean type
    delete Value;
}

//stream operator
std::ostream &operator <<(std::ostream &out,Attribute *att){
    if(!att){
        out << "NULL";
        return out;
    }
    ///@todo DATA_T_ARRAY, DATA_T_CODE
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
        case DATA_T_STRINGVEC:
            if(att->Value->StringVec->nStrings<=0)break;
            out <<att->Value->StringVec->Strings[0];
            for(unsigned int i=1; i < att->Value->StringVec->nStrings; i++)
                out <<", "<<att->Value->StringVec->Strings[i];
            break;
        case DATA_T_INTVEC:
            if(att->Value->IntVec->nIntegers<=0)break;
            out <<att->Value->IntVec->Integers[0];
            for(unsigned int i=1; i < att->Value->IntVec->nIntegers; i++)
                out <<", "<<att->Value->IntVec->Integers[i];
            break;
        case DATA_T_DATETIME:
            //and it's like "WOMP"
            out<<att->Value->DateTime->Part.Month+1<<"/"
                    <<att->Value->DateTime->Part.Day+1<<"/"
                    <<att->Value->DateTime->Part.Year+1900<<" "
                    <<att->Value->DateTime->Part.Hour<<":"
                    <<att->Value->DateTime->Part.Minute<<":"
                    <<att->Value->DateTime->Part.Second;
            //and now it's in a burrito
            break;
        default:
            out<<att->Value;
            break;
    }//end switch
    return out;
}//end operator
