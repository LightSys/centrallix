#include "charsets.h"
#include "centrallix.h"
#include <langinfo.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	charsets.c, charsets.h                                  */
/* Author:	Daniel Rothfus                                          */
/* Creation:	June 29, 2011                                           */
/* Description: This module provides utilities for working with the     */
/*              variable character set support built into Centrallix.   */
/************************************************************************/

char* chrGetEquivalentName(char* name)
    {
    pStructInf highestCharsetNode;
    char * charsetValue;
    
        // Try to find the requested attribute
        highestCharsetNode = stLookup(CxGlobals.CharsetMap, name);
        if(highestCharsetNode)
            {
            stAttrValue(highestCharsetNode, NULL, &charsetValue, 0);
            return charsetValue;
            }
        else
            {
            // If not, return the system's name for the charset.
            return nl_langinfo(CODESET);
            }
    }

/** String Functions Follow **/
size_t chrGetCharNumber(char* character)
    {
    size_t bytesParsed;
    wchar_t firstChar = 0;
        if(!character)
            return CHR_INVALID_ARGUMENT;
        bytesParsed = mbrtowc(&firstChar, character, strlen(character), NULL);
        if(bytesParsed == (size_t) - 1 || bytesParsed == (size_t) - 2)
            {
            return CHR_INVALID_CHAR;
            }
        else
            {
            return (size_t)firstChar;
            }
    }

char* chrSubstring(char* string, size_t begin, size_t end, char* buffer, size_t* bufferLength)
    {
    size_t strCharLen, strByteLen, charsScanned = 0, bytesScanned = 0, tmpBytesScanned, bytesInStringScanned = 0;
    char* initialPos, *output;
    mbstate_t state;
        
        /** Initialize the multi-byte scanning state **/
        memset(&state, 0, sizeof(state));
        
        strByteLen = strlen(string);
        strCharLen = mbstowcs(NULL, string, 0);
        if(strCharLen == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        /** Bounds checking **/
        if(end > strCharLen)
            end = strCharLen;
        if(begin > end)
            begin = end;
        
        /** Check if this is to return the entire string. **/
        if(begin == 0 && end == strCharLen){
            *bufferLength = 0;
            return string;
        }
        
        /** Check if this is being used as a shortcut for right **/
        if(end == strCharLen){
            return chrRight(string, strCharLen - begin, bufferLength);
        }
        
        /** Find the required number of bytes for the output buffer **/
        /** Scan to the beginning character **/
        while(charsScanned < begin)
            {
            tmpBytesScanned = mbrtowc(NULL, string + bytesScanned, strByteLen - bytesScanned, &state);
            if(tmpBytesScanned == (size_t)-1 || tmpBytesScanned == (size_t)-2)
                {
                *bufferLength = CHR_INVALID_CHAR;
                return NULL;
                }
            bytesScanned += tmpBytesScanned;
            charsScanned++;
            }
        initialPos = string + bytesScanned;
        
        /** Find ending position **/
        while(charsScanned < end)
            {
            tmpBytesScanned = mbrtowc(NULL, string + bytesScanned, strByteLen - bytesScanned, &state);
            if(tmpBytesScanned == (size_t)-1 || tmpBytesScanned == (size_t)-2)
            {
                *bufferLength = CHR_INVALID_CHAR;
                return NULL;
            }
            bytesScanned += tmpBytesScanned;
            bytesInStringScanned += tmpBytesScanned;
            charsScanned++;
            }
        
        /** Now chars scanned is the size of the buffer to use **/
        if(buffer && (bytesInStringScanned < *bufferLength))
            {
            *bufferLength = 0;
            output = buffer;
            }
        else
            {
            output = (char *)nmSysMalloc(bytesInStringScanned + 1);
            if(!output)
                {
                *bufferLength = CHR_MEMORY_OUT;
                return NULL;
                }
            *bufferLength = bytesInStringScanned + 1;
            }
        
        /** Convert the string at initial pos using only so many characters **/
        strncpy(output, initialPos, bytesInStringScanned);
        
        /** Append null and return **/
        output[bytesInStringScanned] = '\0';
        return output;
    }

size_t chrSubstringIndex(char* string, char* character)
    {
    char * ptr;
    size_t tmpLen1, tmpLen2;
        if(!string || !character)
            return CHR_INVALID_ARGUMENT;
        ptr = strstr(string, character);
        if (ptr == NULL)
            {
            return CHR_NOT_FOUND;
            }
        else
            {
	    tmpLen1 = mbstowcs(NULL, string, 0);
	    tmpLen2 = mbstowcs(NULL, ptr, 0);
            if(tmpLen1 == (size_t)-1 || tmpLen2 == (size_t)-1)
                return CHR_INVALID_CHAR;
            else
                {
                return tmpLen1 - tmpLen2;
                }
            }
    }

char* chrToUpper(char* string,  char* buffer, size_t* bufferLength)
    {
    size_t stringCharLength, currentPos, newStrByteLength;
    char* toReturn;
    wchar_t* longBuffer;
        
        /** Check arguments **/
        if(!string || !bufferLength)
            {
            *bufferLength = CHR_INVALID_ARGUMENT;
            return NULL;
            }
        
        stringCharLength = mbstowcs(NULL, string, 0);
        if(stringCharLength == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        /** Create wchar_t buffer */
        longBuffer = nmSysMalloc(sizeof(wchar_t) * (stringCharLength + 1));
        if(!longBuffer){
            *bufferLength = CHR_MEMORY_OUT;
            return NULL;
        }
        mbstowcs(longBuffer, string, stringCharLength + 1);
        currentPos = stringCharLength;
        while(currentPos--)
            longBuffer[currentPos] = towupper(longBuffer[currentPos]);
        
        newStrByteLength = wcstombs(NULL, longBuffer, 0);
        if(newStrByteLength == (size_t)-1)
            {
            nmSysFree(longBuffer);
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        if(buffer && (newStrByteLength < *bufferLength))
            {
            toReturn = buffer;
            *bufferLength = 0;
            }
        else
            {
            toReturn = (char *)nmSysMalloc(newStrByteLength + 1);
            if(!toReturn)
            {
                nmSysFree(longBuffer);
                *bufferLength = CHR_MEMORY_OUT;
                return NULL;
            }
            *bufferLength = newStrByteLength + 1;
            }
        
        /** Copy over to output buffer **/
        wcstombs(toReturn, longBuffer, newStrByteLength + 1);
        nmSysFree(longBuffer);
        return toReturn;
    }

char* chrToLower(char* string,  char* buffer, size_t* bufferLength)
    {
    size_t stringCharLength, currentPos, newStrByteLength;
    char* toReturn;
    wchar_t* longBuffer;
        
        /** Check arguments **/
        if(!string || !bufferLength)
            {
            *bufferLength = CHR_INVALID_ARGUMENT;
            return NULL;
            }
        
        stringCharLength = mbstowcs(NULL, string, 0);
        if(stringCharLength == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        /** Create wchar_t buffer */
        longBuffer = nmSysMalloc(sizeof(wchar_t) * (stringCharLength + 1));
        if(!longBuffer){
            *bufferLength = CHR_MEMORY_OUT;
            return NULL;
        }
        mbstowcs(longBuffer, string, stringCharLength + 1);
        currentPos = stringCharLength;
        while(currentPos--)
            longBuffer[currentPos] = towlower(longBuffer[currentPos]);
        
        newStrByteLength = wcstombs(NULL, longBuffer, 0);
        if(newStrByteLength == (size_t)-1)
            {
            nmSysFree(longBuffer);
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        if(buffer && (newStrByteLength < *bufferLength))
            {
            toReturn = buffer;
            *bufferLength = 0;
            }
        else
            {
            toReturn = (char *)nmSysMalloc(newStrByteLength + 1);
            if(!toReturn)
            {
                nmSysFree(longBuffer);
                *bufferLength = CHR_MEMORY_OUT;
                return NULL;
            }
            *bufferLength = newStrByteLength + 1;
            }
        
        /** Copy over to output buffer **/
        wcstombs(toReturn, longBuffer, newStrByteLength + 1);
        nmSysFree(longBuffer);
        return toReturn;
    }

/* Convert a (possibly multibyte) string to mixed case (title case). */
// TODO: This is a chimera of chrToLower (above) and exp_fn_mixed (in exp_functions.c)... might want to start over to make sure need all
char* chrToMixed(char* string, char* buffer, size_t* bufferLength, char* wordlist)
    {
    size_t stringCharLength, listCharLength, currentPos, curWordLen, newStrByteLength;
    int at_boundary, i;
    char* toReturn;
    wchar_t* longBuffer, *listBuffer, *wordPtr, *savePos, *starPos, *match;
    wchar_t temp;
    XArray xaWordlist;
        longBuffer = 0;
        listBuffer = 0;
        /** Check arguments **/
        if(!string || !bufferLength)
            {
                //FIXME: probably shouldn't go here when bufferLength is null; asking for a crash
            *bufferLength = CHR_INVALID_ARGUMENT;
            goto err;
            }
        stringCharLength = mbstowcs(NULL, string, 0);
        if(stringCharLength == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            goto err;
            }
 
        /** Create wchar_t buffer **/
        longBuffer = nmSysMalloc(sizeof(wchar_t) * (stringCharLength + 1));
        if(!longBuffer)
            {
            *bufferLength = CHR_MEMORY_OUT;
            goto err;
            }
        mbstowcs(longBuffer, string, stringCharLength + 1);
        
        /** set up worlist **/
        if(wordlist)
            {
            xaInit(&xaWordlist, 16);
            /** Make a copy of the list with wide chars **/
            listCharLength = mbstowcs(NULL, wordlist, 0);
            if(listCharLength == (size_t)-1)
                {
                *bufferLength = CHR_INVALID_CHAR;
                goto err;
                }
            listBuffer = nmSysMalloc(sizeof(wchar_t) * (listCharLength + 1));
            listCharLength = mbstowcs(listBuffer, wordlist, listCharLength + 1);
            if(!listBuffer)
                {
                *bufferLength = CHR_MEMORY_OUT;
                goto err;
                }

            /** convert to XArray **/
            wordPtr = wcstok(listBuffer, L",", &savePos);
            while(wordPtr)
                {
                xaAddItem(&xaWordlist, wordPtr);
                wordPtr = wcstok(NULL, L",", &savePos);
                }
            }

        /** Convert to mixed case **/
        at_boundary = 1; /* starts on boundary */
        for(currentPos = 0 ; currentPos < stringCharLength ; currentPos++)
            {
            if(!at_boundary)
                {
                longBuffer[currentPos] = towlower(longBuffer[currentPos]);
                /** check for boundary. If any non-aplhabetic char besides 
                 ** ' and -, needs to become a boundary 
                 **/
                 //TODO: make sure locale will not be a problem for this
                 if(!iswalpha(longBuffer[currentPos]) && longBuffer[currentPos] != '\'' 
                    && longBuffer[currentPos] != '-') at_boundary = 1;
                }
            else 
                {
                /** If have a wordlist, change input to match the list **/
                if(wordlist) 
                    {
                    /** Find length of next workd in input string **/
                    curWordLen = 0;
                    while(iswalpha(*(longBuffer + currentPos + curWordLen)) && currentPos + curWordLen < stringCharLength)
                        {
                        curWordLen++;
                        }
                    /** Compare word found against the word list
                     ** If len == 0, then no alpha chars to upper/lower, so just let fall through. 
                     **/
                    if(curWordLen > 0)
                        {
                        /** null terminate to endable string compare **/
                        temp = *(longBuffer + currentPos + curWordLen);
                        *(longBuffer + currentPos + curWordLen) = L'\0';                    
                        for(i = 0 ; i < xaWordlist.nItems ; i++)
                            {
                            starPos = NULL;
                            match = xaWordlist.Items[i];
                            if(wcscasecmp(match, longBuffer + currentPos) == 0)
                                {
                                *(longBuffer + currentPos + curWordLen) = temp;
                                break;
                                }
                            else  /* No exact match, look for '*' */
                                {
                                /** Wildcard match, implies following capital.  However, do
                                 ** not apply this unless there are at least two characters
                                 ** (one to be uppercase and one to be lower) matched by the
                                 ** wildcard.  So, Mc* would not match McD, and Mac* would
                                 ** not match Mack or Macy.
                                 **/
                                 starPos = wcschr(match, L'*');
                                 if(starPos && wcsncasecmp(match, longBuffer + currentPos, starPos - match) == 0 
                                    && curWordLen > ((starPos - match) + 1)) 
                                    {
                                     *(longBuffer + currentPos + curWordLen) = temp;
                                     curWordLen = starPos - match; /* change length so next part gets looked at again */
                                     break;
                                    } 
                                else 
                                    {
                                    match = NULL;
                                    }
                                }
                            } /* end for loop */
                        /** Restore the temp value **/
                        

                        if(match) /* replace word */
                            {
                            for(i = 0 ; i < curWordLen ; i++)
                                {
                                longBuffer[currentPos + i] = *(match+i);
                                }
                            if(!starPos) at_boundary = 0;
                            }
                        else  /* No replacement. perfrom normal upper first, lower rest*/
                            {
                            *(longBuffer + currentPos + curWordLen) = temp; /* if here, never fixed it */
                            longBuffer[currentPos] = towupper(longBuffer[currentPos]);
                            for(i = 1 ; i < curWordLen ; i++)
                                {
                                longBuffer[currentPos + i] = towlower(longBuffer[currentPos + i]);
                                }
                            at_boundary = 0;
                            }
                        /** update current pos **/
                        currentPos += (curWordLen - 1);                             
                        }
                    }  
                else
                    {
                    /** There is no wordlist. Just set to uppercase and reset boundary if needed **/
                    longBuffer[currentPos] = towupper(longBuffer[currentPos]);
                    if(iswalpha(longBuffer[currentPos])) at_boundary = 0;
                    }
                }
            }      
        
        /** prepare to convert back to a standard string **/
        newStrByteLength = wcstombs(NULL, longBuffer, 0);
        if(newStrByteLength == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            goto err;
            }
        if(buffer && (newStrByteLength < *bufferLength))
            {
            toReturn = buffer;
            *bufferLength = 0;
            }
        else
            {
            toReturn = (char *)nmSysMalloc(newStrByteLength + 1);
            if(!toReturn)
                {
                *bufferLength = CHR_MEMORY_OUT;
                goto err;
                }
            *bufferLength = newStrByteLength + 1;
            }
        
        /** Copy over to output buffer **/
        wcstombs(toReturn, longBuffer, newStrByteLength + 1);
        nmSysFree(longBuffer);
        if(listBuffer) nmSysFree(listBuffer);
        return toReturn;

    err:
        if(longBuffer) nmSysFree(longBuffer);
        if(listBuffer) nmSysFree(listBuffer);
        return NULL;
    }

size_t chrCharLength(char* string)
    {
    size_t length; 
        if(!string)
            return CHR_INVALID_ARGUMENT;
        length = mbstowcs(NULL, string, 0);
        if(length == (size_t)-1)
            return CHR_INVALID_CHAR;
        else
            return length;
    }

char* chrNoOverlong(char* string)
	{
	size_t stringCharLength, newStrByteLength;
	char* toReturn;
	wchar_t* longBuffer;

        /** Check arguments **/
	if(!string)
        	return CHR_INVALID_ARGUMENT;

    /* ensure no overly large characters are included */
    int i;
    for(i = 0 ; i < strlen(string) ; i++)
        {
        if((unsigned char) string[i] == (unsigned char) 0xF4)
            {
            /* make sure is less than F4 90 */
            /* this is safe since it would only hit the null byte */
            if((unsigned char) string[i+1] >= (unsigned char)0x90) return NULL; 
            }
        /* if true, must be a header for more than 4 bytes */
        else if( (unsigned char) string[i] > (unsigned char) 0xF4) return NULL; 
        }
	
	stringCharLength = mbstowcs(NULL, string, 0);
	if(stringCharLength == (size_t)-1)
            	{
        	return NULL;
       		}	

	/** Create wchar_t buffer */
        longBuffer = nmSysMalloc(sizeof(wchar_t) * (stringCharLength + 1));
        if(!longBuffer)
        	return NULL;
        mbstowcs(longBuffer, string, stringCharLength + 1);	
	
	/** Convert back to MBS **/
	newStrByteLength = wcstombs(NULL, longBuffer, 0);
        if(newStrByteLength == (size_t)-1)
            	{
            	nmSysFree(longBuffer);
        	return NULL;
            	}
	
	toReturn = (char *)nmSysMalloc(newStrByteLength + 1);
        if(!toReturn)
            	{
                nmSysFree(longBuffer);
                return NULL;
            	}
            
        wcstombs(toReturn, longBuffer, newStrByteLength + 1);
        
	nmSysFree(longBuffer);
	
	return toReturn;
	}

char* chrRight(char* string, size_t offsetFromEnd, size_t* returnCode)
    {
    size_t currentPos = 0, numScanned = 0;
    size_t strByteLen, strCharLen, step = 0;
    mbstate_t currentState;
    
        /** Clear initial state **/
        memset(&currentState, 0, sizeof(currentState));
    
        strByteLen = strlen(string);
        strCharLen = mbstowcs(NULL, string, 0);
        if(strCharLen == (size_t)-1)
            {
            *returnCode = CHR_INVALID_CHAR;
            return NULL;
            }
        
        /** Boundary checking **/
        if (offsetFromEnd > strCharLen) 
            offsetFromEnd = strCharLen;
        if (offsetFromEnd < 0)
            offsetFromEnd = 0;
        
        /** Scan through until we hit the offset into the string that we need to. **/
        while(numScanned < (strCharLen - offsetFromEnd))
        {
            step = mbrtowc(NULL, string + currentPos, strByteLen + 1, &currentState);
            if(step == (size_t)-1 || step == (size_t)-2)
                {
                *returnCode = CHR_INVALID_CHAR;
                return NULL;
                }
            if(step == 0) /* Hit the end of the string unintentionally */
                break;
            numScanned++;
            currentPos += step;
        }
        
        /** All done - return offset into original string **/
        *returnCode = 0;
        return string + currentPos;
    }

char* chrEscape(char* string, char* escape, char* bad, char* buffer, size_t* bufferLength)
    {
    wchar_t current, *escBuf, *badBuf;
    mbstate_t state;
    char * output;
    size_t escCharLen, badCharLen, numEscapees = 0, i, j, readPos, insertPos, strByteLen, charLen;
        
        /** Check parameters **/
        if(!string || !bufferLength)
            {
            *bufferLength = CHR_INVALID_ARGUMENT;
            return NULL;
            }
        
        /** The esc and bad parameters in wchar_t form **/
        strByteLen = strlen(string);
        escCharLen = mbstowcs(NULL, escape, 0);
        badCharLen = mbstowcs(NULL, bad, 0);
	if (escCharLen == (size_t)-1 || badCharLen == (size_t)-1)
	    {
	    /** This is checked below, but we check here for clarity
	     ** as well as to have the check before we pass the values
	     ** to nmSysMalloc().
	     **/
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }

        /** Declare buffer variables here! **/
        escBuf = nmSysMalloc(sizeof(wchar_t) * (escCharLen + 2));
        if(!escBuf)
            {
            *bufferLength = CHR_MEMORY_OUT;
            return NULL;
            }
        badBuf = nmSysMalloc(sizeof(wchar_t) * (badCharLen + 1));
        if(!badBuf)
            {
            nmSysFree(escBuf);
            *bufferLength = CHR_MEMORY_OUT;
            return NULL;
            }
        
        escCharLen = mbstowcs(escBuf, escape, escCharLen + 1); /* Translate over the escape characters */
        if(escCharLen == (size_t)-1)
            {
            nmSysFree(escBuf);
            nmSysFree(badBuf);
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        escBuf[escCharLen] = '\\'; /* Add in the / character to those to escape. */
        escBuf[++escCharLen] = '\0';
        
        badCharLen = mbstowcs(badBuf, bad, badCharLen + 1); /* Translate over the bad characters */
        if(badCharLen == (size_t)-1)
            {
            nmSysFree(escBuf);
            nmSysFree(badBuf);
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        
        /** Clear the current state of the UTF-8 state holder **/
        memset(&state, '\0', sizeof(state));
        
        /** Scan through the string **/
        for(i = 0; string[i] != '\0';) 
            {
            charLen = mbrtowc(&current, string + i, strByteLen - i, &state);
            if(charLen == (size_t)-1 || charLen == (size_t)-2)
                {
                nmSysFree(escBuf);
                nmSysFree(badBuf);
                *bufferLength = CHR_INVALID_CHAR;
                return NULL;
                }
            i += charLen;
            
            for(j = 0; j < badCharLen; j++) 
                {
                if(current == badBuf[j])
                    {
                    nmSysFree(escBuf);
                    nmSysFree(badBuf);
                    *bufferLength = CHR_BAD_CHAR;
                    return NULL;
                    }
                }
            for(j = 0; j < escCharLen; j++)
                {
                if(current == escBuf[j])
                    {
                    numEscapees++;
                    break;
                    }
                }
            }
        if(numEscapees == 0)
            {
            nmSysFree(escBuf);
            nmSysFree(badBuf);
            *bufferLength = 0;
            return string;
            }
        
        /** Reinitialize state **/
        memset(&state, '\0', sizeof(state));
        
        /** Find the buffer to put the results into **/
        if(buffer && (numEscapees + strByteLen < *bufferLength))
            {
            output = buffer;
            *bufferLength = 0;
            }
        else
            {
            output = nmSysMalloc(numEscapees + strByteLen + 1);
            if(!output)
            {
                nmSysFree(escBuf);
                nmSysFree(badBuf);
                *bufferLength = CHR_MEMORY_OUT;
                return NULL;
            }
            *bufferLength = numEscapees + strByteLen + 1;
            }
        
        /** Insert back slashes **/
        for(readPos = 0, insertPos = 0; readPos < strByteLen;)
            {
            charLen = mbrtowc(&current, string + readPos, strByteLen - readPos, &state);
	    if (charLen == (size_t)-1 || charLen == (size_t)-2)
		{
		/** For clarity **/
                nmSysFree(escBuf);
                nmSysFree(badBuf);
                *bufferLength = CHR_BAD_CHAR;
                return NULL;
		}
            for(j = 0; j < escCharLen; j++)
                {
                if(current == escBuf[j])
                    {
                    output[insertPos] = '\\';
                    insertPos++;
                    break;
                    }
                }
            while(charLen--)
                {
                output[insertPos++] = string[readPos++];
                }
            }
        output[numEscapees + strByteLen] = '\0'; /* Cap off with NULL character */
        nmSysFree(escBuf);
        nmSysFree(badBuf);
        return output;
    }

char* chrRightAlign(char* string, size_t minLength, char* buffer, size_t* bufferLength)
    {
    size_t stringCharLen, newStrByteLen, c, stringByteLength;
    long long numSpaces;
    char * toReturn;
        
        if(!string || !bufferLength)
            {
            *bufferLength = CHR_INVALID_ARGUMENT;
            return NULL;
            }
        
        /** Calculate the number of spaces to add **/
        stringByteLength = strlen(string);
        stringCharLen = mbstowcs(NULL, string, 0);
        if(stringCharLen == (size_t)-1)
            {
            *bufferLength = CHR_INVALID_CHAR;
            return NULL;
            }
        numSpaces = (long long)minLength - stringCharLen;
        if(numSpaces <= 0) /* If we do not have to add spaces, return the original string. */
            {
            *bufferLength = 0;
            return string;
            }
        
        newStrByteLen = numSpaces + stringByteLength; /* The needed number of bytes in the new string */
        
        /** Set the string to return. **/
        if(buffer && (newStrByteLen < *bufferLength))
            {
            toReturn = buffer;
            *bufferLength = 0;
            }
        else
            {
            toReturn = nmSysMalloc(newStrByteLen + 1);
            if(!toReturn)
            {
                *bufferLength = CHR_MEMORY_OUT;
                return NULL;
            }
            *bufferLength = newStrByteLen + 1;
            }
        
        c = numSpaces;
        while(c--)
            {
            toReturn[c] = ' ';
            }
        c = newStrByteLen;
        while(c-- > numSpaces)
            {
            toReturn[c] = string[c - numSpaces];
            }
        toReturn[newStrByteLen] = '\0';
        return toReturn;
    }

int chrValid(int result)
    {
    return !(result == CHR_MEMORY_OUT || result == CHR_BAD_CHAR || result == CHR_INVALID_CHAR || result == CHR_INVALID_ARGUMENT || result == CHR_NOT_FOUND);
    }

