#include <list>
#include <map>
#include <iostream>
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
};//end cppmem

std::map<pPathname, cppmem*> files;

int cppmem::Close(pObjTrxTree* oxt){
    this->nodethingy->OpenCnt--;
    return 0;
}

int cppmem::Delete(pObject obj, pObjTrxTree* oxt){
    Buffer.empty();
    return 0;
}

int cppmem::Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt){
    for(int i=0;i<cnt;i++)
        Buffer.push_back(*(buffer+i));
    return cnt;
}

int cppmem::Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt){
    int cnt=((unsigned int)maxcnt<Buffer.size())?maxcnt:Buffer.size();
    for(int i=0;i<cnt;i++){
        *(buffer+i)=Buffer.front();
        Buffer.pop_front();
    }
    if(cnt==0)return -1;
    return cnt;
}

bool cppmem::UpdateAttr(std::string attrname, pObjTrxTree* oxt){
    objdrv::UpdateAttr(attrname,oxt);
    return false;
}

int cppmem::Info(pObjectInfo info){
    info->Flags |= ( OBJ_INFO_F_NO_SUBOBJ | OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_CAN_ADD_ATTR |
		OBJ_INFO_F_CANT_SEEK | OBJ_INFO_F_CAN_HAVE_CONTENT);
    if(Buffer.size())info->Flags |= OBJ_INFO_F_HAS_CONTENT;
    return 0;
}

cppmem::cppmem(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
        :objdrv(obj,mask,systype,usrtype,oxt){
    obj->SubCnt=1;
    this->Obj=obj;
    this->Pathname=std::string(obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr));
    this->nodethingy = snReadNode(obj);
    this->nodethingy->OpenCnt++;
    Attributes["name"]=new Attribute(DATA_T_STRING,obj_internal_PathPart(obj->Pathname, 0, obj->SubPtr));
    Attributes["outer_type"]=new Attribute(DATA_T_STRING,"text/mem");
    Attributes["inner_type"]=new Attribute(DATA_T_STRING,"application/octet-stream");
    Attributes["source_class"]=new Attribute(DATA_T_STRING,"cpp");
    std::cerr<<"New mem object "<< Attributes["name"]<<" as "<<usrtype<<std::endl;
}

objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    cppmem *tmp;

    tmp=files[obj->Pathname];
    if(!tmp){
        tmp=files[obj->Pathname]=new cppmem(obj,mask,systype,usrtype,oxt);
        tmp->Write("Hello world",12,0,0,0);
    }
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