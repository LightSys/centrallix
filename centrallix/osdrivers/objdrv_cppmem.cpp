#include <list>
#include <map>
#include "objdrv.hpp"


class cppmem: public objdrv{
    std::list<char> Buffer;
public:
    cppmem(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt);
    int Close(pObjTrxTree* oxt);
    int Delete(pObject obj, pObjTrxTree* oxt);
    int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
    bool UpdateAttr(std::string attrname, pObjTrxTree* oxt);
};//end cppmem

std::map<pPathname, cppmem*> files;

int cppmem::Close(pObjTrxTree* oxt){
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

cppmem::cppmem(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
        :objdrv(obj,mask,systype,usrtype,oxt){
    obj->SubCnt=1;
    this->Obj=obj;
    this->Pathname=std::string(obj->Pathname->Pathbuf);
    Attributes["name"]=new Attribute(DATA_T_STRING,obj->Pathname->Pathbuf);
    Attributes["outer_type"]=new Attribute(DATA_T_STRING,"cpp/mem");
    Attributes["inner_type"]=new Attribute(DATA_T_STRING,"system/void");
    Attributes["content_type"]=Attributes["inner_type"];
    Attributes["source_class"]=new Attribute(DATA_T_STRING,"cpp");
}

objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    cppmem *tmp;

    tmp=files[obj->Pathname];
    if(!tmp)tmp=files[obj->Pathname]=new cppmem(obj,mask,systype,usrtype,oxt);
    return tmp;
}

char *GetName(){
    return (char *)"Virtual memory file";
}

std::list<std::string> GetTypes(){
    std::list<std::string> tmp;
    tmp.push_back("cpp/mem");
    return tmp;
}

MODULE_PREFIX("mem");
MODULE_DESC("Virtual object in memory");
MODULE_VERSION(0,0,1);