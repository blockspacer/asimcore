/**************************************************************************
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


#ifndef __dralServerImplementation_h
#define __dralServerImplementation_h

#include "asim/dralWrite.h"

/*
 * Interface of the dral server implementation
 */
class DRAL_SERVER_IMPLEMENTATION_CLASS
{
  public:

    DRAL_SERVER_IMPLEMENTATION_CLASS(UINT16 buffer_size, bool compression);
    
    void SetFileDescriptor (int fd);

    virtual ~DRAL_SERVER_IMPLEMENTATION_CLASS(void);

    virtual void NewNode (
        UINT16 node_id, const char name[], UINT16 name_len,
        UINT16 parent_id, UINT16 instance)=0;

    virtual void NewEdge (
        UINT16 edge_id, UINT16 source_node, UINT16 destination_node,
        UINT32 bandwidth, UINT32 latency, const char name[], UINT16 name_len)=0;

    virtual void SetNodeLayout(
        UINT16 node_id,  UINT16 dimensions, const UINT32 capacity [])=0;

    virtual void SetNodeTag(
        UINT16 node_id, const char tag_name [], UINT16 tag_name_len,
        UINT64 value, UINT16 level, const UINT32 lst [])=0;

    virtual void SetNodeTag(
        UINT16 node_id, const char tag_name [], UINT16 tag_name_len, UINT16 n,
        const UINT64 set [], UINT16 level, const UINT32 lst [])=0;

    virtual void SetNodeTag(
        UINT16 node_id, const char tag_name [], UINT16 tag_name_len,
        const char str [], UINT16 str_len, UINT16 level, const UINT32 lst [])=0;

    virtual void Cycle (UINT64 n)=0;

    virtual void NewItem (UINT32 item_id)=0;

    virtual void SetItemTag (
        UINT32 item_id, const char tag_name[], UINT16 tag_name_len,
        UINT64 value)=0;

    virtual void SetItemTag (
        UINT32 item_id, const char tag_name[], UINT16 tag_name_len,
        const char str[], UINT16 str_len)=0;

    virtual void SetItemTag (
        UINT32 item_id, const char tag_name[], UINT16 tag_name_len,
        UINT32 nval, UINT64 value[])=0;

    virtual void MoveItems (
        UINT16 edge_id, UINT32 n, UINT32 item_id[], UINT32 position [])=0;

    virtual void EnterNode (
        UINT16 node_id, UINT32 item_id, UINT16 dimensions, UINT32 position[])=0;

    virtual void ExitNode (
        UINT16 node_id, UINT32 item_id, UINT16 dimensions, UINT32 position[])=0;

    virtual void DeleteItem (UINT32 item_id)=0;

    virtual void Comment (
        UINT32 magic_num, const char comment [], UINT32 length)=0;

    virtual void CommentBin (
        UINT16 magic_num, const char comment [], UINT32 comment_len)=0;

    virtual void SetCycleTag (
        const char tag_name[], UINT16 tag_name_len, UINT64 value)=0;

    virtual void SetCycleTag (
        const char tag_name[], UINT16 tag_name_len,
        const char str[], UINT16 str_len)=0;

    virtual void SetCycleTag (
        const char tag_name[], UINT16 tag_name_len,
        UINT32 nval, UINT64 value[])=0;

    virtual void SetNodeInputBandwidth (UINT16 nodeId, UINT32 bandwidth)=0;

    virtual void SetNodeOutputBandwidth (UINT16 nodeId, UINT32 bandwidth)=0;

    virtual void StartActivity (UINT64 firstActivityCycle)=0;

    virtual void SetTagDescription (
        const char tag [], UINT16 tag_len,
        const char description [], UINT16 desc_len)=0;

    virtual void SetNodeClock (UINT16 nodeId, UINT16 clockId)=0;
    
    virtual void NewClock (
        UINT16 clockId, UINT64 freq, UINT16 skew, UINT16 divisions,
        const char name [], UINT16 nameLen)=0;

    virtual void Cycle (UINT16 clockId, UINT64 n, UINT16 phase)=0;

    /*
     * Public method used to send the version
     * The version will be sent to the file descriptor when the implementation
     * class is created.
     */
    virtual void Version (void)=0;
    
    /*
     * Public method used to flush the write buffer.
     * The buffer will be flushed when turning off the server and when
     * destroying the implementation class.
     */
    void Flush (void);

  protected:

    /*
     * A pointer to the class used to perform buffered write
     */
    DRAL_BUFFERED_WRITE dralWrite;

};
typedef DRAL_SERVER_IMPLEMENTATION_CLASS * DRAL_SERVER_IMPLEMENTATION;


#endif /* __dralServerImplementation_h */
