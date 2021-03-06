/*
 * Copyright (c) 2014, Intel Corporation
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of the Intel Corporation nor the names of its 
 *   contributors may be used to endorse or promote products derived from 
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/////////////////////////////////////////////////////////////////////////////////
//
// $Log$
// Revision 1.2  2005/05/20 03:35:30  mcadler
// CSN-core-1256
//
// Revision 1.1  2004/11/03 18:32:13  snpeffer
// CSN-core-1063
//
//
// regular expression class implementation
//
/////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <iostream>
#include <stdlib.h>

#include "asim/regexobj.h"

Regex::Regex(const char *pattern, bool caseSensitive)
{
    subArraySize = 0;
    subArray = NULL;

    initialized = false;
    
    if (!pattern) {
	return;
    }
    int status = create(pattern, caseSensitive);
    if (status != 0) {
	error(status);
	dispose();
    }
    else {
	initialized = true;
    }
}

Regex::Regex()
{
    subArraySize = 0;
    subArray = NULL;

    initialized = false;
}

int
Regex::create(const char *pattern, bool caseSensitive)
{
    int flags = REG_EXTENDED;
    if (! caseSensitive)
    {
        flags |= REG_ICASE;
    }
    int status = regcomp(&regexPattern, pattern, flags);
    return status;
}

bool
Regex::assign(const char *pattern, bool caseSensitive)
{
    if (initialized) {
	dispose();
	initialized = false;
    }
    
    if (!pattern) {
	return initialized;
    }
    else {
	int status = create(pattern, caseSensitive);
	if (status != 0) {
	    error(status);
	    dispose();
	}
	else {
	    initialized = true;
	}
	return initialized;
    }
}

string
Regex::getMatchError() {
    string error;
    char buf[1024];
    if (initialized) {
        int len;
        len = regerror(lastStatus, &regexPattern, buf, 1024);
        if(len > 0) {
            error = buf;
        }
    }
    return(error);
}

bool
Regex::_match(const char *str)
{
    if (initialized) {
        lastMatch = str;
	lastStatus = regexec(&regexPattern, str, 0, NULL, 0);
	if (lastStatus) {
	    return false;
	}
	else {
	    return true;
	}
    }
    return(false);
}

bool
Regex::match(const char *str)
{
    return _match(str);
}

bool
Regex::match(const string& str)
{
    return _match(str.c_str());
}

bool
Regex::_matchSub(const char *str)
{
    static const int growthRate = 3;

    if (initialized) {
        // can't start with a size of zero or substring matching is
        // turned off
        if(subArraySize == 0) {
            subArraySize = growthRate;
            subArray = (regmatch_t *)malloc(sizeof(regmatch_t) * subArraySize);
        }

        lastNumSubsMatched = 0;
        lastMatch = str;
	lastStatus = regexec(&regexPattern, str, subArraySize, subArray, 0);
	if (lastStatus) {
	    return false;
	}
	else {
            // Make sure we captured all of the sub expressions.
            bool foundNegOne = false;
            for(unsigned i = 0; i < subArraySize; i++) {
                if(subArray[i].rm_so == -1) {
                    foundNegOne = true;
                    lastNumSubsMatched = i;
                }
            }
            // If not, a bigger array and try again.
            if(!foundNegOne) {
                if(subArray) {
                    free(subArray);
                }
                subArraySize += growthRate;
                subArray = (regmatch_t *)malloc(sizeof(regmatch_t) * subArraySize);
                return(matchSub(str));
            }
	    return true;
	}
    }
    return(false);
}

bool
Regex::matchSub(const char *str)
{
    return _matchSub(str);
}

bool
Regex::matchSub(const string& str)
{
    return _matchSub(str.c_str());
}

bool 
Regex::isValidIndex(int index) {
    return(index <= (int)lastNumSubsMatched);
}

string
Regex::getSubstring(int index) {
    if(!isValidIndex(index)) {
        return("");
    }
    // extract the matching string - preallocate a buffer for better performance
    string match(0, subArray[index].rm_eo - subArray[index].rm_so + 1);
    for(int i = subArray[index].rm_so; i < subArray[index].rm_eo; i++) {
        match.append(1, lastMatch[i]);
    }
    return(match);
}

void
Regex::error(int status)
{
    char errorBuf[REGEX_ERRORBUF_SIZE];
    regerror(status, &regexPattern, errorBuf, REGEX_ERRORBUF_SIZE);
    cerr << "REGEX ERROR: " << errorBuf << endl;
}

void
Regex::dispose()
{
    if (initialized) {
	regfree(&regexPattern);
	initialized = false;
        subArraySize = 0;
        if(subArray) {
            free(subArray);
            subArray = NULL;
        }
    }
}

bool
Regex::ok()
{
    return initialized;
}

bool 
RegexSubstitution::hasSubstitution() {
    const char *str = substring.c_str();
    while(*str != 0) {
        // skip escaped backslashes
        if(*str == '\\' && *(str + 1) == '\\') {
            str++;
        }
        else if(getSubstitution(str, 0) != -1) {
            return(true);
        }
        str++;
    }
    return(false);
}

bool RegexSubstitution::substitute(Regex &re) {
    substitutedString = "";
    const char *str = substring.c_str();
    char *endptr;
    int index;
    while(*str != 0) {
        // skip escaped backslashes
        if(*str == '\\' && *(str + 1) == '\\') {
            // insert a single backslash
            substitutedString.append(1, '\\');
            str += 2;
        }
        else if((index = getSubstitution(str, &endptr)) != -1) {
            if(!re.isValidIndex(index)) {
                return(false);
            }
            // insert the substring
            substitutedString.append(re.getSubstring(index));
            str = endptr;
        } 
        else {
            substitutedString.append(1, *str);
            str++;
        }
    }
    return(true);
}

int 
RegexSubstitution::getSubstitution(const char *str, char **endptr) {
    if(*str != '\\') {
        return(-1);
    }
    str++;
    if(!isdigit(*str)) {
        return(-1);
    }
    return(strtol(str, endptr, 10));
}

/////////////////////////////////////////////////////////////////////////////////
