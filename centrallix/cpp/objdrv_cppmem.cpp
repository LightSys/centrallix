#include <list>
#include <map>
#include <iostream>
#include <algorithm>
#include "st_node.h"
#include "objdrv.hpp"


class cppmem: public objdrv{
    std::list<char> Buffer;
    pSnNode nodethingy;
public:
    cppmem(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    int Close(pObjTrxTree* oxt);
    int Delete(pObject obj, pObjTrxTree* oxt);
    int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
    bool UpdateAttr(std::string attrname, pObjTrxTree* oxt);
    int Info(pObjectInfo info);
    query_t *OpenQuery (pObjQuery query, pObjTrxTree *oxt);
    std::list<std::string> GetMethods();
    int RunMethod(std::string methodname, pObjData param, pObjTrxTree oxt);
    pObjPresentationHints PresentationHints(std::string attrname, pObjTrxTree* oxt);
    virtual ~cppmem();
};//end cppmem

std::map<std::string, cppmem*> files;

class query_mem: public query_t{
    std::map<std::string, cppmem*>::const_iterator file;
public:
    query_mem(objdrv *start):query_t(start){
       file=files.begin();
    }

    objdrv *Fetch(pObject obj, int mode, pObjTrxTree *oxt){
        if(file==files.end())
            return NULL;
        return (file++)->second;
    }
    
};

int cppmem::Close(pObjTrxTree* oxt){
    if(this->nodethingy)this->nodethingy->OpenCnt--;
    return 0;
}

int cppmem::Delete(pObject obj, pObjTrxTree* oxt){
    std::cerr<<"Dropping mem object "<< GetAtrribute("name")<<std::endl;
    files.erase(std::string(GetAtrribute("name")->Value->String));
    return 0;
}

int cppmem::Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt){
    for(int i=0;i<cnt;i++)
        Buffer.push_back(*(buffer+i));
    SetAtrribute("last_modification",new Attribute((pDateTime)NULL),oxt);
    SetAtrribute("size",new Attribute(Buffer.size()),oxt);
    return cnt;
}

int cppmem::Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt){
    int cnt=((unsigned int)maxcnt<Buffer.size())?maxcnt:Buffer.size();
    for(int i=0;i<cnt;i++){
        *(buffer+i)=Buffer.front();
        Buffer.pop_front();
    }
    if(cnt==0)return -1;
    SetAtrribute("last_modification",new Attribute((pDateTime)NULL),oxt);
    SetAtrribute("size",new Attribute(Buffer.size()),oxt);
    return cnt;
}

bool cppmem::UpdateAttr(std::string attrname, pObjTrxTree* oxt){
    objdrv::UpdateAttr(attrname,oxt);
    return false;
}

int cppmem::Info(pObjectInfo info){
    info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CAN_ADD_ATTR |
		OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CAN_HAVE_CONTENT);
    if(Buffer.size())info->Flags |= OBJ_INFO_F_HAS_CONTENT;
    return 0;
}

std::list<std::string> cppmem::GetMethods(){
    std::list<std::string> tmp;
    tmp.push_back("dump");
    tmp.push_back("genkey");
    tmp.push_back("shuffle");
    return tmp;
}

int cppmem::RunMethod(std::string methodname, pObjData param, pObjTrxTree oxt){
    if(!methodname.compare("dump")){
        std::cerr<<"Dumping "<<GetAtrribute("name")<<std::endl;
        Buffer.erase(Buffer.begin(),Buffer.end());
        return 0;
    }else if(!methodname.compare("genkey")){
        srand(time(NULL));
        std::cerr<<"Filling "<<GetAtrribute("name")<<
                "with 1024 random char"<<std::endl;
        for(int i=0; i<1024; i++){
            char tmp=(char)((rand()%84)+32);
            this->Write(&tmp,1,0,0,0);
        }
    }else if(!methodname.compare("shuffle")){
        std::cerr<<"Shuffling "<<GetAtrribute("name")
                <<" with "<< param->String << std::endl;
        srand(time(NULL));
        //std::random_shuffle(Buffer.begin(),Buffer.end());
    }
    return -1;
}

pObjPresentationHints cppmem::PresentationHints(std::string attrname, pObjTrxTree* oxt){
    pObjPresentationHints hints=NewHints();
    if(!attrname.compare("size")){
        hints->Style = OBJ_PH_STYLE_READONLY;
        return hints;
    }
    if(!attrname.compare("source_class")){
        hints->Style = OBJ_PH_STYLE_LOWERCASE | OBJ_PH_STYLE_NOTNULL;
        hints->Length = 16;
        hints->FriendlyName = strdup(std::string("Source Class of object").c_str());
        hints->BadChars = strdup(std::string("!@#$%^&*()-_=+[]{}\\|;'\",.<>/?").c_str());
        return hints;
    }
    return objdrv::PresentationHints(attrname,oxt);
}

query_t *cppmem::OpenQuery (pObjQuery query, pObjTrxTree *oxt){
    return new query_mem(this);
}

cppmem::cppmem(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
        :objdrv(obj,mask,systype,usrtype,oxt){
    this->Obj=obj;
    this->Pathname=std::string(obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr));
    //this->nodethingy = snReadNode(obj->Prev);
    this->nodethingy = 0;
    if(this->nodethingy)this->nodethingy->OpenCnt++;
    SetAtrribute("name",new Attribute(obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr)),oxt);
    SetAtrribute("outer_type",new Attribute("text/mem"),oxt);
    SetAtrribute("inner_type",new Attribute("application/octet-stream"),oxt);
    SetAtrribute("annotation",new Attribute("cpp object"),oxt);
    SetAtrribute("source_class",new Attribute("cpp"),oxt);
    std::cerr<<"New mem object "<< GetAtrribute("name") <<" as "<<usrtype<<std::endl;
}

cppmem::~cppmem(){
    std::cerr<<"Mem object "<< GetAtrribute("name") <<" signing off."<<std::endl;
    if(this->nodethingy)this->nodethingy->OpenCnt--;
    Buffer.erase(Buffer.begin(),Buffer.end());
}

objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    cppmem *tmp;
    std::string id=std::string(obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr));
    if(id.find(".mem",0)==id.npos){
        std::cerr<<"Rejected opening "<< id <<" as "<<usrtype<<std::endl;
        return NULL;
    }
    tmp=files[id];
    if(!tmp){
        tmp=files[id]=new cppmem(obj,mask,systype,usrtype,oxt);
        tmp->Write("Hello world",12,0,0,0);
    }else std::cerr<<"Retrieved mem object "<< id <<" as "<<usrtype<<std::endl;
    obj->SubCnt=obj->Prev->SubCnt;
    return tmp;
}

char *GetName(){
    return (char *)"Virtual memory file";
}

std::list<std::string> GetTypes(){
    std::list<std::string> tmp;
    tmp.push_back("text/mem");
    return tmp;
}

MODULE_PREFIX("mem");
MODULE_DESC("Virtual object in memory");
MODULE_VERSION(0,0,1);