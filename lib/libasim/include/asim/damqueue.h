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

//
// Initial Author: Shubu Mukherjee
// Adapted by: Federico Silla
//



#ifndef _DAMQUEUE_H
#define _DAMQUEUE_H

// ASIM core
#include "asim/syntax.h"

template <class T> 
class DAMQueue
{
  public:
    DAMQueue<T>()
        :  Head(0), Size(0), SearchPtr(0), Dummy(0)
    {}

    void   Init(UINT16 n); 
    bool   Exists(void); 
    bool   IsEmpty(void);
    bool   IsFull(void); 
    void   Store(T t); 
    T      GetEntry(UINT16 n); 
    void   DelEntry(UINT16 n); 
    UINT16 GetOccupancy(void);
    UINT16 GetCapacity(void);
    UINT16 GetFreeSpace(void);
    INT16  GetNextEntryNum(void);
    T      GetNextEntry(void);
    INT16  GetCurrentEntryNum(void);
    void   ResetSearchPtr(void);



  protected: 
    T	    *Data; 
    UINT16  Head; 
    UINT16  Size; 
    UINT16  SearchPtr; 
    UINT16  Dummy; 
    UINT16  MaxSize; 
};



template <class T> 
void DAMQueue<T>::Init(UINT16 n)
{
    if (n != 0)
       {// allocate one extra buffer for queue management
	MaxSize = n; 
	Dummy = n+1;
	Data = new T[Dummy];
        for (UINT16 i=0; i<Dummy; i++)
            Data[i] = NULL;
       }
    else
       {MaxSize = 0;
	Dummy = 0;
	Data = NULL; 
       }
}



template <class T>
inline
bool DAMQueue<T>::Exists(void)
{
 return Dummy > 1; // one extra buffer added on
}



template <class T>
inline
bool DAMQueue<T>::IsFull(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n"); 
    return (Size == MaxSize); 
}



template <class T>
inline
bool DAMQueue<T>::IsEmpty(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    return (!Size); 
}



template <class T>
inline
UINT16 DAMQueue<T>::GetOccupancy(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    return (Size); 
}




template <class T>
inline
UINT16 DAMQueue<T>::GetCapacity(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    return (MaxSize); 
}




template <class T>
inline
UINT16 DAMQueue<T>::GetFreeSpace(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    return (MaxSize - Size); 
}





template <class T>
INT16 DAMQueue<T>::GetNextEntryNum(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    UINT16 mark = (Head + MaxSize) % Dummy; // that is, (Head - 1) % Dummy
    SearchPtr = (SearchPtr + 1) % Dummy;
    while ((SearchPtr != mark) && (Data[SearchPtr] == NULL))
      SearchPtr = (SearchPtr + 1) % Dummy;
    if (SearchPtr == mark)
      return -1;
    else
      return SearchPtr;
}


template <class T>
T DAMQueue<T>::GetNextEntry(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    UINT16 mark = (Head + MaxSize) % Dummy; // that is, (Head - 1) % Dummy
    SearchPtr = (SearchPtr + 1) % Dummy;
    while ((SearchPtr != mark) && (Data[SearchPtr] == NULL))
      SearchPtr = (SearchPtr + 1) % Dummy;
    if (SearchPtr == mark)
      return NULL;
    else
      return Data[SearchPtr];
}



template <class T>
void DAMQueue<T>::Store(T t)
{
    VERIFY(!IsFull(), "DAMQueue is full\n");
    if (IsEmpty())
      {Data[Head] = t;
       Size++;
       return;
      }
    else
      {UINT16 i = (Head + 1) % Dummy;
      while (i != Head)
          {if (Data[i] == NULL)
             {VERIFYX(i != ((Head + MaxSize) % Dummy));  // (Head-1)%Dummy should always be NULL
              Data[i] = t;
              Size++;
              return;
             }
           i = (i + 1) % Dummy;
          }
      }
    ASIMERROR("DAMQueue<T>::Store() could not find a free entry\n"); 
}




template <class T>
void DAMQueue<T>::DelEntry(UINT16 n)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    VERIFY(Data[n], "Entry does not exists\n"); 
    
    Data[n] = NULL; //this should be compatible with Smart Pointers
    Size--; 
    UINT16 mark = (Head + MaxSize) % Dummy; // that is, (Head - 1) % Dummy
    while ((Data[Head] == NULL) && (Head != mark))
      Head = (Head + 1) % Dummy;
}



template <class T>
inline
T DAMQueue<T>::GetEntry(UINT16 n)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n"); 
    VERIFY(n < Dummy, "Index exceeds DAMQueue limits\n"); 
    VERIFY(Data[n], "Entry 'n' is empty\n"); 
    return Data[n]; 
}



template <class T>
inline
void DAMQueue<T>::ResetSearchPtr(void)
{
    VERIFY(Exists(), "DAMQueue is not initialized\n");
    SearchPtr = Head;
}



template <class T>
inline
INT16 DAMQueue<T>::GetCurrentEntryNum(void)
{
    if (!IsEmpty())
	return SearchPtr; 
    else
	return -1; 
}


#endif //_DAMQUEUE_H

