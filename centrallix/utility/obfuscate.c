#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <openssl/sha.h>
#include "cxlib/mtask.h"
#include "cxlib/strtcpy.h"
#include "cxlib/mtlexer.h"
#include "cxlib/xhash.h"
#include "obj.h"
#include "cxss/cxss.h"
#include "obfuscate.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module: 	obfuscate.c, obfuscate.h				*/
/* Author:	Greg Beeley (GRB)  					*/
/* Creation:	26-Nov-2013						*/
/* Description:	This is the data obfuscation module, which provides	*/
/*		repeatable and non-repeatable obfuscation of data for	*/
/*		testing and demos.					*/
/************************************************************************/

#define OBF_KEY_SIZE		(64 / 8)
#define OBF_HEXKEYBUF_SIZE	(OBF_KEY_SIZE * 2 + 1)

#define	OBF_MAP_EMPTY		(0x7F7F7F7F)

typedef struct
    {
    int		Magnitude;
    unsigned char Hash[OBF_HEXKEYBUF_SIZE];
    int*	Mapping;
    }
    ObfMap, *pObfMap;


typedef struct
    {
    int		Init;		/* is initialized */
    XHashTable	UniqueTables;	/* tables used to track unique mappings */
    }
    OBF_t;

static OBF_t OBF = {.Init = 0};


int
obf_internal_SameCat(char* cat1, char* cat2)
    {
    return (!cat1 && !cat2) || (cat1 && cat2 && strcmp(cat1,cat2) == 0);
    }


int
obf_internal_AddMap(unsigned char* hash, int magnitude, int from_val, int to_val)
    {
    ObfMap test_map;
    pObfMap map;

	if (magnitude > 17 || from_val >= (1<<magnitude) || from_val < 0)
	    return -1;

	/** init? **/
	if (!OBF.Init)
	    {
	    OBF.Init = 1;
	    xhInit(&OBF.UniqueTables, 255, OBF_HEXKEYBUF_SIZE + sizeof(int));
	    }

	/** Find the mapping table **/
	test_map.Magnitude = magnitude;
	memcpy(test_map.Hash, hash, OBF_HEXKEYBUF_SIZE);
	map = (pObfMap)xhLookup(&OBF.UniqueTables, (char*)&test_map);
	if (!map)
	    {
	    /** Create new mapping **/
	    map = (pObfMap)nmMalloc(sizeof(ObfMap));
	    memcpy(map, &test_map, sizeof(ObfMap));
	    map->Mapping = nmSysMalloc((1<<magnitude) * sizeof(int));
	    memset(map->Mapping, 0x7F, (1<<magnitude) * sizeof(int));
	    xhAdd(&OBF.UniqueTables, (char*)map, (char*)map);
	    }

	/** Look up the value **/
	if (map->Mapping[from_val] != OBF_MAP_EMPTY && map->Mapping[from_val] != to_val)
	    return -1;

	map->Mapping[from_val] = to_val;

    return 0;
    }


/*** obf_internal_ParseWordList() - open up a word list file and parse the
 *** words in it.
 ***/
pObfWord
obf_internal_ParseWordList(pObjSession sess, pLxSession lexer, char* pathname)
    {
    pObfWord word, del, head = NULL, *tail;
    pObject words_obj = NULL;
    pLxSession words_lexer;
    int t;
    char* ptr;

	/** Open the word list **/
	words_obj = objOpen(sess, pathname, O_RDONLY, 0600, "system/file");
	if (!words_obj)
	    goto error;
	words_lexer = mlxGenericSession(words_obj, objRead, MLX_F_EOL | MLX_F_EOF | MLX_F_POUNDCOMM);
	if (!words_lexer)
	    goto error;

	/** Read in the list **/
	tail = &head;
	while((t = mlxNextToken(words_lexer)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
	    {
	    if (t != MLX_TOK_STRING && t != MLX_TOK_KEYWORD)
		{
		mssError(1,"OBF","Expected string value (word) in word list");
		mlxNotePosition(words_lexer);
		goto error;
		}

	    /** Create and link in the word **/
	    word = (pObfWord)nmMalloc(sizeof(ObfWord));
	    if (!word)
		goto error;
	    memset(word, 0, sizeof(ObfWord));
	    word->Probability = 0.0;
	    *tail = word;
	    tail = &(word->Next);

	    /** Parse the word list item **/
	    ptr = mlxStringVal(words_lexer, NULL);
	    if (!ptr)
		goto error;
	    word->Word = nmSysStrdup(ptr);

	    /** Allowed next: end-of-line, end-of-file, category, or weight **/
	    t = mlxNextToken(words_lexer);
	    if (t == MLX_TOK_EOF)
		break;
	    else if (t == MLX_TOK_EOL)
		continue;
	    else if (t == MLX_TOK_STRING || t == MLX_TOK_KEYWORD)
		{
		ptr = mlxStringVal(words_lexer, NULL);
		if (!ptr)
		    goto error;
		word->Category = nmSysStrdup(ptr);

		/** Allowed next: end-of-line, end-of-file, or weight **/
		t = mlxNextToken(words_lexer);
		if (t == MLX_TOK_EOF)
		    break;
		else if (t == MLX_TOK_EOL)
		    continue;
		}
	    if (t != MLX_TOK_DOUBLE)
		{
		mssError(1,"OBF","Wordlist line format should be: word {category} {weight}");
		goto error;
		}
	    word->Probability = mlxDoubleVal(words_lexer) / 100.0;
	    t = mlxNextToken(words_lexer);

	    /** Allowed next: end-of-line or end-of-file **/
	    if (t == MLX_TOK_EOF)
		break;
	    if (t != MLX_TOK_EOL)
		{
		mssError(1,"OBF","Wordlist line format should be: word {category} {weight}");
		goto error;
		}
	    }

	mlxCloseSession(words_lexer);
	words_lexer = NULL;
	objClose(words_obj);
	words_obj = NULL;

	return head;

    error:
	if (words_lexer)
	    mlxCloseSession(words_lexer);
	if (words_obj)
	    objClose(words_obj);
	while (head)
	    {
	    del = head;
	    head = head->Next;
	    if (del->Category) nmSysFree(del->Category);
	    if (del->Word) nmSysFree(del->Word);
	    nmFree(del, sizeof(ObfWord));
	    }
	return NULL;
    }


/*** obf_internal_BuildCatList() - given a word list, build the category
 *** summary list.
 ***/
pObfWordCat
obf_internal_BuildCatList(pObfWord wordlist)
    {
    pObfWordCat cathead;
    pObfWordCat category, newcat;

	/** Init the category list **/
	category = cathead = NULL;

	/** Walk through the word list, building the list of categories **/
	while(wordlist)
	    {
	    /** Create a new category if this is the first word in the word
	     ** list or if the category has changed.
	     **/
	    if (!category || !obf_internal_SameCat(wordlist->Category, category->Category))
		{
		newcat = (pObfWordCat)nmMalloc(sizeof(ObfWordCat));
		memset(newcat, 0, sizeof(ObfWordCat));
		if (wordlist->Category)
		    newcat->Category = nmSysStrdup(wordlist->Category);
		newcat->TotalProb = 0.0;
		newcat->CatStart = wordlist;
		if (category)
		    category->Next = newcat;
		else
		    cathead = newcat;
		category = newcat;
		}

	    /** Add in this word to the category **/
	    category->WordCnt++;
	    category->TotalProb += wordlist->Probability;
	    wordlist = wordlist->Next;
	    }

    return cathead;
    }


/*** obf_internal_ParseRule() - parse one rule from the rule file
 ***/
pObfRule
obf_internal_ParseRule(pObjSession sess, pLxSession lexer)
    {
    pObfRule rule = NULL;
    char* ptr;
    char* tokptr;
    int t;

	/** Allocate the rule **/
	rule = (pObfRule)nmMalloc(sizeof(ObfRule));
	if (!rule)
	    goto error;
	memset(rule, 0, sizeof(ObfRule));

	/** Parse the rule match condition **/
	ptr = mlxStringVal(lexer, NULL);
	if (!ptr)
	    goto error;
	if (strcmp(ptr, "*") != 0)
	    {
	    /** Rule matching is something other than * (match all) **/
	    tokptr = strtok(ptr,",");
	    while(tokptr)
		{
		if (!*tokptr || tokptr[1] != ':')
		    {
		    mssError(1,"OBF","Incorrect rule matching format: %s", tokptr);
		    goto error;
		    }
		switch(tokptr[0])
		    {
		    case 'A':	rule->Attrname = nmSysStrdup(tokptr+2);
				break;
		    case 'D':	rule->Datatype = nmSysStrdup(tokptr+2);
				break;
		    case 'N':	rule->Objname = nmSysStrdup(tokptr+2);
				break;
		    case 'T':	rule->Typename = nmSysStrdup(tokptr+2);
				break;
		    case 'V':	rule->ValueString = nmSysStrdup(tokptr+2);
				break;
		    default:	mssError(1,"OBF","Incorrect rule matching format: %s", tokptr);
				goto error;
		    }
		tokptr = strtok(NULL,",");
		}
	    }

	/** Hash inputs and flags **/
	t = mlxNextToken(lexer);
	if (t != MLX_TOK_STRING)
	    {
	    mssError(1,"OBF","Expected hash input specification following rule match spec");
	    goto error;
	    }
	ptr = mlxStringVal(lexer, NULL);
	if (!ptr)
	    goto error;

	rule->NoObfuscate = strchr(ptr, 'X')?1:0;

	if (!rule->NoObfuscate)
	    rule->EquivAttr = strchr(ptr, '=')?1:0;

	if (!rule->NoObfuscate && !rule->EquivAttr)
	    {
	    rule->HashKey = strchr(ptr, 'K')?1:0;
	    rule->HashAttr = strchr(ptr, 'A')?1:0;
	    rule->HashName = strchr(ptr, 'N')?1:0;
	    rule->HashType = strchr(ptr, 'T')?1:0;
	    rule->HashValue = strchr(ptr, 'V')?1:0;
	    }

	/** Parameter? **/
	t = mlxNextToken(lexer);
	if (t == MLX_TOK_EOL || t == MLX_TOK_EOF)
	    {
	    /** We're done **/
	    goto done;
	    }
	if (t != MLX_TOK_STRING)
	    {
	    mssError(1,"OBF","Expected string or end-of-line after hash input spec");
	    goto error;
	    }
	ptr = mlxStringVal(lexer, NULL);
	if (!ptr)
	    goto error;
	rule->Param = nmSysStrdup(ptr);

	/** Must be at end-of-line or end-of-file **/
	t = mlxNextToken(lexer);
	if (t != MLX_TOK_EOL && t != MLX_TOK_EOF)
	    {
	    mssError(1,"OBF","Expected end-of-line after obfuscation options / wordlist filename");
	    goto error;
	    }

	/** Parameter is an OSML pathname? **/
	if (rule->Param[0] == '/')
	    {
	    /** Yes - load a word list **/
	    rule->Wordlist = obf_internal_ParseWordList(sess, lexer, rule->Param);
	    if (!rule->Wordlist)
		goto error;
	    rule->Catlist = obf_internal_BuildCatList(rule->Wordlist);
	    }

    done:
	return rule;

    error:
	if (rule)
	    {
	    if (rule->Param) nmSysFree(rule->Param);
	    if (rule->Attrname) nmSysFree(rule->Attrname);
	    if (rule->Datatype) nmSysFree(rule->Datatype);
	    if (rule->Objname) nmSysFree(rule->Objname);
	    if (rule->Typename) nmSysFree(rule->Typename);
	    if (rule->ValueString) nmSysFree(rule->ValueString);
	    nmFree(rule, sizeof(ObfRule));
	    }
	return NULL;
    }


/*** obfOpenSession() - opens a new "session" for obfuscating data.  This
 *** opens up a rule file (through objOpen, i.e., through the OSML), and
 *** sets a key value to use for future obfObfuscateDataSess calls.
 ***/
pObfSession
obfOpenSession(pObjSession objsess, char* rule_file_osml_path, char* key)
    {
    pObfSession s = NULL;
    unsigned char binkey[OBF_KEY_SIZE];
    char* hexdigit = "0123456789abcdef";
    int i;
    pObject rules_obj = NULL;
    pLxSession lexer = NULL;
    pObfRule rule, *ruletail;
    int t;

	/** Allocate the session and set it up **/
	s = (pObfSession)nmMalloc(sizeof(ObfSession));
	if (!s)
	    goto error;
	memset(s, 0, sizeof(ObfSession));
	if (rule_file_osml_path)
	    strtcpy(s->RuleFilePath, rule_file_osml_path, sizeof(s->RuleFilePath));
	if (key && *key)
	    {
	    /** Use the key the user provided **/
	    s->Key = nmSysStrdup(key);
	    }
	else
	    {
	    /** Generate a random key **/
	    s->Key = nmSysMalloc(OBF_HEXKEYBUF_SIZE); /* 64 bit hexstring */
	    if (!s->Key)
		goto error;
	    if (cxssGenerateKey(binkey, sizeof(binkey)) < 0)
		goto error;
	    for(i=0;i<OBF_KEY_SIZE;i++)
		{
		s->Key[i*2] = hexdigit[binkey[i] >> 4];
		s->Key[i*2 + 1] = hexdigit[binkey[i] & 0x0f];
		}
	    s->Key[OBF_HEXKEYBUF_SIZE - 1] = '\0';
	    }

	/** Load the rules **/
	if (*s->RuleFilePath)
	    {
	    /** Open the object and start a lexer session on it **/
	    rules_obj = objOpen(objsess, s->RuleFilePath, O_RDONLY, 0600, "system/file");
	    if (!rules_obj)
		goto error;
	    lexer = mlxGenericSession(rules_obj, objRead, MLX_F_IFSONLY | MLX_F_EOL | MLX_F_EOF | MLX_F_POUNDCOMM);
	    if (!lexer)
		goto error;
	    ruletail = &(s->RuleList);

	    /** Read in the rules **/
	    while((t = mlxNextToken(lexer)) != MLX_TOK_EOF && t != MLX_TOK_ERROR)
		{
		/** blank line or line with just a comment? **/
		if (t == MLX_TOK_EOL) continue;
		if (t != MLX_TOK_STRING)
		    {
		    mssError(1,"OBF","Could not parse obfuscation rule: expected rule string");
		    goto error;
		    }

		/** Create a new rule **/
		rule = obf_internal_ParseRule(objsess, lexer);
		if (!rule)
		    {
		    mssError(1,"OBF","Could not parse obfuscation rule");
		    goto error;
		    }

		/** Add to the rule list **/
		*ruletail = rule;
		ruletail = &(rule->Next);
		}

	    mlxCloseSession(lexer);
	    lexer = NULL;
	    objClose(rules_obj);
	    rules_obj = NULL;
	    }

	return s;

    error:
	if (lexer)
	    mlxCloseSession(lexer);
	if (rules_obj)
	    objClose(rules_obj);
	if (s)
	    {
	    if (s->Key)
		nmSysFree(s->Key);
	    nmFree(s, sizeof(ObfSession));
	    }
	return NULL;
    }


/*** obfCloseSession - close an existing obfuscation session.
 ***/
int
obfCloseSession(pObfSession sess)
    {
    pObfRule rule, ruledel;
    pObfWord word, worddel;
    pObfWordCat cat, catdel;

	/** Loop through list of rules to free them up **/
	rule = sess->RuleList;
	while(rule)
	    {
	    ruledel = rule;
	    rule = rule->Next;

	    /** Release word list **/
	    word = ruledel->Wordlist;
	    while(word)
		{
		worddel = word;
		word = word->Next;

		if (worddel->Category) nmSysFree(worddel->Category);
		if (worddel->Word) nmSysFree(worddel->Word);
		nmFree(worddel, sizeof(ObfWord));
		}

	    /** Release category list **/
	    cat = ruledel->Catlist;
	    while(cat)
		{
		catdel = cat;
		cat = cat->Next;

		if (catdel->Category) nmSysFree(catdel->Category);
		nmFree(catdel, sizeof(ObfWordCat));
		}

	    /** Release strings **/
	    if (ruledel->Param) nmSysFree(ruledel->Param);
	    if (ruledel->Attrname) nmSysFree(ruledel->Attrname);
	    if (ruledel->Datatype) nmSysFree(ruledel->Datatype);
	    if (ruledel->Objname) nmSysFree(ruledel->Objname);
	    if (ruledel->Typename) nmSysFree(ruledel->Typename);
	    if (ruledel->ValueString) nmSysFree(ruledel->ValueString);
	    nmFree(ruledel, sizeof(ObfRule));
	    }

	/** Release the session itself **/
	if (sess->Key) nmSysFree(sess->Key);
	nmFree(sess, sizeof(ObfSession));

    return 0;
    }


/*** obf_internal_GetBits() - gets one or more bits from the hash string
 ***/
int
obf_internal_GetBits(unsigned char* hash, int *bitstart, int bitcnt)
    {
    int result = 0;
    int idx, shift;

	/** Get as many bits as asked for **/
	while(bitcnt)
	    {
	    result *= 2;
	    idx = (*bitstart)/8;
	    shift = (*bitstart)%8;
	    result |= ((hash[idx]>>shift) & 0x01);
	    (*bitstart)++;
	    if ((*bitstart) >= 160)
		(*bitstart) = 0;
	    bitcnt--;
	    }

    return result;
    }


int
obf_internal_ObfuscateInteger(unsigned char* hash, unsigned char* hash_novalue, int* bitcnt, int input_value)
    {
    int orig_input_value = input_value;
    int output_value = 0;
    int sign = 1;
    int powtwo = 1;
    int use_map = ((*bitcnt) == 0) && hash_novalue;

	/** Special case **/
	if (input_value == 0)
	    return 0;

	/** Normalize the sign **/
	if (input_value < 0)
	    {
	    sign = -1;
	    input_value *= -1;
	    }

	/** compute power-of-2 size **/
	while (input_value >= (1<<powtwo)) powtwo++;

	/** Create a new value based on random data **/
	while (input_value) 
	    {
	    input_value >>= 1;
	    output_value <<= 1;
	    output_value |= (output_value)?(obf_internal_GetBits(hash, bitcnt, 1)):1;
	    }

	/** Check the mapping? **/
	if (powtwo <= 17 && use_map)
	    {
	    while(obf_internal_AddMap(hash_novalue, powtwo, output_value, orig_input_value) < 0)
		{
		output_value = (output_value + 1)&((1<<powtwo)-1);
		if (output_value == 0)
		    output_value = 1<<(powtwo-1);
		}
	    }

    return output_value * sign;
    }


int
obf_internal_ObfuscateIntegerMultiples(unsigned char* hash, unsigned char* hash_novalue, int* bitcnt, int input_value)
    {
    int output_value = input_value;
    int whichfactors;

	whichfactors = obf_internal_GetBits(hash, bitcnt, 3);
	if (whichfactors == 0x07) whichfactors = 0;

	if (obf_internal_GetBits(hash, bitcnt, 1))
	    {
	    /* increase */
	    if (whichfactors & 1) output_value *= 2;
	    if (whichfactors & 2) output_value *= 3;
	    if (whichfactors & 4) output_value *= 5;
	    }
	else
	    {
	    /* decrease */
	    if ((whichfactors & 1) && output_value%2 == 0)
		{
		output_value /= 2;
		}
	    if ((whichfactors & 2) && output_value%3 == 0)
		{
		output_value /= 3;
		}
	    if ((whichfactors & 4) && output_value%5 == 0)
		{
		output_value /= 5;
		}
	    }

    return output_value;
    }


#define OBF_PUNCT   " \t\r\n-,;\"&./?!%()[]"

/** 104 consonants **/
static char* obf_consonants[] = 
    {
    "ght","ngl","sch","scr","shr","spl","spr","squ","str",
    "thr",
    "bl","br","ch","cl","cr","ct","dr","fl","fr","gh","gl",
    "gr","nc","nd","ng","nt","pl","pr","qu","rn","rs","rt",
    "sc","sh","sk","sl","sm","sn",
    "sp","st","sw","th","tr","tw","wh","wr",
    "bb","cc","dd","ff","gg","hh","kk","ll","mm",
    "nn","pp","rr","ss","tt","vv","xx","zz",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "t","n","s","h","r",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "t","n","s","h","r",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "b","c","d","f","g","h","j","k","l","m","n",
    "p","q","r","s","t","v","w","x","y","z",
    "t","n","s","h","r",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    "t","n","s",
    };

/** 64 vowels **/
static char* obf_vowels[] =
    {
    "igh",
    "uoy","aia",
    "ae","ai","ao","au","ay",
    "ea","ee","ei","eo","eu","ey",
    "ia","ie","io",
    "oa","oe","oi","oo","ou","ow","oy",
    "ua","ue","ui","uo",
    "a","e","i","o","u","y",
    "a","e","i","o","u","y",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o",
    "a","e",
    "a","e",
    "e",
    "a","e","i","o","u","y",
    "a","e","i","o","u","y",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o",
    "a","e",
    "a","e",
    "e",
    "a","e","i","o","u","y",
    "a","e","i","o","u","y",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o","u",
    "a","e","i","o",
    "a","e",
    "a","e",
    "e"
    };

static char* obf_lower64 = "abcdefghijklmnopqrstuvwxyzetaoinshrdlcumwfgypbetaoinshrdletaoinshe";
static char* obf_upper64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZETAOINSHRDLCUMWFGYPBETAOINSHRDLETAOINSHE";
static char* obf_num16 = "0123456789012301";

int
obf_internal_GeneratePhonemeString(pXString str, unsigned char* hash, int* bitcnt, int num_phonemes, int upper_first, int upper_subseq)
    {
    int do_vowel;
    int r;
    char* phoneme;
    char* ptr;
    char ch;
    int cnt = 0;

	/** Special case - don't just use any phoneme if length == 1 **/
	if (num_phonemes == 1)
	    {
	    if (obf_internal_GetBits(hash, bitcnt, 1))
		xsConcatenate(str, upper_first?"A":"a", 1);
	    else
		xsConcatenate(str, "I", 1);
	    return 0;
	    }

	/** Randomly begin with consonant or vowel **/
	do_vowel = obf_internal_GetBits(hash, bitcnt, 1);

	while(cnt < num_phonemes)
	    {
	    /** Grab a vowel or a consonant **/
	    if (do_vowel)
		{
		r = obf_internal_GetBits(hash, bitcnt, 7);
		if (r >= sizeof(obf_vowels)/sizeof(char*))
		    r -= (128 - sizeof(obf_vowels)/sizeof(char*));
		phoneme = obf_vowels[r];
		}
	    else
		{
		r = obf_internal_GetBits(hash, bitcnt, 8);
		if (r >= sizeof(obf_consonants)/sizeof(char*))
		    r -= (256 - sizeof(obf_consonants)/sizeof(char*));
		phoneme = obf_consonants[r];
		}

	    /** Concatenate it to the string, minding upper/lower case **/
	    for(ptr=phoneme; *ptr; ptr++)
		{
		if (((ptr == phoneme && cnt == 0) && upper_first) || (!(ptr == phoneme && cnt == 0) && upper_subseq))
		    ch = toupper(*ptr);
		else if (((ptr == phoneme && cnt == 0) && !upper_first) || (!(ptr == phoneme && cnt == 0) && !upper_subseq))
		    ch = tolower(*ptr);
		xsConcatenate(str, &ch, 1);
		}
	    do_vowel = !do_vowel;
	    cnt++;
	    }

    return 0;
    }


/*** obf_internal_CountPhonemes() - count the number of phonemes (according
 *** to our list above, this is not linguistically accurate, as many of our
 *** "phonemes" in these lists consist of multiple phonemes) and return that
 *** count.  Return -1 if the word does not contain an alternating series of
 *** consonant/vowel phonemes.
 ***/
int
obf_internal_CountPhonemes(char* str, int len)
    {
    int last_was_vowel = 0;
    int last_was_consonant = 0;
    char* ptr;
    char* phoneme;
    int phoneme_cnt = 0;
    int i;
    int found_phoneme;

	/** Special case single-phoneme words **/
	if (len == 1 && (*str == 'a' || *str == 'A' || *str == 'i' || *str == 'I'))
	    return 1;

	ptr = str;
	while(*ptr && ptr < str+len)
	    {
	    found_phoneme = 0;

	    /** Check for a vowel next **/
	    if (!last_was_vowel)
		{
		for(i=0;i<sizeof(obf_vowels)/sizeof(char*);i++)
		    {
		    phoneme = obf_vowels[i];
		    if (!strncasecmp(phoneme, ptr, strlen(phoneme)))
			{
			last_was_vowel = 1;
			last_was_consonant = 0;
			found_phoneme = 1;
			phoneme_cnt++;
			ptr += strlen(phoneme);
			break;
			}
		    }
		if (found_phoneme) continue;
		}

	    /** Check for consonant **/
	    if (!last_was_consonant)
		{
		for(i=0;i<sizeof(obf_consonants)/sizeof(char*);i++)
		    {
		    phoneme = obf_consonants[i];
		    if (!strncasecmp(phoneme, ptr, strlen(phoneme)))
			{
			last_was_vowel = 0;
			last_was_consonant = 1;
			found_phoneme = 1;
			phoneme_cnt++;
			ptr += strlen(phoneme);
			break;
			}
		    }
		if (found_phoneme) continue;
		}

	    /** Nothing matched **/
	    return -1;
	    }

	/** Only I and A allowed for phoneme_cnt == 1 **/
	if (phoneme_cnt == 1)
	    return -1;

    return phoneme_cnt;
    }


int
obf_internal_ObfuscateString(char* src, pXString dst, unsigned char* hash, unsigned char* hash_novalue, int* bitcnt, pObfWord wordlist, pObfWordCat catlist)
    {
    pObfWord search;
    pObfWord found_word;
    pObfWordCat found_cat;
    pObfWordCat searchcat;
    char* sepptr;
    int seplen;
    char* segment;
    int segmentlen;
    int ival, n, limit, bits;
    double prob;
    double found_prob;
    double accumprob;
    int upper_first;
    int upper_subseq;
    int i;
    char ch;

	/** Separate the string into chunks. **/
	sepptr = segment = src;
	while(sepptr)
	    {
	    /** Get ready for the next segment **/
	    seplen = strspn(sepptr, OBF_PUNCT);
	    xsConcatenate(dst, sepptr, seplen);
	    segment = sepptr + seplen;

	    /** Find the next segment of the string **/
	    sepptr = strpbrk(segment, OBF_PUNCT);
	    if (!sepptr)
		segmentlen = strlen(segment);
	    else
		segmentlen = sepptr - segment;

	    /** Decide how to handle this segment... **/
	    if (!segmentlen)
		continue;

	    /** Integer in the string **/
	    if (strspn(segment, "0123456789") == segmentlen && segmentlen < 10)
		{
		ival = strtol(segment, NULL, 10);
		ival = obf_internal_ObfuscateInteger(hash, (segment[segmentlen] == '\0')?hash_novalue:NULL, bitcnt, ival);
		xsConcatPrintf(dst, "%d", ival);
		continue;
		}

	    /** Wordlist lookup? **/
	    if (wordlist && catlist)
		{
		searchcat = catlist;
		found_cat = NULL;
		found_word = NULL;
		found_prob = 0.0;

		/** Look through the list of categories **/
		while(searchcat)
		    {
		    search = searchcat->CatStart;

		    /** Look through the list of words in the category **/
		    while(search)
			{
			/** Category section change? **/
			if (!obf_internal_SameCat(searchcat->Category, search->Category))
			    break;
			
			/** Word match? **/
			if (search->Word && strlen(search->Word) == segmentlen && strncasecmp(search->Word, segment, segmentlen) == 0)
			    {
			    if (searchcat->TotalProb > 0.0)
				prob = search->Probability / searchcat->TotalProb;
			    else
				prob = 1.0/searchcat->WordCnt;
			    if (prob > found_prob)
				{
				found_word = search;
				found_cat = searchcat;
				found_prob = prob;
				}
			    break;
			    }
			search = search->Next;
			}

		    searchcat = searchcat->Next;
		    }

		/** Did we find a match? **/
		if (found_word)
		    {
		    /** Select a new item from the category **/
		    bits = 0;
		    limit = 1;
		    while(limit < found_cat->WordCnt)
			{
			limit <<= 1;
			bits++;
			}
		    if (found_cat->TotalProb)
			{
			/** Increase randomness 8x if using explicit probabilities. **/
			limit <<= 3;
			bits += 3;
			}
		    prob = obf_internal_GetBits(hash, bitcnt, bits) / (double)limit;
		    search = found_cat->CatStart;
		    accumprob = 0.0;
		    while(prob > accumprob)
			{
			accumprob += (search->Probability > 0.0)?(search->Probability/found_cat->TotalProb):(1.0/found_cat->WordCnt);
			if (prob <= accumprob)
			    break;
			search = search->Next;
			if (!search || !obf_internal_SameCat(found_cat->Category, search->Category))
			    {
			    search = found_cat->CatStart;
			    break;
			    }
			}

		    /** Add the selected word to the string we're building,
		     ** paying attention to capitalization.
		     **/
		    if (search->Word)
			{
			upper_first = upper_subseq = 0;
			if (isupper(segment[0]))
			    upper_first = 1;
			if (segment[1] && isupper(segment[1]))
			    upper_subseq = 1;
			for(i=0; i<strlen(search->Word); i++)
			    {
			    ch = search->Word[i];
			    if ((i == 0 && upper_first) || (i > 0 && upper_subseq))
				ch = toupper(ch);
			    else
				ch = tolower(ch);
			    xsConcatenate(dst, &ch, 1);
			    }
			}

		    continue;
		    }
		}

	    /** Not in any word list.  Check for phonetical sequence **/
	    n = obf_internal_CountPhonemes(segment, segmentlen);
	    if (n > 0)
		{
		if (n > 1 && obf_internal_GetBits(hash, bitcnt, 1))
		    n--;
		if (obf_internal_GetBits(hash, bitcnt, 1))
		    n++;
		upper_first = upper_subseq = 0;
		if (isupper(segment[0]))
		    upper_first = 1;
		if (segment[1] && isupper(segment[1]))
		    upper_subseq = 1;
		obf_internal_GeneratePhonemeString(dst, hash, bitcnt, n, upper_first, upper_subseq);
		continue;
		}

	    /** Ok, handle as an abstract string **/
	    for(i=0; i<segmentlen; i++)
		{
		ch = segment[i];
		if (ch >= '0' && ch <= '9')
		    {
		    ch = obf_num16[obf_internal_GetBits(hash, bitcnt, 4)];
		    if (i < segmentlen-1 && segment[i+1] >= '0' && segment[i+1] <= '9')
			{
			n = obf_internal_GetBits(hash, bitcnt, 3);
			if (n == 0)
			    ch = 0;
			else if (n == 7)
			    {
			    xsConcatenate(dst, &ch, 1);
			    ch = obf_num16[obf_internal_GetBits(hash, bitcnt, 4)];
			    }
			}
		    }
		else if (ch >= 'a' && ch <= 'z')
		    {
		    ch = obf_lower64[obf_internal_GetBits(hash, bitcnt, 6)];
		    if (i < segmentlen-1 && segment[i+1] >= 'a' && segment[i+1] <= 'z')
			{
			n = obf_internal_GetBits(hash, bitcnt, 3);
			if (n == 0)
			    ch = 0;
			else if (n == 7)
			    {
			    xsConcatenate(dst, &ch, 1);
			    ch = obf_lower64[obf_internal_GetBits(hash, bitcnt, 6)];
			    }
			}
		    }
		else if (ch >= 'A' && ch <= 'Z')
		    {
		    ch = obf_upper64[obf_internal_GetBits(hash, bitcnt, 6)];
		    if (i < segmentlen-1 && segment[i+1] >= 'A' && segment[i+1] <= 'Z')
			{
			n = obf_internal_GetBits(hash, bitcnt, 3);
			if (n == 0)
			    ch = 0;
			else if (n == 7)
			    {
			    xsConcatenate(dst, &ch, 1);
			    ch = obf_upper64[obf_internal_GetBits(hash, bitcnt, 6)];
			    }
			}
		    }

		if (ch)
		    xsConcatenate(dst, &ch, 1);
		}
	    }

    return 0;
    }


/*** obfObfuscateData() - the core obfuscation function, but which requires
 *** the 'which' (defaults to KANTV) and 'param' (defaults to empty)
 *** parameters to be passed since it does not look at a rule file.  A
 *** 'key' can also be supplied; if it is not supplied, the empty string
 *** is used for a key.
 ***
 *** Returns 0 on success, -1 on failure.
 ***
 *** dstval is set to static data for money and datetime types; these
 *** values must be copied out before the next call to this function.
 ***
 *** dstval gets a nmSysMalloc() allocated string for string data types;
 *** this value will be freed on subsequent calls to this function, so
 *** the data should be consumed or copied out before subsequent calls.
 ***/
int
obfObfuscateData(pObjData srcval, pObjData dstval, int data_type, char* attrname, char* objname, char* type_name, char* key, char* which, char* param, pObfWord wordlist, pObfWordCat catlist)
    {
    SHA_CTX sha1ctx;
    int round;
    char* val_str;
    unsigned char hash[SHA_DIGEST_LENGTH];
    unsigned char hash_novalue[SHA_DIGEST_LENGTH];
    static MoneyType m;
    static DateTime dt;
    static char* str = NULL;
    int bitcnt = 0;
    int iv, dv;
    int scale;
    XString xs;

	/** Empty param? **/
	if (!param)
	    param = "";

	/** Generate the hash which becomes our random source **/
	if (strchr(which, 'V'))
	    {
	    if (data_type == DATA_T_MONEY || data_type == DATA_T_DATETIME || data_type == DATA_T_STRING)
		val_str = objDataToStringTmp(data_type, srcval->Generic, 0);
	    else
		val_str = objDataToStringTmp(data_type, srcval, 0);
	    }
	SHA1_Init(&sha1ctx);
	for(round=0; round<2; round++)
	    {
	    if (strchr(which, 'K') && key)
		SHA1_Update(&sha1ctx, key, strlen(key));
	    if (strchr(which, 'A') && attrname)
		SHA1_Update(&sha1ctx, attrname, strlen(attrname));
	    if (strchr(which, 'N') && objname)
		SHA1_Update(&sha1ctx, objname, strlen(objname));
	    if (strchr(which, 'T') && type_name)
		SHA1_Update(&sha1ctx, type_name, strlen(type_name));
	    if (strchr(which, 'V') && val_str)
		SHA1_Update(&sha1ctx, val_str, strlen(val_str));
	    }
	SHA1_Final(hash, &sha1ctx);
	SHA1_Init(&sha1ctx);
	for(round=0; round<2; round++)
	    {
	    if (strchr(which, 'K') && key)
		SHA1_Update(&sha1ctx, key, strlen(key));
	    if (strchr(which, 'A') && attrname)
		SHA1_Update(&sha1ctx, attrname, strlen(attrname));
	    /*if (strchr(which, 'N') && objname)
		SHA1_Update(&sha1ctx, objname, strlen(objname));*/
	    if (strchr(which, 'T') && type_name)
		SHA1_Update(&sha1ctx, type_name, strlen(type_name));
	    }
	SHA1_Final(hash_novalue, &sha1ctx);

	/** Now, do the hard work - based on data type. **/
	switch(data_type)
	    {
	    case DATA_T_INTEGER:
		iv = srcval->Integer;
		if (strchr(param,'i'))
		    dstval->Integer = obf_internal_ObfuscateIntegerMultiples(hash, hash_novalue, &bitcnt, iv);
		else
		    {
		    /*if (obf_internal_GetBits(hash, &bitcnt, 1)) iv = (iv + 1)*10;
		    if (obf_internal_GetBits(hash, &bitcnt, 1)) iv = iv/10 + 1;*/
		    dstval->Integer = obf_internal_ObfuscateInteger(hash, hash_novalue, &bitcnt, iv);
		    }
		break;

	    case DATA_T_MONEY:
		//iv = srcval->Money->WholePart * 100 + (srcval->Money->FractionPart / 100);
		iv = srcval->Money->Value/10000 * 100 + (srcval->Money->Value%10000 / 100);
		if (strchr(param,'i'))
		    dv = obf_internal_ObfuscateIntegerMultiples(hash, hash_novalue, &bitcnt, iv);
		else
		    {
		    if (obf_internal_GetBits(hash, &bitcnt, 1)) iv = (iv + 1)*10;
		    if (obf_internal_GetBits(hash, &bitcnt, 1)) iv = iv/10 + 1;
		    dv = obf_internal_ObfuscateInteger(hash, hash_novalue, &bitcnt, iv);
		    }
		//m.WholePart = floor(dv/100.0);
		//m.FractionPart = (dv - m.WholePart*100) * 100;
		m.Value = dv * 100;
		dstval->Money = &m;
		break;

	    case DATA_T_DOUBLE:
		scale = floor(log10(srcval->Double));
		iv = (srcval->Double * exp10(8 - scale));
		if (strchr(param,'i'))
		    dv = obf_internal_ObfuscateIntegerMultiples(hash, hash_novalue, &bitcnt, iv);
		else
		    dv = obf_internal_ObfuscateInteger(hash, hash_novalue, &bitcnt, iv);
		dstval->Double = dv * exp10(scale - 8);
		break;

	    case DATA_T_DATETIME:
		memcpy(&dt, srcval->DateTime, sizeof(DateTime));
		if (strchr(param,'h'))
		    {
		    iv = obf_internal_GetBits(hash, &bitcnt, 5);
		    if (iv >= 24) iv -= 16;
		    dt.Part.Hour = (dt.Part.Hour + iv)/2;
		    }
		if (strchr(param,'m'))
		    {
		    iv = obf_internal_GetBits(hash, &bitcnt, 6);
		    if (iv >= 60) iv = 0;
		    dt.Part.Minute = (dt.Part.Minute + iv)/2;
		    }
		if (strchr(param,'s'))
		    {
		    iv = obf_internal_GetBits(hash, &bitcnt, 6);
		    if (iv >= 60) iv = 0;
		    dt.Part.Second = (dt.Part.Second + iv)/2;
		    }
		dstval->DateTime = &dt;
		break;

	    case DATA_T_STRING:
		xsInit(&xs);
		obf_internal_ObfuscateString(srcval->String, &xs, hash, hash_novalue, &bitcnt, wordlist, catlist);
		if (str) nmSysFree(str);
		str = nmSysStrdup(xs.String);
		xsDeInit(&xs);
		dstval->String = str;
		break;

	    default:
		mssError(1,"OBF","Unsupported data type for obfuscation");
		break;
	    }

    return 0;
    }


/*** obf_internal_FindRuleMatch() - find a matching rule in the session's rule
 *** list.
 ***/
pObfRule
obf_internal_FindRuleMatch(pObfSession sess, pObjData srcval, int data_type, char* attrname, char* objname, char* type_name)
    {
    pObfRule rule;
    char* data_type_str;
    char* val_str;

	/** Get name of data type we're using **/
	if (data_type <= 0 || data_type >= OBJ_TYPE_NAMES_CNT)
	    data_type_str = NULL;
	else
	    data_type_str = obj_type_names[data_type];

	/** Search for a matching rule **/
	for(rule=sess->RuleList; rule; rule=rule->Next)
	    {
	    /** Check the match conditions **/
	    if (rule->Attrname && (!attrname || strcmp(rule->Attrname,attrname) != 0)) continue;
	    if (rule->Objname && (!objname || strcmp(rule->Objname,objname) != 0)) continue;
	    if (rule->Datatype && (!data_type_str || strcmp(rule->Datatype,data_type_str) != 0)) continue;
	    if (rule->Typename && (!type_name || strcmp(rule->Typename,type_name) != 0)) continue;

	    /** If value match, check it - requires converting srcval to string **/
	    if (rule->ValueString)
		{
		if (data_type == DATA_T_MONEY || data_type == DATA_T_DATETIME || data_type == DATA_T_STRING)
		    val_str = objDataToStringTmp(data_type, srcval->Generic, 0);
		else
		    val_str = objDataToStringTmp(data_type, srcval, 0);
		if (!val_str || strcmp(rule->ValueString, val_str) != 0) continue;
		}

	    /** Got a match - we're done **/
	    return rule;
	    }

    return NULL;
    }


/*** obfObfuscateDataSess() - looks up any appropriate rule in the rule
 *** file (if supplied), and calls obfObfuscateData accordingly.
 ***/
int
obfObfuscateDataSess(pObfSession sess, pObjData srcval, pObjData dstval, int data_type, char* attrname, char* objname, char* type_name)
    {
    pObfRule rule;
    char which[8];
    int rval;

	/** Find a rule **/
	rule = obf_internal_FindRuleMatch(sess, srcval, data_type, attrname, objname, type_name);

	/** No obfuscation? **/
	if (rule && rule->NoObfuscate)
	    {
	    switch(data_type)
		{
		case DATA_T_INTEGER:
		    dstval->Integer = srcval->Integer;
		    break;
		case DATA_T_STRING:
		case DATA_T_DATETIME:
		case DATA_T_MONEY:
		    dstval->Generic = srcval->Generic;
		    break;
		case DATA_T_DOUBLE:
		    dstval->Double = srcval->Double;
		    break;
		}
	    return 0;
	    }

	/** Attribute equivalency? **/
	if (rule && rule->EquivAttr && rule->Param)
	    {
	    attrname = rule->Param;
	    rule = obf_internal_FindRuleMatch(sess, srcval, data_type, attrname, objname, type_name);
	    }

	/** Call the main obfuscation function **/
	if (rule)
	    {
	    which[0] = '\0';
	    if (rule->HashKey) strcat(which,"K");
	    if (rule->HashAttr) strcat(which,"A");
	    if (rule->HashName) strcat(which,"N");
	    if (rule->HashType) strcat(which,"T");
	    if (rule->HashValue) strcat(which,"V");
	    rval = obfObfuscateData(srcval, dstval, data_type, attrname, objname, type_name, sess->Key, which, rule->Param, rule->Wordlist, rule->Catlist);
	    }
	else
	    {
	    strtcpy(which, "KANTV", sizeof(which));
	    rval = obfObfuscateData(srcval, dstval, data_type, attrname, objname, type_name, sess->Key, which, NULL, NULL, NULL);
	    }

    return rval;
    }


