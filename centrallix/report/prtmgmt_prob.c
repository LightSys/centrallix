#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "barcode.h"
#include "report.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/util.h"
#include "cxlib/xstring.h"
#include "prtmgmt.h"
#include "htmlparse.h"


/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	prtmgmt.c,prtmgmt.h					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 9, 1999   					*/
/* Description:	Provides the general non-device-specific printing 	*/
/*		functionality for the reporting output mechanism.	*/
/************************************************************************/



/** GLOBALS **/
struct
    {
    XArray	Printers;
    }
    PRTINF;


/*** prtDumpCmdStream - dump a command stream list to the console
 ***/
int
prtDumpCmdStream(pPrtCommand cmd)
    {
    while(cmd)
        {
	printf("L%4.4d ",cmd->CurRelLine);
	switch(cmd->CmdType)
	    {
	    case PRT_CMD_DOTABLE:
	        printf("DoTable.\n");
		break;
	    case PRT_CMD_DOCOLHDR:
	        printf("ColHdr:   start=%.2lf, set_width=%d, id=%d, width=%d\n",
			cmd->StartLineX, cmd->IntArgs[0], cmd->IntArgs[1], cmd->IntArgs[2]);
		break;
	    case PRT_CMD_DOCOLUMN:
	        printf("Column:   start=%.2lf, colspan=%d, id=%d, width=%d\n",
			cmd->StartLineX, cmd->IntArgs[0], cmd->IntArgs[1], cmd->IntArgs[2]);
		break;
	    case PRT_CMD_ENDTABLE:
	        printf("EndTable.\n");
		break;
	    case PRT_CMD_SETHPOS:
	        printf("SetHPos:  start=%.2lf\n",
			cmd->StartLineX);
	    case PRT_CMD_WRITESTRING: 
	        printf("String:   start=%.2lf, len=%.2lf, text='%s'\n",
	    		cmd->StartLineX, cmd->XLength, cmd->StringArgs[0]);
		break;
	    case PRT_CMD_WRITENL:
	        printf("Newline:  start=%.2lf, len=%.2lf\n",
			cmd->StartLineX, cmd->XLength);
		break;
	    case PRT_CMD_WRITEFF:
	        printf("FormFeed: start=%.2lf, len=%.2lf\n",
			cmd->StartLineX, cmd->XLength);
		break;
	    case PRT_CMD_WRITELINE:
	        printf("HorizLN:  start=%.2lf, len=%.2lf\n",
			cmd->StartLineX, cmd->XLength);
		break;
	    case PRT_CMD_ATTRON:
	    case PRT_CMD_ATTROFF:
	        if (cmd->CmdType == PRT_CMD_ATTRON)
		    printf("AttrON:   ");
		else
		    printf("AttrOFF:  ");
		if (cmd->IntArgs[0] == RS_TX_BOLD) printf("bold\n");
		else if (cmd->IntArgs[0] == RS_TX_CENTER) printf("center\n");
		else if (cmd->IntArgs[0] == RS_TX_UNDERLINE) printf("underline\n");
		else if (cmd->IntArgs[0] == RS_TX_ITALIC) printf("italic\n");
		else if (cmd->IntArgs[0] == RS_TX_EXPANDED) printf("expanded\n");
		else if (cmd->IntArgs[0] == RS_TX_COMPRESSED) printf("compressed\n");
		else if (cmd->IntArgs[0] == RS_TX_PBARCODE) printf("postal barcode\n");
		break;
	    case PRT_CMD_AUTOWRAP:
	        printf("AutoNL:   start=%.2lf, len=%.2lf\n",
			cmd->StartLineX, cmd->XLength);
		break;
	    default:
	        printf("Unknown.\n");
		break;
	    }
	cmd = cmd->Next;
	}
    }


/*** prtAllocCommand - allocates a new command structure
 ***/
pPrtCommand
prtAllocCommand(int cmdtype, char* strval, int intval)
    {
    pPrtCommand cmd;

    	/** Allocate **/
	cmd = (pPrtCommand)nmMalloc(sizeof(PrtCommand));
	if (!cmd) return NULL;
	memset(cmd, 0, sizeof(PrtCommand));

	/** Fill in the structure **/
	cmd->IntArgs[0] = intval;
	cmd->StringArgs[0] = NULL;
	if (strval)
	    {
	    cmd->StringArgs[0] = (char*)malloc((intval == -1)?(strlen(strval)+1):(intval+1));
	    if (intval != -1)
	        memcpy(cmd->StringArgs[0],strval,intval);
	    else
		strcpy(cmd->StringArgs[0],strval);
	    cmd->StringAlloc[0] = 1;
	    cmd->StringArgs[0][(intval == -1)?(strlen(strval)):(intval)] = '\0';
	    }
	cmd->CmdType = cmdtype;
	cmd->Next = NULL;
	cmd->Prev = NULL;
	cmd->StartLineX = 0;
	cmd->XLength = 0;

    return cmd;
    }


/*** prtFreeCommand - frees a command stream
 ***/
int
prtFreeCommand(pPrtCommand cmd)
    {
    pPrtCommand del;
    int i;

    	/** Walk the cmd stream, deleting one at a time **/
	while(cmd)
	    {
	    for(i=0;i<3;i++) if (cmd->StringAlloc[i]) free(cmd->StringArgs[i]);
	    del = cmd;
	    cmd = cmd->Next;
	    nmFree(del, sizeof(PrtCommand));
	    }

    return 0;
    }


/*** prtAddCommand - adds a new command to a session structure's cmd
 *** stream list.
 ***/
int
prtAddCommand(pPrtSession this, pPrtCommand cmd)
    {
    pPrtCommand* cmdptr;

    	/** Set cur line ptr if null. **/
	if (this->CmdStreamLinePtr == NULL) this->CmdStreamLinePtr = cmd;

    	/** Get the cmdptr **/
	if (this->CmdStreamTail == NULL)
	    cmdptr = &(this->CmdStream);
	else
	    cmdptr = &(this->CmdStreamTail->Next);

	/** Set the cmdptr and tail ptr **/
	cmd->Prev = this->CmdStreamTail;
	cmd->Next = NULL;
	*cmdptr = cmd;
	this->CmdStreamTail = cmd;

    return 0;
    }


/*** prtInsertCommand - inserts a command into the middle of a command stream
 *** just before the given cmd.
 ***/
int
prtInsertCommand(pPrtSession this, pPrtCommand cur, pPrtCommand newcmd)
    {

    	/** Insert newcmd **/
	if (cur->Prev) cur->Prev->Next = newcmd;
	newcmd->Next = cur;
	newcmd->Prev = cur->Prev;
	cur->Prev = newcmd;

	/** Check head ptr **/
	if (this->CmdStream == cur) this->CmdStream = newcmd;
	if (this->CmdStreamLinePtr == cur) this->CmdStreamLinePtr = newcmd;

    return 0;
    }


/*** prtGenerateSepLine - generate a separator line for a table
 ***/
int
prtGenerateSepLine(pPrtSession this, pPrtCommand cmdstream, int line_num)
    {
    int total_len=0, ck_len;
    char linebuf[512];

	/** Do separator row **/
	memset(linebuf, '=', 512);
	while(cmdstream->Next && cmdstream->Next->CurRelLine <= line_num) 
	    {
	    ck_len = cmdstream->StartLineX + cmdstream->XLength;
	    if (cmdstream->CmdType == PRT_CMD_DOCOLHDR && ck_len < cmdstream->StartLineX + cmdstream->IntArgs[0])
	        ck_len = cmdstream->StartLineX + cmdstream->IntArgs[0];
	    if (ck_len > total_len) 
	        {
		total_len = ck_len;
		linebuf[total_len-1] = ' ';
		}
	    cmdstream = cmdstream->Next;
	    }
	linebuf[total_len] = 0;
	this->Driver->Comment(this, "TABLESEP");
	this->Driver->WriteString(this, linebuf, -1);

    return 0;
    }


/*** prtGenerateLine - output one line's worth of text, formatted appropriately
 *** from a segment of a command stream.  Output should be directly to the
 *** printer content driver.
 ***/
int
prtGenerateLine(pPrtSession this, pPrtCommand cmdstream, int line_num)
    {
    pPrtCommand cmdheadptr;
    double basepos;
    int curattr;
    int total_len;
    char sbuf[80];

    	/** First, set attributes as appropriate. **/
	this->Driver->SetAttr(this, cmdstream->CurAttr);
	curattr = cmdstream->CurAttr;
	basepos = this->Driver->GetAlign(this);

	/** Thing is just an auto-nl or forced-nl? **/
	if (cmdstream->CmdType == PRT_CMD_AUTOWRAP || cmdstream->CmdType == PRT_CMD_WRITENL)
	    {
	    if (cmdstream->CmdType == PRT_CMD_WRITENL && !(cmdstream->Flags & PRT_CMD_F_NOOUTPUT))
	        {
		this->Driver->Comment(this,"NL");
		}
	    cmdstream = cmdstream->Next;
	    if (!cmdstream || cmdstream->CurRelLine > line_num) return 0;
	    }

	/** Sep. line? **/
	if (cmdstream->CmdType == PRT_CMD_TABLESEP)
	    {
	    cmdheadptr = cmdstream;
	    while(cmdheadptr->CmdType != PRT_CMD_DOTABLE) cmdheadptr = cmdheadptr->Prev;
	    prtGenerateSepLine(this, cmdheadptr, line_num);
	    return 0;
	    }

	/** In a table? **/
	if (cmdstream->CmdType == PRT_CMD_DOTABLE || cmdstream->CmdType == PRT_CMD_DOCOLHDR ||
	    cmdstream->CmdType == PRT_CMD_DOCOLUMN)
	    {
	    cmdheadptr = NULL;

	    /** Skip over 'do table' block. **/
	    if (cmdstream->CmdType == PRT_CMD_DOTABLE) 
	        {
		this->Driver->Comment(this,"TABLE");
		cmdheadptr = cmdstream;
		cmdstream = cmdstream->Next;
		}
	    if (!cmdstream || cmdstream->CurRelLine > line_num) return 0;
	    if (cmdstream->CmdType == PRT_CMD_ENDTABLE) return 0;

	    /** Search backwards to table header. **/
	    if (!cmdheadptr)
	        {
		cmdheadptr = cmdstream;
		while(cmdheadptr->CmdType != PRT_CMD_DOTABLE) cmdheadptr = cmdheadptr->Prev;
		}

	    /** Is the a row, header, or separator line? **/
	    if (cmdstream->CurRelLine != line_num && (cmdstream->CmdType == PRT_CMD_DOCOLUMN || cmdstream->CmdType == PRT_CMD_DOCOLHDR))
	        {
		prtGenerateSepLine(this, cmdstream, line_num);
		}
	    else if (cmdstream->CmdType == PRT_CMD_DOCOLUMN)
	        {
		/** Do a row of columns **/
		do  {
		    if (cmdstream->CmdType == PRT_CMD_WRITEFF) 
		        break;
		    else if (cmdstream->CmdType == PRT_CMD_DOCOLUMN)
		        {
			sprintf(sbuf,"COLUMN SPAN=%d ID=%d",cmdstream->IntArgs[0], cmdstream->IntArgs[1]);
			this->Driver->Comment(this,sbuf);
		        this->Driver->Align(this, basepos + cmdstream->StartLineX*this->Driver->GetPitch(this));
			}
		    else if (cmdstream->CmdType == PRT_CMD_WRITESTRING)
		        this->Driver->WriteString(this, cmdstream->StringArgs[0], -1);
		    else if (cmdstream->CmdType == PRT_CMD_ATTRON)
			this->Driver->SetAttr(this, curattr |= cmdstream->IntArgs[0]);
		    else if (cmdstream->CmdType == PRT_CMD_ATTROFF)
			this->Driver->SetAttr(this, curattr &= ~(cmdstream->IntArgs[0]));
	            cmdstream = cmdstream->Next;
		    } while(cmdstream && cmdstream->CurRelLine <= line_num);
		    /* while(cmdstream && cmdstream->CmdType != PRT_CMD_ENDTABLE && 
		            (cmdstream->CmdType != PRT_CMD_DOCOLHDR || cmdstream->IntArgs[0] != 0) &&
			    (cmdstream->CmdType != PRT_CMD_DOCOLUMN || cmdstream->IntArgs[0] != 0));*/
		/*this->Driver->WriteNL(this);*/
		}
	    else if (cmdstream->CmdType == PRT_CMD_DOCOLHDR)
	        {
		/** Do header row **/
		do  {
		    if (cmdstream->CmdType == PRT_CMD_WRITEFF) 
		        break;
		    else if (cmdstream->CmdType == PRT_CMD_DOCOLHDR)
		        {
			sprintf(sbuf,"COLHDR WIDTH=%d ID=%d",cmdstream->IntArgs[0],cmdstream->IntArgs[1]);
			this->Driver->Comment(this,sbuf);
		        this->Driver->Align(this, basepos + cmdstream->StartLineX*this->Driver->GetPitch(this));
			}
		    else if (cmdstream->CmdType == PRT_CMD_WRITESTRING)
		        this->Driver->WriteString(this, cmdstream->StringArgs[0], -1);
		    else if (cmdstream->CmdType == PRT_CMD_ATTRON)
			this->Driver->SetAttr(this, curattr |= cmdstream->IntArgs[0]);
		    else if (cmdstream->CmdType == PRT_CMD_ATTROFF)
			this->Driver->SetAttr(this, curattr &= ~(cmdstream->IntArgs[0]));
		    cmdstream = cmdstream->Next;
		    } while(cmdstream && cmdstream->CurRelLine <= line_num);
		    /* while(cmdstream && cmdstream->CmdType != PRT_CMD_ENDTABLE && 
		            (cmdstream->CmdType != PRT_CMD_DOCOLHDR || cmdstream->IntArgs[0] != 0) &&
			    (cmdstream->CmdType != PRT_CMD_DOCOLUMN || cmdstream->IntArgs[0] != 0));*/
		/*this->Driver->WriteNL(this);*/
		}
	    }
	else
	    {
	    /** Just write the string(s) **/
	    while(cmdstream && cmdstream->CurRelLine <= line_num)
	        {
		switch(cmdstream->CmdType)
		    {
		    case PRT_CMD_DOTABLE:
		        this->Driver->Comment(this,"TABLE");
		        break;

		    case PRT_CMD_ENDTABLE:
		        this->Driver->Comment(this,"ENDTABLE");
		        break;

		    case PRT_CMD_WRITENL:
		        this->Driver->Comment(this,"NL");
		        break;

		    case PRT_CMD_WRITEFF:
		    case PRT_CMD_AUTOWRAP:
		    case PRT_CMD_WRITELINE:
		    case PRT_CMD_SETMARGINLR:
		    case PRT_CMD_SETMARGINTB:
		        cmdstream = NULL;
		        break;

		    case PRT_CMD_SETCOLS:
		        sprintf(sbuf,"SETCOLS NUM=%d SEP=%d",cmdstream->IntArgs[0], cmdstream->IntArgs[1]);
		        this->Driver->Comment(this,sbuf);
		        cmdstream = NULL;
		        break;

		    case PRT_CMD_ATTRON:
		        curattr |= cmdstream->IntArgs[0];
			this->Driver->SetAttr(this, curattr);
			break;

		    case PRT_CMD_ATTROFF:
		        curattr &= ~(cmdstream->IntArgs[0]);
			this->Driver->SetAttr(this, curattr);
			break;

		    case PRT_CMD_DOCOLHDR:
		    case PRT_CMD_DOCOLUMN:
		        if (cmdstream->CmdType == PRT_CMD_DOCOLHDR)
		            {
			    sprintf(sbuf,"COLHDR WIDTH=%d ID=%d",cmdstream->IntArgs[0],cmdstream->IntArgs[1]);
			    this->Driver->Comment(this,sbuf);
		            this->Driver->Align(this, basepos + cmdstream->StartLineX*this->Driver->GetPitch(this));
			    }
			else
		            {
			    sprintf(sbuf,"COLUMN SPAN=%d ID=%d",cmdstream->IntArgs[0],cmdstream->IntArgs[1]);
			    this->Driver->Comment(this,sbuf);
		            this->Driver->Align(this, basepos + cmdstream->StartLineX*this->Driver->GetPitch(this));
			    }
		        if (line_num > cmdstream->CurRelLine && cmdstream->IntArgs[1] == 0)
			    prtGenerateSepLine(this, cmdstream, line_num);
			else
		            this->Driver->Align(this, basepos + cmdstream->StartLineX*this->Driver->GetPitch(this));
			break;

		    case PRT_CMD_SETHPOS:
		        sprintf(sbuf,"ALIGN X=%d",cmdstream->IntArgs[0]);
		        this->Driver->Comment(this,sbuf);
		        this->Driver->Align(this, basepos + cmdstream->IntArgs[0]);
			break;

		    case PRT_CMD_WRITESTRING:
		        this->Driver->WriteString(this, cmdstream->StringArgs[0], -1);
			break;
		    }
		if (!cmdstream) break;
		cmdstream = cmdstream->Next;
		}
	    }

    return 0;
    }



/*** prtExecuteSection - print a part of the command stream to the output
 *** device using the low-level print content driver.  We decrement the 
 *** ExecCnt variable appropriately such that when there is nothing more
 *** to do, it equals 0.
 ***/
int
prtExecuteSection(pPrtSession this)
    {
    pPrtCommand cmdptr,endptr,searchptr,termptr=NULL;
    pPrtCommand colptrs[16];
    pPrtCommand origcolptrs[16];
    pPrtCommand oldendptr;
    int i,nlines,curline,wrote_data,oldattr;
    int prevline,maxline;

    	/** Print until we get to a section-terminating command **/
	cmdptr = this->CmdStream;
	if (!cmdptr) 
	    {
	    this->CurRelY = 0;
	    return 0;
	    }

	/** Find the end command of this section **/
	endptr = cmdptr;
	while(endptr)
	    {
	    if (endptr->CmdType == PRT_CMD_WRITEFF || endptr->CmdType == PRT_CMD_WRITELINE ||
	        endptr->CmdType == PRT_CMD_SETCOLS || endptr->CmdType == PRT_CMD_SETMARGINLR ||
		endptr->CmdType == PRT_CMD_SETMARGINTB || endptr->CmdType == PRT_CMD_SETLINESPACING)
		{
		termptr = endptr;
		break;
		}
	    if (endptr->Next == NULL) break;
	    endptr = endptr->Next;
	    }

	/** Determine start of the column(s) of text (not table cols) **/
	if (termptr && endptr->Prev) endptr = endptr->Prev;
	nlines = (endptr->CurRelLine - cmdptr->CurRelLine);
	nlines = ((nlines+(this->Columns-1))/this->Columns)*this->Columns;
	searchptr = cmdptr;
	for(i=0;i<this->Columns;i++)
	    {
	    origcolptrs[i] = colptrs[i] = NULL;
	    while(searchptr && searchptr->Prev != endptr)
	        {
		if (searchptr->CurRelLine >= cmdptr->CurRelLine + (nlines/this->Columns)*i)
		    {
		    origcolptrs[i] = colptrs[i] = searchptr;
		    break;
		    }
		searchptr = searchptr->Next;
		}
	    if (searchptr && searchptr->Prev != endptr) searchptr = searchptr->Next;
	    }

	/** OK, now process all commands from cmdptr to endptr. **/
	curline = cmdptr->CurRelLine;
	while(curline < cmdptr->CurRelLine + (nlines/this->Columns))
	    {
	    wrote_data = 0;
	    for(i=0;i<this->Columns;i++)
	        {
		/** Check -- we may be at the end of the section or col **/
		if ((i < this->Columns-1 && colptrs[i] == origcolptrs[i+1]) || colptrs[i] == termptr) continue;

		/** Send the comment indicating a column sep tab **/
		if (i != 0) this->Driver->Comment(this, "CS");

		/** Generate one section of this line. **/
		this->Driver->Align(this,(double)(this->LMargin + ((this->PageX - this->RMargin - this->LMargin - (this->ColSep*(this->Columns-1)))/this->Columns + this->ColSep)*i));
		prtGenerateLine(this,colptrs[i],curline + (nlines/this->Columns)*i);
		wrote_data = 1;
		}
	    if (wrote_data)
		{
		/** Send a linefeed. **/
		this->Driver->WriteNL(this);
		}
	    curline++;
	    for(i=0;i<this->Columns;i++)
	        {
		while(colptrs[i]->Next && colptrs[i]->CurRelLine < curline + (nlines/this->Columns)*i) 
		    colptrs[i] = colptrs[i]->Next;
		}
	    }
	this->CurPageY += (nlines/this->Columns);

	/** Need to issue specific end-of-section tags to the driver? **/
	oldendptr = endptr;
	while(endptr->Prev && endptr->Prev->CurRelLine == endptr->CurRelLine) endptr = endptr->Prev;
	while(endptr)
	    {
	    if (endptr->CmdType == PRT_CMD_ENDTABLE)
	        this->Driver->Comment(this,"ENDTABLE");
	    else if (endptr->CmdType == PRT_CMD_WRITENL)
	        this->Driver->Comment(this,"NL");
	    if (endptr == oldendptr) break;
	    endptr=endptr->Next;
	    }

	/** What was endptr?  If FF, issue the FF now. **/
	if (termptr && termptr->CmdType == PRT_CMD_WRITEFF)
	    {
	    oldattr = this->Driver->GetAttr(this);
	    this->Driver->SetAttr(this,0);
	    this->Driver->WriteFF(this);
	    this->Driver->SetAttr(this,oldattr);
	    }
	else if (termptr && termptr->CmdType == PRT_CMD_WRITELINE)
	    {
	    this->Driver->WriteLine(this);
	    }

	/** Now free the list and update the session cmd ptrs. **/
	if (termptr) endptr = termptr;
	cmdptr = endptr->Next;
	endptr->Next = NULL;
	prtFreeCommand(this->CmdStream);
	this->CmdStream = cmdptr;
	if (cmdptr) cmdptr->Prev = NULL;
	if (!cmdptr) this->CmdStreamTail = NULL;
	if (!cmdptr) this->CmdStreamLinePtr = NULL;
	this->ExecCnt--;

	/** Ok -- adjust the CurRelLine on post-nlptr commands **/
	cmdptr = this->CmdStream;
	if (cmdptr) prevline = cmdptr->CurRelLine;
	maxline = 0;
	while(cmdptr)
	    {
	    cmdptr->CurRelLine -= prevline;
	    maxline = cmdptr->CurRelLine;
	    cmdptr = cmdptr->Next;
	    }

	/** Set the new max lines **/
	this->CurRelY = maxline;

    return 0;
    }



/*** prtCheckAutoWrap - checks for autowrap at the line wrap point.  If
 *** the wrap point has been reached, it breaks the appropriate text part
 *** into two pieces, or truncates it if it can't be broken, and inserts
 *** an AutoWrap item into the command stream at that point.
 ***/
int
prtCheckAutoWrap(pPrtSession this)
    {
    pPrtCommand cmdptr;
    pPrtCommand newcmd;
    double pos;
    int cnt,len,brk;
    char *ptr;

    	/** Not a string cmd? **/
	if (!this->CmdStream || this->CmdStreamTail->CmdType != PRT_CMD_WRITESTRING) return 0;

	/** Check cur rel line position on page **/
	if (this->CmdStreamTail->Prev == NULL)
	    this->CmdStreamTail->CurRelLine = 0;
	else
	    this->CmdStreamTail->CurRelLine = this->CmdStreamTail->Prev->CurRelLine;

    	/** Determine 'XLength' of the current command. **/
	len = strlen(this->CmdStreamTail->StringArgs[0]);
	this->CmdStreamTail->XLength = len * this->CurPitch;
	this->CmdStreamTail->StartLineX = 
	    (this->CmdStreamTail->Prev)?(this->CmdStreamTail->Prev->StartLineX + this->CmdStreamTail->Prev->XLength):0.0;

	/** Wrap width exceeded? **/
	if (this->CmdStreamTail->XLength + this->CmdStreamTail->StartLineX > this->CurWrapWidth)
	    {
	    cmdptr = this->CmdStreamTail;
	    cnt = ((this->CurWrapWidth - this->CmdStreamTail->StartLineX) / this->CurPitch);
	    pos = this->CurWrapWidth;
	    while(cnt >= 0)
	        {
		ptr = this->CmdStreamTail->StringArgs[0]+cnt;
		if (*ptr == ' ')
		    {
		    this->CmdStreamTail->XLength = pos - this->CmdStreamTail->StartLineX;
		    *ptr = '\0';
		    newcmd = prtAllocCommand(PRT_CMD_AUTOWRAP, NULL, 0);
		    newcmd->StartLineX = 0;
		    newcmd->XLength = 0;
		    newcmd->CurRelLine = cmdptr->CurRelLine+1;
		    newcmd->CurAttr = cmdptr->CurAttr;
		    prtAddCommand(this,newcmd);
		    this->CurRelY++;
		    do { ptr++; } while (*ptr == ' ');
		    newcmd = prtAllocCommand(PRT_CMD_WRITESTRING, ptr, 0);
		    newcmd->CurAttr = cmdptr->CurAttr;
		    newcmd->CurRelLine = cmdptr->CurRelLine+1;
		    prtAddCommand(this,newcmd);
		    if (this->CmdStreamLinePtr == cmdptr) this->CmdStreamLinePtr = newcmd;
		    return prtCheckAutoWrap(this);
		    }
		cnt--;
		pos -= this->CurPitch;
		}
	    
	    /** Couldn't find a break point? **/
	    if (this->CmdStreamTail->XLength > this->CurWrapWidth)
	        {
		/** If no chance it would fit, break at end-of-line. **/
		cnt = ((this->CurWrapWidth - this->CmdStreamTail->StartLineX) / this->CurPitch);
		ptr = this->CmdStreamTail->StringArgs[0]+cnt;
		this->CmdStreamTail->XLength = this->CurWrapWidth - this->CmdStreamTail->StartLineX;
		newcmd = prtAllocCommand(PRT_CMD_AUTOWRAP, NULL, 0);
		newcmd->StartLineX = 0;
		newcmd->XLength = 0;
		newcmd->CurRelLine = cmdptr->CurRelLine+1;
		newcmd->CurAttr = cmdptr->CurAttr;
		prtAddCommand(this,newcmd);
		this->CurRelY++;
		newcmd = prtAllocCommand(PRT_CMD_WRITESTRING, ptr, 0);
		newcmd->CurAttr = cmdptr->CurAttr;
		newcmd->CurRelLine = cmdptr->CurRelLine+1;
		prtAddCommand(this,newcmd);
		*ptr = '\0';
		if (this->CmdStreamLinePtr == cmdptr) this->CmdStreamLinePtr = newcmd;
		return prtCheckAutoWrap(this);
		}
	    else
	        {
		/** If it can fit if we break at start of cmd, break there. **/
		newcmd = prtAllocCommand(PRT_CMD_AUTOWRAP, NULL, 0);
		newcmd->StartLineX = 0;
		newcmd->XLength = 0;
		newcmd->CurRelLine = cmdptr->CurRelLine+1;
		newcmd->CurAttr = cmdptr->CurAttr;
		prtInsertCommand(this,cmdptr,newcmd);
		this->CurRelY++;
		cmdptr->CurRelLine = cmdptr->CurRelLine+1;
		cmdptr->StartLineX = 0;
		}
	    }

    return 0;
    }



/*** prtCheckPagination - check to see if the current cmd stream is exceeding
 *** the line limit, and if so, invoke a page break.
 ***/
int
prtCheckPagination(pPrtSession this)
    {
    pPrtCommand cmdptr, nlptr, newcmd, nextcmd;
    pPrtCommand tablecmd, copycmd;
    int prevline,maxline,oldtype;

    	/** Set cmdptr to last cmd. **/
	cmdptr = this->CmdStreamTail;
	if (!cmdptr) return 0;

	/** Scan backwards 'until' lines are OK **/
	while(cmdptr->CurRelLine >= this->CurMaxLines)
	    {
	    cmdptr = cmdptr->Prev;
	    }

	/** OK? **/
	if (cmdptr == this->CmdStreamTail) return 0;

	/** Need to insert a page break after current cmd. **/
	if (this->CmdStreamTablePtr)
	    {
	    /** In table?  Determine whether we need to issue the FF _before_ the tbl header. **/
	    if (cmdptr->CmdType == PRT_CMD_TABLESEP || cmdptr->Next->CmdType == PRT_CMD_TABLESEP)
	        {
		/** Find table beginning. **/
		nextcmd = cmdptr;
		while(nextcmd->CmdType != PRT_CMD_DOTABLE) nextcmd = nextcmd->Prev;

		/** Backspace to beginning of line table starts on **/
		while(nextcmd->Prev && nextcmd->Prev->CurRelLine == nextcmd->CurRelLine && 
		      nextcmd->Prev->CmdType != PRT_CMD_WRITENL && nextcmd->Prev->CmdType != PRT_CMD_AUTOWRAP) 
		     {
		     nextcmd = nextcmd->Prev;
		     }

		/** Insert the FF **/
		nlptr = newcmd = prtAllocCommand(PRT_CMD_WRITEFF, NULL, 0);
		newcmd->StartLineX = 0;
		newcmd->XLength = 0;
		newcmd->CurRelLine = 0;
		newcmd->CurAttr = nextcmd->CurAttr;
		prtInsertCommand(this, nextcmd, newcmd);
		}
	    else if (cmdptr->Next->CmdType == PRT_CMD_ENDTABLE)
	        {
		newcmd = prtAllocCommand(PRT_CMD_WRITEFF, NULL, 0);
		newcmd->StartLineX = 0;
		newcmd->XLength = 0;
		newcmd->CurRelLine = 0;
		prtAddCommand(this, newcmd);
		}
	    else
	        {
		/** Copy a new instance of the tbl header **/
		nextcmd = cmdptr->Next;
                newcmd = prtAllocCommand(PRT_CMD_ENDTABLE, NULL, 0);
                newcmd->StartLineX = nextcmd->Prev->StartLineX + nextcmd->Prev->XLength;
                newcmd->XLength = 0;
		newcmd->Flags |= PRT_CMD_F_NOOUTPUT;
                newcmd->CurRelLine = nextcmd->Prev->CurRelLine+1;
                prtInsertCommand(this, nextcmd, newcmd);
                nlptr = newcmd = prtAllocCommand(PRT_CMD_WRITEFF, NULL, 0);
                newcmd->StartLineX = 0;
                newcmd->XLength = 0;
                newcmd->CurRelLine = 0;
                prtInsertCommand(this, nextcmd, newcmd);
                tablecmd = prtAllocCommand(PRT_CMD_DOTABLE, NULL, 0);
                tablecmd->IntArgs[0] = this->CmdStreamTablePtr->IntArgs[0];
                tablecmd->IntArgs[1] = this->CmdStreamTablePtr->IntArgs[1];
                tablecmd->IntArgs[2] = this->CmdStreamTablePtr->IntArgs[2];
                tablecmd->CurAttr = this->CmdStreamTablePtr->CurAttr;
                tablecmd->StartLineX = 0;
                tablecmd->XLength = 0;
		tablecmd->Flags |= PRT_CMD_F_NOOUTPUT;
                if (this->CmdStreamTablePtr->Next->CmdType == PRT_CMD_DOCOLHDR)
                    tablecmd->CurRelLine = nextcmd->CurRelLine-2;
                else
                    tablecmd->CurRelLine = nextcmd->CurRelLine;
                prtInsertCommand(this, nextcmd, tablecmd);
    
                /** Copy the table header, if any. **/
                if (this->CmdStreamTablePtr->Next->CmdType == PRT_CMD_DOCOLHDR)
                    {
                    copycmd = this->CmdStreamTablePtr->Next;
                    while(copycmd && copycmd->CmdType != PRT_CMD_DOCOLUMN && copycmd->CmdType != PRT_CMD_ENDTABLE)
                        {
                        newcmd = prtAllocCommand(copycmd->CmdType, copycmd->StringArgs[0], copycmd->IntArgs[0]);
                        newcmd->IntArgs[1] = copycmd->IntArgs[1];
                        newcmd->IntArgs[2] = copycmd->IntArgs[2];
                        if (newcmd->CmdType == PRT_CMD_TABLESEP)
			    newcmd->CurRelLine = tablecmd->CurRelLine + 1;
			else
			    newcmd->CurRelLine = tablecmd->CurRelLine;
                        newcmd->StartLineX = copycmd->StartLineX;
                        newcmd->XLength = copycmd->XLength;
                        newcmd->CurAttr = copycmd->CurAttr;
			newcmd->Flags |= PRT_CMD_F_NOOUTPUT;
                        prtInsertCommand(this, nextcmd, newcmd);
                        copycmd = copycmd->Next;
                        }
                    }
    
                /** Set new table ptr **/
		if (nextcmd->CmdType != PRT_CMD_ENDTABLE)
		    {
                    this->CmdStreamTablePtr = tablecmd;
		    }
		}
	    }
	else if (cmdptr->Next->CmdType == PRT_CMD_AUTOWRAP || cmdptr->Next->CmdType == PRT_CMD_WRITENL)
	    {
	    /** newline? **/
	    oldtype = cmdptr->Next->CmdType;
	    cmdptr->Next->CmdType = PRT_CMD_WRITEFF;
	    nlptr = cmdptr->Next;
	    newcmd = prtAllocCommand(PRT_CMD_WRITENL, NULL, 0);
	    newcmd->CurRelLine = nlptr->CurRelLine;
	    newcmd->XLength = 0;
	    newcmd->StartLineX = 0;
	    newcmd->CurAttr = nlptr->CurAttr;
	    if (oldtype == PRT_CMD_AUTOWRAP) newcmd->Flags |= PRT_CMD_F_NOOUTPUT;
	    prtInsertCommand(this, nlptr, newcmd);
	    }
	else
	    {
	    /** hmmm... horizontal rule, etc., just insert a FF. **/
	    nlptr = newcmd = prtAllocCommand(PRT_CMD_WRITEFF, NULL, 0);
	    newcmd->CurRelLine = 0;
	    newcmd->XLength = 0;
	    newcmd->StartLineX = 0;
	    newcmd->CurAttr = cmdptr->Next->CurAttr;
	    prtInsertCommand(this, cmdptr->Next, newcmd);
	    }

	/** Now execute, since we inserted a FF. **/
	prtExecuteSection(this);
	this->CurPageY = 0;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;

	/** -just-in-case- we need to break it by issuing yet another FF, check that. **/
	if (prtCheckPagination(this) < 0) return -1;

    return 0;
    }



/*** prtProcessCmdStream - process the command stream that is currently
 *** in place on the session, and see if any commands can be 'eaten'.
 ***/
int
prtProcessCmdStream(pPrtSession this)
    {
    pPrtCommand cmdptr;

    	/** First order of business is to check autowrapping. **/
	if (prtCheckAutoWrap(this) < 0) return -1;

	/** Now check for pagination issues. **/
	if (prtCheckPagination(this) < 0) return -1;

    return 0;
    }


/*** prtWriteString - write a string to the printer.  This basically
 *** just creates a new string cmd, and puts it on the command stream, and then
 *** calls ProcessCmdStream to update the pagination, etc.
 ***/
int
prtWriteString(pPrtSession this, char* text, int len)
    {
    pPrtCommand cmd;
    char* ptr;
    char* endptr;
    int cnt;
    int idx;

	/** Loop, separating nl/cr delimited lines into writestr and writenl **/
	ptr = text;
	while(((ptr - text) < len || len == -1) && *ptr)
	    {
	    endptr = strpbrk(ptr,"\r\n\t");
	    if (endptr)
		cnt = endptr - ptr;
	    else if (len >= 0)
	        cnt = len - (ptr - text);
	    else 
	        cnt = strlen(ptr);
	   
	    /** Don't needlessly print an empty string -- only if cnt > 0 **/
	    if (cnt > 0)
	        {
	        /** Create a cmd from the text. **/
	        cmd = prtAllocCommand(PRT_CMD_WRITESTRING, ptr, cnt);
	        if (!cmd) return -1;
	        cmd->CurAttr = this->CurAttr;
	        cmd->CurRelLine = this->CurRelY;

	        /** Queue the command **/
	        prtAddCommand(this, cmd);

	        /** Process the cmd stream... **/
	        if (prtProcessCmdStream(this) < 0) return -1;
		}

	    /** Check for anything after the nl/cr **/
	    if (endptr)
	        {
		if (*endptr == '\n') prtWriteNL(this);
		if (*endptr == '\t')
		    {
		    if (this->CmdStream)
		        idx = this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength;
		    else
		        idx = 0;
		    idx = (idx+8)&~7;
		    prtSetHPos(this,(double)idx);
		    }
		ptr += (cnt+1);
		}
	    else
	        {
		ptr += cnt;
		}
	    }

    return 0;
    }


/*** prtWriteNL - write a NewLine (plus carriage return... )
 ***/
int
prtWriteNL(pPrtSession this)
    {
    pPrtCommand cmd;

    	/** Create and queue the command. **/
	cmd = prtAllocCommand(PRT_CMD_WRITENL, NULL, 0);
	if (!cmd) return -1;
	cmd->CurAttr = this->CurAttr;
	cmd->StartLineX = 0;
	cmd->XLength = 0;
	prtAddCommand(this, cmd);
	this->CurRelY++;
	cmd->CurRelLine = this->CurRelY;

	/** Process the cmd stream to check for auto-FF, etc. **/
	if (prtProcessCmdStream(this) < 0) return -1;

    return 0;
    }


/*** prtWriteFF - start a new page.
 ***/
int
prtWriteFF(pPrtSession this)
    {
    pPrtCommand cmd;
    int oldline;

    	/** Need to issue NL first for consistency? **/
	if (this->CmdStreamTail && this->CmdStreamTail->StartLineX != 0.0) 
	    {
	    /** Make sure NL didn't trigger auto-ff, if so don't issue another ff **/
	    prtWriteNL(this);
	    oldline = this->CurPageY;
	    if (prtProcessCmdStream(this) < 0) return -1;
	    if (oldline != this->CurPageY) return 0;
	    }

    	/** Create and queue the command. **/
	cmd = prtAllocCommand(PRT_CMD_WRITEFF, NULL, 0);
	if (!cmd) return -1;
	cmd->CurAttr = this->CurAttr;
	cmd->StartLineX = 0;
	cmd->XLength = 0;
	cmd->CurRelLine = 0;
	prtAddCommand(this, cmd);

	/** Execute the section. **/
	this->ExecCnt++;
	prtExecuteSection(this);
	this->Driver->Comment(this,"FF");
	this->CurPageY = 0;
	this->CurRelY = 0;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;

    return 0;
    }


/*** prtWriteLine - write a horizontal line across the page, creating a boundary
 *** between sections.
 ***/
int
prtWriteLine(pPrtSession this)
    {
    pPrtCommand cmd;

    	/** Need to issue NL first for consistency? **/
	if (this->CmdStreamTail && this->CmdStreamTail->StartLineX != 0.0) prtWriteNL(this);

    	/** Create and queue the command. **/
	cmd = prtAllocCommand(PRT_CMD_WRITELINE, NULL, 0);
	if (!cmd) return -1;
	cmd->CurAttr = this->CurAttr;
	cmd->CurRelLine = 0;
	prtAddCommand(this, cmd);

	/** Execute the section. **/
	this->ExecCnt++;
	prtExecuteSection(this);
	this->CurPageY++;
	this->CurRelY = 0;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;
    
    return 0;
    }


/*** prtSetAttr - set attributes like bold, italic, etc.
 ***/
int
prtSetAttr(pPrtSession this, int new_attr)
    {
    pPrtCommand cmd;
    int i;

	/** See what needs to be turned on and off... **/
	for(i=0;RS_TX_ALL & (1<<i);i++)
	    {
	    if ((new_attr & (1<<i)) && !(this->CurAttr & (1<<i)))
	        {
    		/** Create and queue the command. **/
		cmd = prtAllocCommand(PRT_CMD_ATTRON, NULL, 1<<i);
		if (!cmd) return -1;
		this->CurAttr |= (1<<i);
		cmd->CurAttr = this->CurAttr;
		cmd->CurRelLine = this->CurRelY;
		cmd->XLength = 0;
		cmd->StartLineX = (this->CmdStreamTail)?(this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength):0.0;
		prtAddCommand(this, cmd);
		}
	    else if ((this->CurAttr & (1<<i)) && !(new_attr & (1<<i)))
	        {
    		/** Create and queue the command. **/
		cmd = prtAllocCommand(PRT_CMD_ATTROFF, NULL, 1<<i);
		if (!cmd) return -1;
		this->CurAttr &= ~(1<<i);
		cmd->CurAttr = this->CurAttr;
		cmd->CurRelLine = this->CurRelY;
		cmd->XLength = 0;
		cmd->StartLineX = (this->CmdStreamTail)?(this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength):0.0;
		prtAddCommand(this, cmd);
		}
	    }

	this->CurPitch = this->Driver->GetPitch(this);

    return 0;
    }


/*** prtGetAttr - return the current attributes in the cmd stream.
 ***/
int
prtGetAttr(pPrtSession this)
    {
    return this->CurAttr;
    }


/*** prtDoTable - start a table
 ***/
int
prtDoTable(pPrtSession this)
    {
    pPrtCommand cmd;

    	/** Create the cmd **/
	cmd = prtAllocCommand(PRT_CMD_DOTABLE, NULL, 0);
	if (!cmd) return -1;
	
	/** Need to start a fresh line? **/
	if (this->CmdStreamTail && this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength > 0)
	    {
	    prtWriteNL(this);
	    }

	/** Build the cmd **/
	cmd->CurRelLine = this->CurRelY;
	cmd->StartLineX = 0;
	cmd->XLength = 0;
	cmd->IntArgs[0] = 0;
	cmd->IntArgs[2] = 0;
	cmd->CurAttr = this->CurAttr;
	prtAddCommand(this,cmd);
	this->CmdStreamTablePtr = cmd;

    return 0;
    }


/*** prtDoColHdr - write a column header item.  Send a prtWriteString
 *** command afterwards to print the the string itself.
 ***/
int
prtDoColHdr(pPrtSession this, int start_row, int set_width)
    {
    pPrtCommand cmd;
    pPrtCommand prevcmd;
    double start_x;

    	/** Create the cmd **/
	cmd = prtAllocCommand(PRT_CMD_DOCOLHDR, NULL, 0);
	if (!cmd) return -1;

	/** Find the previous header/dotable cmd **/
	prevcmd = this->CmdStreamTail;
	while(prevcmd && prevcmd->CmdType != PRT_CMD_DOTABLE && prevcmd->CmdType != PRT_CMD_DOCOLHDR)
	    prevcmd = prevcmd->Prev;
	if (!prevcmd) return -1;

	/** Determine starting column for this col **/
	start_x = prevcmd->IntArgs[2] + prevcmd->StartLineX;
	if (prevcmd->IntArgs[2] == 0 && prevcmd->CmdType != PRT_CMD_DOTABLE)
	    start_x = this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength + 1;
	if ((start_row && start_x != 0) || (!start_row && start_x == 0)) return -1;

	/** Determine column # **/
	if (start_row)
	    cmd->IntArgs[1] = 0;
	else
	    cmd->IntArgs[1] = prevcmd->IntArgs[1] + 1;

	/** Build the cmd **/
	cmd->StartLineX = start_x;
	cmd->XLength = 0;
	cmd->IntArgs[0] = set_width;
	cmd->IntArgs[2] = set_width;
	cmd->CurRelLine = this->CurRelY;
	cmd->CurAttr = this->CurAttr;
	prtAddCommand(this,cmd);

    return 0;
    }


/*** prtDoColumn - write a column item.  Send one or more prtWriteString
 *** commands after this to write the column itself.
 ***/
int
prtDoColumn(pPrtSession this, int start_row, int col_span)
    {
    pPrtCommand cmd, sepcmd;
    pPrtCommand prevcmd;
    double start_x;
    int set_width = 0;

    	/** Create the cmd **/
	cmd = prtAllocCommand(PRT_CMD_DOCOLUMN, NULL, 0);
	if (!cmd) return -1;

	/** Find the previous header/dotable cmd **/
	prevcmd = this->CmdStreamTail;
	while(prevcmd && prevcmd->CmdType != PRT_CMD_DOTABLE && prevcmd->CmdType != PRT_CMD_DOCOLHDR && prevcmd->CmdType != PRT_CMD_DOCOLUMN)
	    prevcmd = prevcmd->Prev;
	if (!prevcmd) return -1;

	/** Determine starting column for this col **/
	start_x = prevcmd->IntArgs[2] + prevcmd->StartLineX;
	if (prevcmd->IntArgs[2] == 0 && prevcmd->CmdType != PRT_CMD_DOTABLE)
	    start_x = this->CmdStreamTail->StartLineX + this->CmdStreamTail->XLength + 1;
	if (start_row && prevcmd->CmdType != PRT_CMD_DOTABLE) 
	    {
	    start_x = 0;
	    this->CurRelY++;
	    }

	/** Skip the separator line? **/
	if (prevcmd->CmdType == PRT_CMD_DOCOLHDR) 
	    {
	    sepcmd = prtAllocCommand(PRT_CMD_TABLESEP, NULL, 0);
	    if (!sepcmd)
	        { 
		prtFreeCommand(cmd);
		return -1;
	        }
	    sepcmd->StartLineX = 0;
	    sepcmd->XLength = 0;
	    sepcmd->CurRelLine = this->CurRelY;
	    sepcmd->CurAttr = this->CurAttr;
	    prtAddCommand(this,sepcmd);
	    prtCheckPagination(this);
	    this->CurRelY++;
	    }

	/** Determine column id # **/
	if (start_row)
	    cmd->IntArgs[1] = 0;
	else
	    cmd->IntArgs[1] = prevcmd->IntArgs[1] + prevcmd->IntArgs[0]; /* add colspan */

	/** Find header item with same col # to get its width **/
	prevcmd = this->CmdStreamTablePtr;
	while(prevcmd && (prevcmd->CmdType != PRT_CMD_DOCOLHDR || prevcmd->IntArgs[1] != cmd->IntArgs[1]))
	    prevcmd = prevcmd->Next;
	if (prevcmd) set_width = prevcmd->IntArgs[2];

	/** Build the cmd **/
	cmd->StartLineX = start_x;
	cmd->XLength = 0;
	cmd->CurRelLine = this->CurRelY;
	cmd->IntArgs[0] = col_span;
	cmd->IntArgs[2] = set_width;
	cmd->CurAttr = this->CurAttr;
	prtAddCommand(this,cmd);
	prtCheckPagination(this);

    return 0;
    }


/*** prtEndTable - end a table.
 ***/
int
prtEndTable(pPrtSession this)
    {
    pPrtCommand cmd;

    	/** Create the cmd **/
	cmd = prtAllocCommand(PRT_CMD_ENDTABLE, NULL, 0);
	if (!cmd) return -1;

	/** Add the cmd. **/
	cmd->StartLineX = 0;
	cmd->XLength = 0;
	this->CurRelY++;
	cmd->CurRelLine = this->CurRelY;
	cmd->CurAttr = this->CurAttr;
	prtAddCommand(this,cmd);
	prtCheckPagination(this);
	this->CmdStreamTablePtr = NULL;

    return 0;
    }


/*** prtSetPageGeom - sets the page geometry, which sets the number of 
 *** normal-pitch columns and rows on the page in the case a page is printed
 *** with default line spacing and attributes.
 ***/
int
prtSetPageGeom(pPrtSession this, int x, int y)
    {

    	/** Not at beginning of page?  Bark if so. **/
	if (this->CurRelY > 0 || this->CurPageY >0 || this->CmdStream) return -1;

    	/** Set the params and tell the driver about it. **/
	this->PageY = y;
	this->PageX = x;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;
	this->CurWrapWidth = (this->PageX - this->LMargin - this->RMargin - (this->Columns-1)*this->ColSep)/this->Columns;
	this->Driver->PageGeom(this, x, y);

    return 0;
    }


/*** prtSetColumns - specifies that the text on the page from this point 
 *** forward should be split vertically into <n> columns.  col_sep is given in
 *** normal-pitch columns of text, same units as 'x' in prtSetPageGeom.
 ***/
int
prtSetColumns(pPrtSession this, int n_columns, int col_sep)
    {
    char sbuf[80];

	/** Execute the section. **/
	this->ExecCnt++;
	prtExecuteSection(this);
	sprintf(sbuf,"SETCOLS NUM=%d SEP=%d",n_columns,col_sep);
	this->Driver->Comment(this,sbuf);
	this->CurRelY = 0;
	this->Columns = n_columns;
	this->ColSep = col_sep;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;
	this->CurWrapWidth = (this->PageX - this->LMargin - this->RMargin - (this->Columns-1)*this->ColSep)/this->Columns;
   
    return 0;
    }


/*** prtSetLRMargins - set the left/right margins on the page.  Begins a new
 *** section, even if we're in columns right now.
 ***/
int
prtSetLRMargins(pPrtSession this, int l_margin, int r_margin)
    {
    char sbuf[80];

	/** Execute the section. **/
	this->ExecCnt++;
	prtExecuteSection(this);
	sprintf(sbuf,"LRMARGIN L=%d R=%d",l_margin,r_margin);
	this->Driver->Comment(this,sbuf);
	this->CurRelY = 0;
	this->LMargin = l_margin;
	this->RMargin = r_margin;
	this->CurMaxLines = (this->PageY*this->LinesPerInch/6 - this->TMargin - this->BMargin - this->CurPageY)*this->Columns;
	this->CurWrapWidth = (this->PageX - this->LMargin - this->RMargin - (this->Columns-1)*this->ColSep)/this->Columns;
    
    return 0;
    }


/*** prtGetColumns - retrieve the current column parameters for printing.
 ***/
int
prtGetColumns(pPrtSession this, int* n_columns, int* col_sep)
    {
    	
	*n_columns = this->Columns;
	*col_sep = this->ColSep;

    return 0;
    }


/*** prtGetLRMargins - return the current left/right margins.
 ***/
int
prtGetLRMargins(pPrtSession this, int* l_margin, int* r_margin)
    {

    	*l_margin = this->LMargin;
	*r_margin = this->RMargin;

    return 0;
    }


/*** prtSetHPos - move to a specified horizontal position within the page or
 *** current column relative to the column's left edge.  The 'x' is given in
 *** standard pitch units.
 ***/
int
prtSetHPos(pPrtSession this, double x)
    {
    pPrtCommand cmd;

    	/** Allocate the new cmd **/
	cmd = prtAllocCommand(PRT_CMD_SETHPOS, NULL, (int)(x+0.0001));
	if (!cmd) return -1;

	/** Build the set horiz pos cmd **/
	cmd->StartLineX = x;
	cmd->XLength = 0;
	cmd->CurRelLine = this->CurRelY;
	cmd->CurAttr = this->CurAttr;
	prtAddCommand(this,cmd);

    return 0;
    }


/*** prtSetVPos - skip down to a given line on the page relative to normal-pitch
 *** lines.
 ***/
int
prtSetVPos(pPrtSession this, double y)
    {
    int cnt,i;

    	/** Exec the current contents of the cmd stream **/
	this->ExecCnt++;
	prtExecuteSection(this);
	this->CurRelY = 0;

	/** Can we skip _down_ to the position? **/
	if (y < this->CurPageY) return -1;

	/** Ok, issue enough NL's to bring us to that position. **/
	cnt = (y - this->CurPageY)*this->LinesPerInch/6;
	for(i=0;i<cnt;i++) prtWriteNL(this);

    return 0;
    }


/*** prtConvertHTML - read from one object/file that is in HTML format and then
 *** output to a print session, in HTML, or any other print content type supported
 *** by a content driver.
 ***/
int
prtConvertHTML(int (*read_fn)(), void* read_arg, pPrtSession s)
    {
    pHtmlSession hts;
    char* ptr;
    int t,enabled=0,v1,v2,v3, intable,attr,n;

    	/** Open the html session. **/
	hts = htsOpenSession(read_fn, read_arg, 0);
	if (!hts) return -1;

	/** Read tokens, converting them to print session cmds **/
	while((t = htsNextToken(hts)) != HTS_T_EOF && t != HTS_T_ERROR)
	    {
	    switch(t)
	        {
		case HTS_T_COMMENT:
		    /** Get the type of comment **/
		    ptr = htsTagPart(hts,0);
		    if (!ptr || strcmp(ptr,"HTP")) break;
		    ptr = htsTagPart(hts,1);
		    if (!ptr) break;

		    /** Check for ENABLE **/
		    if (!strcmp(ptr,"ENABLE")) enabled = 1;

		    /** All other COMMENT types require enable **/
		    if (enabled)
		        {
			if (!strcmp(ptr,"NL")) 
			    {
			    prtWriteNL(s);
			    }
			else if (!strcmp(ptr,"ALIGN"))
			    {
			    ptr = htsTagValue(hts,2);
			    if (!ptr) break;
			    v1 = strtoi(ptr,NULL,10);
			    prtSetHPos(s, v1);
			    }
			else if (!strcmp(ptr,"LRMARGIN"))
			    {
			    if (!(ptr = htsTagValue(hts,2))) break;
			    v1 = strtoi(ptr,NULL,10);
			    if (!(ptr = htsTagValue(hts,3))) break;
			    v2 = strtoi(ptr,NULL,10);
			    prtSetLRMargins(s, v1, v2);
			    }
			else if (!strcmp(ptr,"SETCOLS"))
			    {
			    if (!(ptr = htsTagValue(hts,2))) break;
			    v1 = strtoi(ptr,NULL,10);
			    if (!(ptr = htsTagValue(hts,3))) break;
			    v2 = strtoi(ptr,NULL,10);
			    prtSetColumns(s, v1, v2);
			    }
			else if (!strcmp(ptr,"TABLE"))
			    {
			    prtDoTable(s);
			    intable = 1;
			    }
			else if (!strcmp(ptr,"COLHDR"))
			    {
			    if (!(ptr = htsTagValue(hts,2))) break;
			    v1 = strtoi(ptr,NULL,10);
			    if (!(ptr = htsTagValue(hts,3))) break;
			    v2 = strtoi(ptr,NULL,10);
			    prtDoColHdr(s,(v2==0)?1:0,v1);
			    }
			else if (!strcmp(ptr,"COLUMN"))
			    {
			    if (!(ptr = htsTagValue(hts,2))) break;
			    v1 = strtoi(ptr,NULL,10);
			    if (!(ptr = htsTagValue(hts,3))) break;
			    v2 = strtoi(ptr,NULL,10);
			    prtDoColumn(s,(v2==0)?1:0,v1);
			    }
			else if (!strcmp(ptr,"TABLESEP"))
			    {
			    /** Ignore the following String **/
			    t = htsNextToken(hts);
			    if (t == HTS_T_EOF || t == HTS_T_ERROR) break;
			    }
			else if (!strcmp(ptr,"ENDTABLE"))
			    {
			    intable = 0;
			    prtEndTable(s);
			    }
			else if (!strcmp(ptr,"FF"))
			    {
			    prtWriteFF(s);
			    }
			else if (!strcmp(ptr,"DISABLE"))
			    {
			    enabled = 0;
			    if (intable)
			        {
				intable = 0;
				prtEndTable(s);
				}
			    }
			}
		    break;

		case HTS_T_TAG:
		    /** Get the type of tag **/
		    ptr = htsTagPart(hts,0);
		    if (!ptr) break;

		    /** If not enabled, BR and P mean one or two NL's **/
		    if (!enabled)
		        {
			if (!strcmp(ptr,"BR")) 
			    {
			    prtWriteNL(s);
			    }
			else if (!strcmp(ptr,"P"))
			    {
			    prtWriteNL(s);
			    prtWriteNL(s);
			    }
			}

		    /** Check for various attributes **/
		    attr = prtGetAttr(s);
		    if (!strcmp(ptr,"B"))
			prtSetAttr(s,attr | RS_TX_BOLD);
		    else if (!strcmp(ptr,"/B"))
			prtSetAttr(s,attr & ~RS_TX_BOLD);
		    else if (!strcmp(ptr,"CENTER"))
		        prtSetAttr(s,attr | RS_TX_CENTER);
		    else if (!strcmp(ptr,"/CENTER"))
		        prtSetAttr(s,attr & ~RS_TX_CENTER);
		    else if (!strcmp(ptr,"U"))
		        prtSetAttr(s,attr | RS_TX_UNDERLINE);
		    else if (!strcmp(ptr,"/U"))
		        prtSetAttr(s,attr & ~RS_TX_UNDERLINE);
		    else if (!strcmp(ptr,"I"))
		        prtSetAttr(s,attr | RS_TX_ITALIC);
		    else if (!strcmp(ptr,"/I"))
		        prtSetAttr(s,attr & ~RS_TX_ITALIC);
		    else if (!strcmp(ptr,"SMALL"))
		        prtSetAttr(s,attr | RS_TX_COMPRESSED);
		    else if (!strcmp(ptr,"/SMALL"))
		        prtSetAttr(s,attr & ~RS_TX_COMPRESSED);
		    else if (!strcmp(ptr,"BIG"))
		        prtSetAttr(s,attr | RS_TX_EXPANDED);
		    else if (!strcmp(ptr,"/BIG"))
		        prtSetAttr(s,attr & ~RS_TX_EXPANDED);
		    else if (!strcmp(ptr,"HR"))
		        prtWriteLine(s);
		    break;

		case HTS_T_STRING:
		    ptr = NULL;
		    htsStringVal(hts, &ptr, &n);
		    if (!ptr) break;
		    prtWriteString(s, ptr, n);
		    break;
		}
	    }

	/** Close the thing. **/
	htsCloseSession(hts);

    return 0;
    }


/*** prtInitialize - initialize the print management system.
 ***/
int
prtInitialize()
    {

    	/** Init the globals **/
	memset(&PRTINF, 0, sizeof(PRTINF));
	xaInit(&PRTINF.Printers, 16);

    return 0;
    }


/*** prtOpenSession - open a new printing session, using a specified output
 *** function, normally either fdWrite or objWrite, with the pFile or pObject
 *** descriptor as write_arg.
 ***/
pPrtSession
prtOpenSession(char* content_type, int (*write_fn)(), void* write_arg, int flags)
    {
    pPrtSession this;
    pPrintDriver drv;
    int i,j;

    	/** Allocate the session data **/
	this = (pPrtSession)nmMalloc(sizeof(PrtSession));
	if (!this) return NULL;

	/** Find a driver with an appropriate content type **/
	for(i=0;i<PRTINF.Printers.nItems;drv=NULL,i++)
	    {
	    drv = (pPrintDriver)(PRTINF.Printers.Items[i]);
	    if (!strcasecmp(content_type, drv->ContentType)) break;
	    }
	if (!drv)
	    {
	    nmFree(this, sizeof(PrtSession));
	    return NULL;
	    }

	/** Setup the structure **/
	memset(this, 0, sizeof(PrtSession));
	this->WriteFn = write_fn;
	this->WriteArg = write_arg;
	this->Driver = drv;
	this->PageY = 60;
	this->PageX = 80;
	this->CurWrapWidth = this->PageX;
	this->CurLineX = 0;
	this->LinesPerInch = 6;
	this->Columns = 1;
	this->CurMaxLines = this->PageY;
	this->CurPageY = 0;
	this->CurRelY = 0;
	this->CurPitch = 1.0;

	/** Call the driver to open the session **/
	this->Driver->Open(this);
	this->Driver->Comment(this, "ENABLE");
	this->Driver->PageGeom(this, this->PageY, this->PageX);

    return this;
    }


/*** prtCloseSession - close a printing session.  DOES NOT close the object
 *** or file handle (duh! you didn't give it the close function, did you,
 *** huh?  Sigh.  Silly user.)
 ***/
int
prtCloseSession(pPrtSession this)
    {

	/** Execute the section. **/
	this->ExecCnt++;
	prtExecuteSection(this);
    	this->Driver->Comment(this,"DISABLE");

    	/** Need to output a formfeed? **/
	if (this->CurPageY > 0 || this->CurRelY > 0) prtWriteFF(this);

    	/** Close the session with the driver **/
	this->Driver->Close(this);

    	/** Free the session data structure **/
	nmFree(this, sizeof(PrtSession));

    return 0;
    }


/*** prtRegisterPrintDriver - register a printer content driver with the
 *** system.
 ***/
int
prtRegisterPrintDriver(pPrintDriver drv)
    {
    xaAddItem(&PRTINF.Printers, (void*)drv);
    return 0;
    }

