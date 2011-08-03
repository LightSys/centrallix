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
/* Module: 	net_http.h, net_http.c, net_http_conn.c, net_http_sess.c, net_http_osml.c, net_http_app.c, net_http_diff.c*/
/* Author:	Micah Shennum					*/
/* Creation:	July 28, 2011  					*/
/* Description:	Network handler providing updates to clients.			*/
/************************************************************************/

#include "obj.h"
#include "net_http.h"

pXArray nht_internal_DecomposeSQL(pObjSession session, char *sql){
    pXArray cache;
    pObjQuery query;
    
    query = objMultiQuery(session, sql, NULL, 0);
    if(!query){
        mssError(0,"NHT","Open query for %s",sql);
        return NULL;
    }
    objQueryFetch(query, 0);
    cache = objMultiQueryObjects(query);
    if(!cache || xaCount(cache)<1)
        mssError(0,"NHT","Could not decompose sql statement %s",sql);
    objQueryClose(query);
    return cache;
}

///@brief fetches the result of a SQL statment as would be seen by the http client
pXArray nht_internal_FetchSQL(char *sql, pObjSession session, pNhtConn conn){
    int rowid;
    pObject obj;
    XString str;
    pXArray results;
    pObjQuery query;

    mssError(1,"NHT","Fetching %s",sql);

    //open the query
    query = objMultiQuery(session, sql, NULL, 0);
    results = nmMalloc(sizeof(XArray));
    xaInit(results,64);

    //now get results
    rowid = 1;
    while((obj=objQueryFetch(query,O_RDONLY))){
        xsInit(&str);
        nht_internal_WriteAttrsStr(obj,&str,rowid,1);
        objClose(obj);
        mssError(1,"NHT","Got %s",xsString(&str));
        xaAddItem(results,nmSysStrdup(xsString(&str)));
        xsDeInit(&str);
        rowid++;
    }//end while
    objQueryClose(query);
    return results;
}//end nht_internal_FetchSQL

//frees array of strings, as from above
void nht_internal_FreeResults(pXArray results){
    int i;
    for(i=0;i<xaCount(results);i++)
        nmSysFree(xaGetItem(results,i));
    xaDeInit(results);
    nmFree(results,sizeof(XArray));
}//end nht_internal_FreeResults

pNhtUpdate nht_internal_CreateUpdates(){
    pNhtUpdate update = nmMalloc(sizeof(NhtUpdate));
    update->Saved = nmMalloc(sizeof(XHashTable));
    xhInit(update->Saved,16,0);
    update->Notifications = nmMalloc(sizeof(XHashTable));
    xhInit(update->Notifications,8,0);
    update->Querys = nmMalloc(sizeof(XTree));
    xtInit(update->Querys,0);
    return update;
}//end nht_internal_createUpdates

int freeNotifyies(void *item,void *data){
    objCloseObserver((pObjObserver)item);
    return 0;
}

int freeResulties(void *item,void *data){
    nht_internal_FreeResults(item);
    return 0;
}

void nht_internal_FreeUpdates(pNhtUpdate update){
    if(!update)return;
    xhClear(update->Notifications,freeNotifyies,NULL);
    xhDeInit(update->Notifications);
    nmFree(update->Notifications,sizeof(XHashTable));
    xhClear(update->Saved,freeResulties,NULL);
    xhDeInit(update->Saved);
    nmFree(update->Saved,sizeof(XHashTable));
    xtDeInit(update->Querys);
    nmFree(update->Querys,sizeof(XTree));
    return;
}//end nht_internal_freeUpdates

void nht_internal_WriteDiffline(pFile fd, char add, int objid, char *text){
    char *place, *line, *tmp;
    if(!strstr(text,"%x")){
        mssError(1,"Could not find place holder in diff item %s, giving up", text);
        return;
    }
    text=nmSysStrdup(text);
    //size of handle converted to hex + null
    tmp = nmSysMalloc(sizeof(handle_t)*2+1);
    line = nmSysMalloc(strlen(text)+strlen(tmp));
    place[0]=0;
    //place begining of line
    strcat(line,text);
    place+=2;//eat %x
    strcat(line,place);
    fdWrite(fd,(add)?"A ":"D ",2,0,0);
    fdWrite(fd,line,strlen(line),0,0);
    nmSysFree(tmp);
    nmSysFree(line);
    nmSysFree(text);
    return;
}

void nht_internal_WriteDiff(pXArray prevous, pXArray results, pFile fd){
    int o;
    int preI=0;
    int resI=0;
    char *A,*D;
    int resMax=results?xaCount(results):0;
    int preMax=prevous?xaCount(prevous):0;
    while(preI<preMax && resI<resMax){
        D=xaGetItem(prevous,preI);
        A=xaGetItem(results,resI);
        if(strcmp(D,A)){
            if((o=xaFindItem(results,D))>0 && o>resI){
                while(resI<o){
                    nht_internal_WriteDiffline(fd,1,resI,xaGetItem(results,resI));
                    resI++;
                }//end ff
            }else if((o=xaFindItem(prevous,A))>0 && o>preI){
                while(preI<o){
                    nht_internal_WriteDiffline(fd,0,preI,xaGetItem(prevous,preI));
                    preI++;
                }//end ff
            }//end if in other
        }//end if differant
        preI++;resI++;
    }//end while both
    //delete anything left over
    for(;preI<preMax;preI++){
        nht_internal_WriteDiffline(fd,0,preI,xaGetItem(prevous,preI));
    }//end delete leftovers
    //now grab anything left over
    for(;resI<resMax;resI++){
        nht_internal_WriteDiffline(fd,1,resI,xaGetItem(results,resI));
    }//end send all left over
    return;
}//end nht_internal_WriteDiff

int nht_internal_writeUpdate(pNhtConn conn, pNhtUpdate updates, pObjSession session, char *path){
    int rowid;
    char *sql;
    pXArray results;
    pXArray prevous;

    //open the query
    sql = xtLookupBeginning(updates->Querys,path);
    results = nht_internal_FetchSQL(sql, session, conn);

    //now diff these things
    prevous = (pXArray)xhLookup(updates->Saved,sql);
    nht_internal_WriteDiff(prevous, results, conn->ConnFD);

    //drop last results and save these
    if(prevous)xhRemove(updates->Saved,sql);
    xhAdd(updates->Saved,sql,(char *)results);

    if(prevous){
        for(rowid=0;rowid<xaCount(prevous);rowid++)
            nmSysFree(xaGetItem(prevous,rowid));
        xaDeInit(prevous);
        nmFree(prevous,sizeof(XArray));
    }
    return 0;
}

/* We get:
     * ls__sid
     * ls__sql#
     * ls__rowcount#
     * ls__notify#
     * ls__req -- multiquery?
     * ls__autofetch
     * ls__autoclose_sr
     */
int nht_internal_GetUpdates(pNhtConn conn,pStruct url_inf){
    int i;
    int reqc;
    char *sid;
    char *sql;
    pXArray fetch;
    pStruct tmp_inf;
    char name[0xff];
    pXArray waitfor;
    pXArray results;
    pXArray sqlobjects;
    pNhtUpdate updates;
    pObjSession session;
    pObjObserver observer;
    handle_t session_handle;
    ObjObserverEventType event_t;

    /** Get the session data **///stolen from net_http_osml.c:514, but now unreconizable
    stAttrValue_ne(stLookup_ne(url_inf,"ls__sid"),&sid);
    if (!sid || !strcmp(sid,"XDEFAULT")){
        mssError(1,"NHT","Session ID required for update request");
        nht_internal_ErrorExit(conn,405,"Update request without session ID.");
        return -1;
    }
    sid++;
    session_handle = xhnStringToHandle(sid,NULL,16);
    session = (pObjSession)xhnHandlePtr(&(conn->NhtSession->Hctx), session_handle);
    if (!session){
        mssError(1,"NHT","Session ID bad for update request %s",sid);
        nht_internal_ErrorExit(conn,405,"Update request with bad session ID.");
        return -1;
    }
    updates = (pNhtUpdate)xhLookup(&NHT.UpdateLists,(char *)session);
    if(!updates){
        mssError(0,"NHT","Couldn't fetch list of update request, making new one");
        updates = nht_internal_CreateUpdates();
        xhAdd(&NHT.UpdateLists,(char *)session,(char *)updates);
    }//end if new updates

    //Get requested sql's
    tmp_inf=stLookup_ne(url_inf,"cx__numObjs");
    if(tmp_inf)reqc = strtoi(tmp_inf->StrVal,NULL,16);
    else{
        mssError(0,"NHT","Malformed update request");
        return -1;
    }
    if(errno == ERANGE){
        mssError(0,"NHT","Imposable number of sql request: %s", tmp_inf->StrVal);
        reqc=0;
    }
    waitfor = nmMalloc(sizeof(XArray));
    xaInit(waitfor,reqc);
    fetch = nmMalloc(sizeof(XArray));
    xaInit(fetch,reqc);
    //get all the requested queries
    for(i=0;i<reqc;i++){
        char *obj;
        int j,save;
        snprintf(name,0xff,"ls__notify%x",i);
        tmp_inf = stLookup_ne(url_inf,name);
        if(!tmp_inf)break;
        save=(tmp_inf->StrVal[0]!='0');
        snprintf(name,0xff,"ls__sql%x",i);
        tmp_inf = stLookup_ne(url_inf,name);
        if(!tmp_inf)break;
        sql = tmp_inf->StrVal;
        if(!save){
            mssError(0,"NHT","Add to fetch list %s",sql);
            xaAddItem(fetch,sql);
            continue;
        }
        mssError(0,"NHT","Decomposing %s",sql);
        sqlobjects = nht_internal_DecomposeSQL(session,sql);
        if(!sqlobjects || xaCount(sqlobjects)<1)
            continue;
        for(j=0;j<xaCount(sqlobjects);j++){
            observer = NULL;
            obj=xaGetItem(sqlobjects,j);
            mssError(0,"NHT","You pick up a %s",obj);
            //if we don't already have it, open a observer for it
            if(!xhLookup(updates->Notifications,obj)){
                observer=objOpenObserver(session,obj);
                if(!observer)
                    mssError(0,"NHT","Could not open observer for %s",obj);
                if(!xaFindItem(fetch,sql))
                    xaAddItem(fetch,sql);
                xaAddItem(waitfor,observer);
                xhAdd(updates->Notifications,obj,(void *)observer);
                xtAdd(updates->Querys,obj,sql);
            }//end if saveable new observer
        }//end for sqlobjects
    }//end for reqc

    fdPrintf(conn->ConnFD,"Content-Type: text/html\r\n\r\n");
    //send non-persistent request
    for(i=0;i<xaCount(fetch);i++){
        results = nht_internal_FetchSQL(xaGetItem(fetch,i), session, conn);
        nht_internal_WriteDiff(NULL,results,conn->ConnFD);
        nht_internal_FreeResults(results);
    }

    //now that that's over, get some updates!
    event_t = objPollObservers(waitfor,!xaCount(fetch),&observer,&sid);
    //write header
    while(event_t != OBJ_OBSERVER_EVENT_NONE){
        nht_internal_writeUpdate(conn,updates,session,sid);
        event_t=objPollObservers(waitfor,0,&observer,&sid);
    }

    xaDeInit(waitfor);
    nmFree(waitfor,sizeof(XArray));
    return 0;
}//end of nht_internal_GetUpdates
