#include <list>
#include <map>
#include "objdrv.hpp"


class cppmem: public objdrv{
    std::list<char> Buffer;
public:
    int Close(pObjTrxTree* oxt);
    int Delete(pObject obj, pObjTrxTree* oxt);
    int Write(char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt);
    int Read(char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt);
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
    int cnt=(maxcnt<Buffer.size())?maxcnt:Buffer.size();
    for(int i=0;i<cnt;i++){
        *(buffer+i)=Buffer.front();
        Buffer.pop_front();
    }
    return cnt;
}

objdrv *GetInstance(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt){
    cppmem *tmp;
    tmp=files[obj->Pathname];
    return tmp;
}

char *GetName(){
    return "Virtual memory file";
}

char *GetType(){
    return "mem/list";
}

MODULE_PREFIX("mem");
MODULE_DESC("Virtual object in memory");
MODULE_VERSION(0,0,1);