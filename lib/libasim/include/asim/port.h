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

/**
 * @file
 * @author Nathan Binkert, Eric Borch
 * @brief ASIM port abstraction - access points to wires.
 */

#ifndef _PORT_
#define _PORT_

// generic
#include <stdio.h>
//#include <string.h>
#include <typeinfo>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/vector.h"
#include "asim/trace.h"
#include "asim/item.h"
#include "asim/event.h"
#include "asim/atomic.h"
#include "asim/phase.h"
#include "asim/module.h"

extern bool registerPortStats;


template<class T, int S>
class BufferStorage;

/////////////////////////////////////////////////////////////////////////
// class BasePort
//
// This class contains code that is shared between the two buffer
// endpoints.  Certain information such as scope, name, connection
// state, etc are shared among all buffer endpoints.
//
class BasePort : public TRACEABLE_CLASS
{
public:
  enum PortType
  { BaseType,
    WriteType,
    WritePhaseType,
    ReadType,
    ReadPhaseType,
    ConfigType,
    PeekType,        
    LastType }; // This should always be the last one!!!

  static const char *PortName[];
  static int id_count;
  int my_id;

protected:
  static asim::Vector<BasePort*> AllPorts;

private:
  virtual void *GetBuffer();
  virtual void SetBuffer(void *buf, int rdPortNum);
  virtual void SetBufferInfo();

  // Needed to notify the connection structure via event.
  virtual void EventConnect(int bufNum, int destination);
  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();

protected:
  // this is used to ensure the endpoints of a connected port have the same type,
  // and is implemented in derived classes:
  virtual const type_info &GetDataType() = 0;

  // These three variable define how to identify an endpoint and how
  // its connection to the other endpoint should be handled.
  char *Scope;
  char *Name;
  int Instance;

  // Indicates whether this buffer has been attached to another yet.
  bool Connected;

  // The bandwidth and latency of the buffer.  This information is
  // stored at each endpoint.
  int Bandwidth;
  int Latency;

  // Fanout for WritePorts only!  Needed here because when sorting and
  // connecting up BasePorts, we need to access Fanout to determine
  // what to connect to what
  int Fanout;

  // For automatic event generation purposes, the node that uses this port must be known 
  int node;
  
  // List of the ports connected to this one
  list<BasePort*> connectedPorts;
  list<BasePort*> getConnectedPorts() { return connectedPorts; }

public:
  // Accessors for bandwidth and latency.
  int GetBandwidth() const;
  int GetLatency() const;
  int GetFanout() const;
  int GetUid() const; 
  
public:
  // Initialization of variables.
  BasePort();

  // Destructor must be virtual to make sure that we destroy objects
  // that the base class may have created such as Name and Scope.
  virtual ~BasePort();

  const char *GetName() const;
  const char *GetScope() const;
  int GetInstance() const;
  bool IsConnected() const;

  // Access functions to set the bandwidth and latency of the buffer.
  // These operations may each only occur once.
  bool SetBandwidth(int bw);
  bool SetLatency(int lat);
  void SetFanout(int fo);


  bool Init(const char *name, int nodeId = 0, int instance = 0, const char *scope = NULL);
  bool Init(ASIM_CLOCKABLE m, const char *name, int nodeId = 0, int instance = 0, const char *scope = NULL);
  bool Config(int bw, int lat);
  bool InitConfig(const char *name, int bw, int lat, int nodeId = 0 );
  bool InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId = 0 );

public:
  static void ConnectAll();
  static bool ConnectPorts(int port, int writePort, int index, 
		      asim::Vector<BasePort*>::Iterator i);

  virtual PortType GetType() const = 0;
  const char *GetTypeName() const;
};


///////////////////////////////////////////////////////////////////
// class BufferStorage<T,S>
//
template<class T, int S = 0>
class BufferStorage
{
private:
  // This looks unnecessary.  And GCC won't compile it.  GCC stinks!
  //  friend void BasePort::ConnectAll();
  
protected:
  // For events purposes, every single Port needs to have an unique identifier.
  UINT16 myEventEdgeId;
  
//  static const int DefaultBandwidth = W;
  // if S == 0, assume programmer does not want to worry about sizing
  // buffer, so double the size to cover for round trip latency.  If S
  // > 0, assume programmer wants to set the size for optization or a
  // much longery return latency
//  static const int DefaultLatency = (S == 0 ? CEIL_POW2(L + 1) : CEIL_POW2(L + S));

  struct CycleEntry
  {
    // cycle written
    INT64 CycleWritten;

      ATOMIC_INT32 Start;
      ATOMIC_INT32 End;
    T *Data;
  } *Store;
  const T Dummy;

  bool Enabled;

  int Bandwidth;
  int Latency;

  int BufferSize;
  int BufferMask;

  bool active;
  bool stalled;

  int ReadIndex;
  int WriteIndex;

  INT64 CycleRowRead;

  int PeekStart;
  int PeekReadIndex;

 

private:
  // Copying is not allowed.
  BufferStorage(const BufferStorage& s);
  const BufferStorage& operator=(const BufferStorage& s);

  // This functions are used to automatically generate events when an ASIM_ITEM
  // moves through a port
  void Notify(const ASIM_ITEM& data);
  void Notify(const ASIM_ITEM_CLASS& data);
  void Notify(const ASIM_SILENT_ITEM& data);
  void Notify(const ASIM_SILENT_ITEM_CLASS& data);
  // this one catches pointers or smart pointers to classes that inherit from neither ASIM_ITEM_CLASS
  // nor ASIM_SILENT_ITEM_CLASS, and handles them silently.
  void Notify(void *data);
  // this catches everyting else, but beware of passing non-POD (plain-old-data) types over ports,
  // since varargs doesn't handle them.  You will get a compiler warning but the code will crash
  // with an illegal instruction.  Solution: have your class inherit from ASIM_SILENT_ITEM_CLASS.
  void Notify(...);
  void Notify(const T& data);

  int Test(ASIM_ITEM);
  int Test(ASIM_ITEM*);
  int Test(ASIM_ITEM_CLASS);
  char Test(...);
  T* MakeT();

  UINT64 LastAccessed;
  UINT64 LastWritten;  //the most recent write cycle
  UINT32 SequentialWrites;   // the number of writes without a read
                             // (used for assertion checking)

public:
  BufferStorage();
  ~BufferStorage();

  bool IsFull(int i) const;
  bool IsEmpty(int i) const;
  bool CreateStorage(UINT32 latency, UINT32 bandwidth) ;
  bool DeleteStorage() ;

  INT16 GetEventEdgeId() const { return myEventEdgeId; }
  void SetEventEdgeId(const UINT16 id) { myEventEdgeId = id; }

  bool SomethingToRead(UINT64 cycle) const;

  int GetBandwidth() const;
  int GetLatency() const;
  int GetBufferSize() const;
  UINT64 GetLastAccessed() const{ return LastAccessed; }
  UINT64 GetLastWritten() const{ return LastWritten; }
  void SetLastAccessed(UINT64 c) { LastAccessed = c; }
  int GetWriteIndex() const { return WriteIndex; }

  bool IsEnabled() const;
  bool SetEnable(int bw, int lat, const char* portName);

  bool Read(T& data, UINT64 cycle, const char* portName, bool relaxAsserts = false);

  bool Write(const T& data, UINT64 cycle, const char* portName);
    
  bool Look(T& data, UINT64 cycle, const char* portName, bool relaxAsserts);

  void Delay(INT64 delay, const char* portName, bool relaxAsserts);

  bool IsStalled() const;
  void SetStalled(bool s);

  void SetActive(bool a);

  bool PeekNext(T& data, UINT64 cycle);
  void PeekReset();

  INT64 LatestWrite();
  bool IsActive(){ return active; } 

  void Clear(); // empty out all data from the buffer
};

template <class T, int S = 0>
class Storage : public BufferStorage<T,S>
{};

///////////////////////////////////////////////////////////////////
// class WriteRemotePort<T, F> : public WritePort
/*
template<class T, int F = 1>
class WriteRemotePort : protected WritePort
{
    public:
    bool WriteRemote(T data, UINT64 cycle);
    INT64 LatestWrite();
    void SetLastAccessed(UINT64);
    void Deactivate();
}
*/
///////////////////////////////////////////////////////////////////
// class WritePort<T, F> : public BasePort
//
template<class T, int F = 1>
class WritePort : public BasePort
{
  // Buffer is protected for access from derived PowerWritePort class
  //private:
  protected:
  Storage<T> *Buffer[F];

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void SetBuffer(void *buf, int rdPortNum);

protected:
  int Fanout;

public:
  WritePort();
  
  bool Write(T data, UINT64 cycle);

  virtual PortType GetType() const;

  virtual void EventConnect(int bufNum, int destination);

  int GetLatency() const;

  INT16 GetEventEdgeId();


  bool WriteRemote(T data, UINT64 cycle);
  INT64 LatestWrite();
  void SetLastAccessed(UINT64);
  void Deactivate();

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class WriteSkidPort<T, S> : public BasePort
//
template<class T, int S = 0>
class WriteSkidPort : public BasePort
{
  // Buffer is protected for access from PowerWriteSkidPort class
  //private:
  protected:
// The sizing of Skid ports currently does not work!  The value of S is not
// used! Eric
  Storage<T,S> *Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void SetBuffer(void *buf, int rdPortNum);

public:
  WriteSkidPort();
  
  bool Write(T data, UINT64 cycle);

  virtual void EventConnect(int bufNum, int destination);

  virtual PortType GetType() const;

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class WriteStallPort<T> : public BasePort
//
template<class T>
class WriteStallPort : public BasePort
{
  // Buffer is protected for access from PowerWriteStallPort class
  //private:
 protected:
  Storage<T> *Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void SetBuffer(void *buf, int rdPortNum);

public:
  WriteStallPort();
  
  bool Write(T data, UINT64 cycle);

  bool IsStalled();

  virtual void EventConnect(int bufNum, int destination);

  virtual PortType GetType() const;

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};



///////////////////////////////////////////////////////////////////
// class ReadPort<T> : public BasePort
//
// ReadPort must read the buffer endpoint EVERY cycle
template<class T>
class ReadPort : public BasePort
{
  // Buffer is protected for access from PowerReadPort class
  //private:
 protected:
  Storage<T> Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void *GetBuffer();
  void SetBufferInfo();
  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();
    
public:
  virtual ~ReadPort() { DeleteStorage(); };

  bool Read(T& data, UINT64 cycle);
  
  virtual PortType GetType() const;

  INT16 GetEventEdgeId();
  
  bool ReadRemote(T& data, UINT64 cycle);

  void Deactivate();

  UINT64 GetLastAccessed();
  bool IsActive();

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};


///////////////////////////////////////////////////////////////////
// class ReadSkidPort<T, S> : public BasePort
//
// ReadBuffer doesn't have to read the buffer endpoint every cycle.
// It can treat the buffer like a skid buffer
template<class T, int S = 0>
class ReadSkidPort : public BasePort
{
  // Buffer is protected for access from PowerReadSkidPort
  //private:
  protected:
  Storage<T,S> Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void *GetBuffer();
  void SetBufferInfo();
  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();

public:
  virtual ~ReadSkidPort() { DeleteStorage(); };

  bool Read(T& data, UINT64 cycle);

  // This is the ONLY port type that needs SomethingToRead.  It's used
  // sometimes to check to see if there's anything to read, and then
  // it decides if it wants to read it.
  bool SomethingToRead(UINT64 cycle) const;

  virtual PortType GetType() const;

  INT16 GetEventEdgeId();

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class ReadStallPort<T, S> : public BasePort
//
// ReadBuffer doesn't have to read the buffer endpoint every cycle.
// It can treat the buffer like a skid buffer
template<class T>
class ReadStallPort : public BasePort
{
  // Buffer is protected for access from PowerReadStallPort
  //private:
 protected:
  Storage<T> Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void *GetBuffer();
  void SetBufferInfo();
  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();

public:
  virtual ~ReadStallPort() { DeleteStorage(); };

  bool Read(T& data, UINT64 cycle);

  // like read, but no side effects or pops
  bool Look(T& data, UINT64 cycle);

  virtual PortType GetType() const;

  // this sets the stall and delays the port contents by one cycle
  void Stall(bool s);

  // just manipulate the stall variable
  void SetStalled(bool s);
  bool IsStalled();

  INT16 GetEventEdgeId();

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class PeekPort<T> : public BasePort
//
template<class T>
class PeekPort : public BasePort
{
private:
  Storage<T> *Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void *GetBuffer();
  virtual void SetBuffer(void *buf, int rdPortNum);
  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();

public:
  virtual ~PeekPort() { DeleteStorage(); };

  void PeekReset();
  bool PeekNext(T& data, UINT64 cycle);

  virtual PortType GetType() const;

  INT16 GetEventEdgeId();

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class WritePhasePort<T, F> : public BasePort
//
template<class T, int F = 1>
class WritePhasePort : public BasePort
{
  // Buffer is protected for access from PowerWritePhasePort
  //private:
  protected:
  Storage<T> *Buffer[F];

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void SetBuffer(void *buf, int rdPortNum);

protected:
  int Fanout;

public:
  WritePhasePort();
  
  bool Write(T data, UINT64 cycle);
  bool Write(T data, PHASE ph);
  bool Write(T data, UINT64 cycle, CLK_EDGE ed);

  virtual PortType GetType() const;

  virtual void EventConnect(int bufNum, int destination);

  int GetLatency() const;

  INT16 GetEventEdgeId();

  bool SetLatency(int lat);
  bool SetLatencyPhases(int lat);
  bool Config(int bw, int lat);
  bool InitConfig(const char *name, int bw, int lat, int nodeId = 0 );  
  bool InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId = 0 );  

  void Clear(); // empty out all data from the port buffer
  

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};

///////////////////////////////////////////////////////////////////
// class ReadPhasePort<T> : public BasePort
//
// ReadPhasePort must read the buffer endpoint EVERY phase
template<class T>
class ReadPhasePort : public BasePort
{
  // Buffer is protected for access from PowerReadPhasePort

  //private:
 protected:
  Storage<T> Buffer;

  // This is WAY dangerous if not done properly.  C programmers feel
  // happy passing void and doing a cast, but C++ programmers cringe.
  // This is done this way because we lack a virtual base class shared 
  // among buffers.  Maybe that should be done.
  virtual void *GetBuffer();
  void SetBufferInfo();

  virtual bool CreateStorage(UINT32 latency, UINT32 bandwidth);
  virtual bool DeleteStorage();

public:
  virtual ~ReadPhasePort() { DeleteStorage(); };

  bool Read(T& data, UINT64 cycle);
  bool Read(T& data, PHASE ph);
  bool Read(T& data, UINT64 cycle, CLK_EDGE ed);
  
  virtual PortType GetType() const;

  INT16 GetEventEdgeId();

  bool SetLatency(int lat);
  bool SetLatencyPhases(int lat);
  bool Config(int bw, int lat);
  bool InitConfig(const char *name, int bw, int lat, int nodeId = 0 );  
  bool InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId = 0 );  

  void Clear(); // empty out all data from the port buffer

protected:
  // this is used to ensure the endpoints of a connected port have the same type:
  virtual const type_info &GetDataType() { return typeid( T ); };
};


///////////////////////////////////////////////////////////////////
// class ConfigPort : public BasePort
//
class ConfigPort : public BasePort
{
public:
  virtual PortType GetType() const;

protected:
  // not really used, but we have to implement it.
  // We'll return the type id of ourself:
  virtual const type_info &GetDataType() { return typeid( ConfigPort ); };
};


//////////////////////
// class BasePort
//
inline
BasePort::BasePort()
  : Scope(NULL), Name(NULL), Instance(0), Connected(false),
    Bandwidth(-1), Latency(-1)
{ 
   AllPorts.Insert(AllPorts.End(), this); 
   my_id = id_count;
   id_count ++;
   SetTraceableName("PORTS");
   T1("declared port with id " << my_id);
}

inline
BasePort::~BasePort()
{
    // remove this port from the global list
    asim::Vector<BasePort*>::Iterator i = AllPorts.Begin();
    while (i != AllPorts.End() && *i != this) i++;
    VERIFYX(i != AllPorts.End());
    AllPorts.Remove(i);

    delete [] Scope;
    delete [] Name;
}

inline int
BasePort::GetUid() const
{
   return my_id;
}

inline const char *
BasePort::GetName() const
{ return Name; }

inline const char *
BasePort::GetScope() const
{ return Scope; }

inline int
BasePort::GetInstance() const
{ return Instance; }

inline bool
BasePort::IsConnected() const
{ return Connected; }

inline int
BasePort::GetBandwidth() const
{ return Bandwidth; }

inline int
BasePort::GetLatency() const
{ return Latency; }

inline int
BasePort::GetFanout() const
{ return Fanout; }

inline bool
BasePort::SetBandwidth(int bw)
{
  if (IsConnected() || Bandwidth >= 0)
    return false;

  Bandwidth = bw;
  VERIFYX(bw > 0);
  return true;
}

inline void
BasePort::SetFanout(int fo)
{
  Fanout = fo;
}

// TODO: This hack should be removed when asim have a clock server 
// supporting multiple chip frequencies. 
#ifdef BARCELONA
extern UINT32 VBOX_CLOCK_RATIO;

inline bool
BasePort::SetLatency(int lat)
{
  if (IsConnected() || Latency >= 0)
    return false;
  Latency = lat * VBOX_CLOCK_RATIO;
  return true;
}    
#else 
inline bool
BasePort::SetLatency(int lat)
{
  if (IsConnected() || Latency >= 0)
    return false;
  Latency = lat;
  return true;
}
#endif 

inline bool
BasePort::Init(const char *name, int nodeId, int instance, const char *scope)
{
  if (Connected)
    return false;

  ASSERT(!Name, "This port has already been initialized with " << Name << ", now " << name); 
  ASSERT(name, "A name for a buffer must always be defined.");

  int name_len = strlen(name);
  Name = new char[name_len + 1];
  if (!Name)
    return false;

  // need to copy name_len + 1 so '\0' gets copied.  Otherwise 'Name',
  // which may get initialized with junk, may not be an exact copy of
  // 'name'.  Assertion below checks this.
  strncpy(Name, name, name_len + 1);

  VERIFYX(!strcmp(name, Name));

  Instance = instance;

  if (scope) {
    int scope_len = strlen(scope);
    Scope = new char[scope_len + 1];
    if (!Scope)
      return false;
    strncpy(Scope, scope, scope_len);
  }

  node = nodeId;
  return true;
}

inline bool
BasePort::Init(ASIM_CLOCKABLE m, const char *name, int nodeId, int instance, const char *scope)
{ return Init(name, nodeId, instance, scope); }

inline bool
BasePort::Config(int bw, int lat)
{ return SetBandwidth(bw) && SetLatency(lat); }

inline bool
BasePort::InitConfig(const char *name, int bw, int lat, int nodeId)
{ return Init(name, nodeId) && SetBandwidth(bw) && SetLatency(lat); }

inline bool
BasePort::InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId)
{ return BasePort::InitConfig(name, bw, lat, nodeId); }

inline const char*
BasePort::GetTypeName() const
{ VERIFYX(GetType() < LastType); return PortName[GetType()]; }

inline void *
BasePort::GetBuffer()
{
  ASSERT(false, "You cannot call GetBuffer() on this class type (" << GetName() << ")\n");
  return NULL;
}
inline bool
BasePort::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
  ASSERT(false, "You cannot call CreateStorage on this class type (" << GetName() << ")\n");
  return false;
}
inline bool
BasePort::DeleteStorage()
{
    //cout << "BasePort::DeleteStorage() called for " << Name << endl;
    return false;
}
inline void
BasePort::SetBuffer(void *buf, int rdPortNum)
{ ASSERT(false, "You cannot call SetBuffer() on this class type (" << GetName() << ")\n"); }

inline void
BasePort::SetBufferInfo()
{ ASSERT(false, "You cannot call SetBufferInfo() on this class type (" << GetName() << ").  This may be an indication of an error on the previous port in the list.  Run with -tr /PORTS/=1 as an argument to find the previous port in the list.\n"); }

inline void
BasePort::EventConnect(int bufNum, int destination)
{
  // Do nothing in the general case. This funcion will be redefined only by WritePorts
}

inline bool
operator==(const BasePort& l, const BasePort& r)
{ return (!strcmp(l.GetName(), r.GetName()) &&
	  l.GetInstance() == r.GetInstance() &&
	  l.GetType() == r.GetType()); }

inline bool
operator!=(const BasePort& l, const BasePort& r)
{ return !operator==(l, r); }

inline bool
operator< (const BasePort& l, const BasePort& r)
{
  int cmp = strcmp(l.GetName(), r.GetName());
  return (cmp < 0 ||
	  (cmp == 0 && l.GetInstance() < r.GetInstance()) ||
	  (cmp == 0 && l.GetInstance() == r.GetInstance() &&
	   l.GetType() < r.GetType()));
}

inline bool
operator> (const BasePort& l, const BasePort& r)
{
  int cmp = strcmp(l.GetName(), r.GetName());
  return (cmp > 0 ||
	  (cmp == 0 && l.GetInstance() > r.GetInstance()) ||
	  (cmp == 0 && l.GetInstance() == r.GetInstance() &&
	   l.GetType() > r.GetType()));
}

inline bool
operator<=(const BasePort& l, const BasePort& r)
{ return !operator>(l, r); }

inline bool
operator>=(const BasePort& l, const BasePort& r)
{ return !operator<(l, r); }


/////////////////////////////////
// class BufferStorage<T>
//

// Private members
// Buffer storage should never be copied;
template<class T, int S>
inline
BufferStorage<T,S>::BufferStorage(const BufferStorage& s)
{ operator=(s); }

template<class T, int S>
inline const BufferStorage<T,S>&
BufferStorage<T,S>::operator=(const BufferStorage& s)
{ ASSERT(false, "Cannot copy buffer storage!"); return *this; }

// Protected members
template<class T, int S>
inline bool
BufferStorage<T,S>::IsFull(int index) const
{ 
    return ((Store[index].End - Store[index].Start) >= Bandwidth);
}    

//{ return (Store[index].Start == 0) && (Store[index].End >= Bandwidth); }

template<class T, int S>
inline bool
BufferStorage<T,S>::IsEmpty(int index) const
{ 
    return ((bool)!Store || (Store[index].Start == Store[index].End)); 
}

template<class T, int S>
inline bool
BufferStorage<T,S>::CreateStorage(UINT32 latency, UINT32 bandwidth) 
{
    //The port will be inited right after this step, but we are about
    //to give the write port the handle to the buffer so we need to
    //allocate space now 

    Latency = latency;
    Bandwidth = bandwidth;

    //We need to have at least one entry and when the latency ==1 we
    //need to be able to let the read port read an empty slot on
    //startup and we will write the other entry that cycle.
    // CJB: in fact we need even more than this, if we are running in
    // parallel with lookahead.

    // FIX FIX should be tied to CLOCKSERVER_FUZZY_BARRIER_LOOKAHEAD !!
    const unsigned int         BUFFER_STORAGE_LOOKAHEAD = 3;
    BufferSize = Latency + 1 + BUFFER_STORAGE_LOOKAHEAD;
    Store = new CycleEntry[BufferSize];

    //Make sure that memory was allocated
    ASSERT(Store, "No storage was created!!");

    //iterate over the buffersize initializing all of the entries
    for (int count = 0; count < (BufferSize); count++) 
    {
        Store[count].Data=new T[Bandwidth];
        ASSERT(Store[count].Data, "No storage was created!!");

        Store[count].CycleWritten = -1;
        Store[count].Start = 0;
        Store[count].End = 0;
        //iterate over this entry's bandwidth and initialize all the entries
        for (int position = 0; position < Bandwidth; position++) 
        {
            Store[count].Data[position] = Dummy;
        }
    }  
    //set the write index to be the last buffer entry
    WriteIndex = BufferSize - 1;
    return true;
}


template<class T, int S>
inline bool
BufferStorage<T,S>::DeleteStorage() 
{
    if (Store == NULL)
    {
        return false;
    }

    // delete all buffer entries (which are bandwidth-sized arrays)
    for (int count = 0; count < (BufferSize); count++) 
    {
        delete [] Store[count].Data;
    }
    
    // delete the entire buffer (which is a latency-sized array)
    delete [] Store;

    // just to be safe
    BufferSize = 0;
    Store = NULL;

    return true;
}


// Public Members
template<class T, int S>
inline
BufferStorage<T,S>::BufferStorage()
  : myEventEdgeId(0),
    Store(NULL), 
    Dummy(T()), 
    Enabled(false), 
    Bandwidth(0),
    Latency(0),
    BufferSize(0),
    BufferMask(0),
    active(true), 
    stalled(false), 
    ReadIndex(0), 
    WriteIndex(1), 
    CycleRowRead(-1), 
    PeekReadIndex(0), 
    LastAccessed(0),
    LastWritten(0),
    SequentialWrites(0)
{
}

template<class T, int S>
inline
BufferStorage<T,S>::~BufferStorage()
{
    Clear();
}

// clear out everything - releases smart pointers
template<class T, int S>
inline void
BufferStorage<T,S>::Clear()
{
    if(Store)
    {
        // clear out everything - releases smart pointers
        for (int count = 0; count < (BufferSize); count++) {
            Store[count].CycleWritten = -1;
            Store[count].Start = 0;
            Store[count].End = 0;
            for (int position = 0; position < Bandwidth; position++) {
                Store[count].Data[position] = Dummy;
            }
        }  
        // also reset read and write indexes
        ReadIndex = 0;
        WriteIndex = BufferSize-1;
        PeekReadIndex = 0;
        SequentialWrites = 0;
    }
}

template<class T, int S>
inline int
BufferStorage<T,S>::GetBandwidth() const
{ return Bandwidth; }

template<class T, int S>
inline int
BufferStorage<T,S>::GetLatency() const
{ return Latency; }

template<class T, int S>
inline int
    BufferStorage<T,S>::GetBufferSize() const
{ return BufferSize; }

template<class T, int S>
inline bool
BufferStorage<T,S>::IsEnabled() const
{ return Enabled; }

template<class T, int S>
inline void
BufferStorage<T,S>::SetActive(bool a)
{ active = a; }

template<class T, int S>
inline void
BufferStorage<T,S>::SetStalled(bool s)
{ stalled = s; }

template<class T, int S>
inline bool
BufferStorage<T,S>::IsStalled() const 
{ return stalled;}

template<class T, int S>
inline void
BufferStorage<T,S>::PeekReset()
{ PeekReadIndex = ReadIndex; PeekStart = 0; }

// Enable storage.  Returns true on success (state was successfully
// changed to enabled) and false on failure (buffer storage was
// already enabled).
template<class T, int S>
inline bool
BufferStorage<T,S>::SetEnable(int bw, int lat, const char* portName)
{
  if (!Enabled) {
    Bandwidth = bw;
    Latency = lat;
    Enabled = true;
    return true;
  }

  return false;
}

// This checks to see if there's something that can be read from the
// buffer THIS cycle.  It doesn't, however, account for how many items that have
// already been read out this cycle.  So, it could return that there's something
// to read, but because the full bandwidth has already been read out this cycle,
// the port won't allow any more items to be read.  Is this really needed now?  Eric
template<class T, int S>
inline bool
BufferStorage<T,S>::SomethingToRead(UINT64 cycle) const
{
  int ri = ReadIndex;

  // move ri to the first location that might have data.
  while (IsEmpty(ri) && (ri != WriteIndex)) {
    if (++ri >= BufferSize)
      ri = 0;
  }

  // if no data, return false.
  if (IsEmpty(ri))
      return false;

  UINT64 ReadCycle = Store[ri].CycleWritten + Latency;

  // if next thing to read isn't ready to be read, return false
  if (ReadCycle > cycle)
    return false;

  // if we get here, there IS something that could be read this cycle
  return true;
}


template<class T, int S>
inline bool
BufferStorage<T,S>::Read(T& data, UINT64 cycle, const char* portName, bool relaxAsserts)
{
    // if no data, return false.  I don't think this first condidtion
    // should ever be true since we're always advancing readindex at t
    // the end when all items are read out.  Maybe upon startup, but
    // maybe 2nd condition can take care of that.
    if (IsEmpty(ReadIndex) || (CycleRowRead == (INT64) cycle && Store[ReadIndex].Start == 0)) 
    {
        return false;
    }
    
    CycleEntry &entry = Store[ReadIndex];
    UINT64 ReadCycle = entry.CycleWritten + Latency;
    
    // if next thing to read isn't ready to be read, return false
    if (ReadCycle > cycle)
    {
        return false;
    }

    // do we want to add another branch condition here?  This code is
    // executed so often it may hurt performance.  Eric
    if (!relaxAsserts)
    {
        ASSERT(ReadCycle == cycle,
               "Reading data from port " << portName <<
	       " in the wrong cycle!  (got " << cycle << 
	       " instead of required: " << ReadCycle << ")");
    }
    else {
	if (SequentialWrites == UINT32(Latency) + 1) {
	    // if there have been too many writes w/out a read, then the port
	    // is being mis-used --> Only makes sense for a stall port
	    ASSERT(LastWritten == cycle, "Write to " << portName << 
		   " (cycle " << LastWritten << "): illegal use of port");
	}
    }

    //  Clear this
    SequentialWrites = 0;
    
    //  T1("\tport = " << portName
    //    << ", start = " << entry.Start
    //    << ", end = " << entry.End
    //    << ", ReadIndex = " << ReadIndex);

    // read Data
    data = entry.Data[entry.Start];
    entry.Data[entry.Start] = Dummy;

    if (entry.Start == 0)
    {
        CycleRowRead = (INT64)cycle;
    }
         
    entry.Start += 1;

    // if we're reading this port, it must be active.  To be ultra safe, this
    // should be the first line in this method.  However, it's probably safe to
    // wait until we actually read something out until we set it as active.  We
    // MIGHT lose a few cycles of error checking in the write code at the very
    // beginning of the run.  But then we only set it when we're actually
    // setting ReadIndex, so we're paying for a write to (hopefully) the same
    // cache line anyway.
    active = true;

    // the assumption here is that ReadIndex always points to the correct spot,
    // even at start up.
    if (IsEmpty(ReadIndex))
    {
        ReadIndex++;
        if (ReadIndex >= (BufferSize))
        {
            ReadIndex = 0;
        }
    }

    return true;
}

template<class T, int S>
inline void
BufferStorage<T,S>::Delay(INT64 delay, const char* portName, bool relaxAsserts)
{
    for (int i = 0; i < BufferSize; ++i)
    {
        Store[i].CycleWritten += delay;
    }

    return;
}

template<class T, int S>
inline bool
BufferStorage<T,S>::Look(T& data, UINT64 cycle, const char* portName, bool relaxAsserts)
{
    // if no data, return false.  I don't think this first condidtion
    // should ever be true since we're always advancing readindex at t
    // the end when all items are read out.  Maybe upon startup, but
    // maybe 2nd condition can take care of that.
    if (IsEmpty(ReadIndex) || (CycleRowRead == (INT64) cycle && Store[ReadIndex].Start == 0)) 
    {
        return false;
    }
    
    CycleEntry &entry = Store[ReadIndex];
    UINT64 ReadCycle = entry.CycleWritten + Latency;
    
    // if next thing to read isn't ready to be read, return false
    if (ReadCycle > cycle)
    {
        return false;
    }

    // do we want to add another branch condition here?  This code is
    // executed so often it may hurt performance.  Eric
    if (!relaxAsserts)
    {
        ASSERT(ReadCycle == cycle,
               "Reading data from port " << portName <<
	       " in the wrong cycle!  (got " << cycle << 
	       " instead of required: " << ReadCycle << ")");
    }
    else {
	if (SequentialWrites == UINT32(Latency) + 1) {
	    // if there have been too many writes w/out a read, then the port
	    // is being mis-used --> Only makes sense for a stall port
	    ASSERT(LastWritten == cycle, "Write to " << portName << 
		   " (cycle " << LastWritten << "): illegal use of port");
	}
    }

    // read Data
    data = entry.Data[entry.Start];

    return true;
}

template<class T, int S>
inline bool
BufferStorage<T,S>::Write(const T& data, UINT64 cycle, const char* portName)
{
    if (((UINT64)Store[WriteIndex].CycleWritten) != cycle) 
    {
        // this assert isn't really THAT necessary.  I mean, time always
        // advances, so you'd have to try REAL hard to get this to fail....
        // Santi Galan @ BSSAD
        // We must do this funny condition because gcc 3.1.0 detect a spurious
        // parse error in th first ")"

        ASSERT( ! (  (INT64)Store[WriteIndex].CycleWritten >= (INT64)cycle ), 
                "Port " << portName << " buffers are fifo and time must always advance" << endl);

        WriteIndex++;
        if (WriteIndex >= (BufferSize))
        {
            WriteIndex = 0;
        }

        ASSERT(IsEmpty(WriteIndex) || !active,
               "Port " << portName << " trying to write to buffer entry "
               << WriteIndex << " that has data in it");

        // Note! CycleWritten has to be set before Start and 
        // End are initialized. Do not re-order these 
        // statements unless you know what you are doing.
        // This ordering is needed for the parallel runs.

        Store[WriteIndex].CycleWritten = (INT64)cycle;
        MemBarrier();

        // this is the first time we're writing into this row this cycle, so reset
        // the start and end to 0.
        Store[WriteIndex].Start = 0;
        Store[WriteIndex].End = 0;
        
        LastWritten = (UINT64)cycle;

	++SequentialWrites;
    }

    // in order to use entry as a reference to Store[], the compiler
    // required it to be on assignment and declaration to be the same
    // line.  BUT, the value of WriteIndex might change in above if
    // body, so we shouldn't assign entry until here. - Eric
    CycleEntry &entry = Store[WriteIndex];
    
    //     T1("\tport = " << portName
    //    << ", start = " << entry.Start
    //   << ", end = " << entry.End
    //   << ", WriteIndex = " << WriteIndex);
    
    // we really want to check to see if we've exceed the bandwidth this cycle,
    // so let's check for just that.
    //    ASSERT(!IsFull(WriteIndex), "Port " << portName << "
    //    exceeded bandwidth!");

    ASSERT(Store[WriteIndex].End < Bandwidth, "Port " << portName << " exceeded bandwidth (" << Bandwidth << ")!");

    // This assert is sort of redundant with the one above that checks for
    // IsEmpty() the first time you write this row.  So, this assertion should
    // NEVER fire, since the one above will (should) catch all these cases.
    //    ASSERT((entry.Data[entry.End] == Dummy) || !active, "Port " << portName <<
    //           " buffer data was either written twice or modified externally.");

    // can't write into a port that's stalled
    ASSERT(IsStalled() == false, "Trying to write port " << portName << " while it's stalled!\n");

    entry.Data[entry.End] = data;
    entry.End += 1;
    
    // Automatic Events notify
    // Note: If you get a compile warning on this line with something like:
    //
    //      warning: cannot pass objects of non-POD type xxxxxx through `...'
    //
    // The class xxxxx MUST inherit from ASIM_ITEM_CLASS if you want to
    // obtain events or inherit from ASIM_SILENT_ITEM_CLASS if you
    // don't want events.
    // This error may also manifest itself by a SEGFLT.
    if (runWithEventsOn)
    {
        EVENT(Notify(data));
    }
    
    return true;
}

template <class T, int F>
inline INT64
BufferStorage<T,F>::LatestWrite(){
    return (Store[WriteIndex].CycleWritten);
}

// Generate an event only in the case of ASIM_ITEM 
template <class T, int F>
inline void
BufferStorage<T,F>::Notify(const ASIM_ITEM& data)
{
    if (runWithEventsOn && data && data->GetEventsEnabled())
    {
        DRALEVENT(MoveItem(GetEventEdgeId(), data->GetItemId()));
    }
}

template <class T, int F>
inline void
BufferStorage<T,F>::Notify(const ASIM_ITEM_CLASS& data)
{
    if (runWithEventsOn && data.GetEventsEnabled())
    {
        DRALEVENT(MoveItem(GetEventEdgeId(), data.GetItemId()));
    }
}

// Other-wise do nothing
template <class T, int F>
inline void
BufferStorage<T,F>::Notify(const ASIM_SILENT_ITEM& data) 
{ 
}

template <class T, int F>
inline void
BufferStorage<T,F>::Notify(const ASIM_SILENT_ITEM_CLASS& data) 
{ 
}

template <class T, int F>
inline void
BufferStorage<T,F>::Notify(void *data) 
{
    // this case handles pointers or smart pointers to non-ASIM_ITEM_CLASS objects silently
    // and without the non-POD issues of the case below.
}

template <class T, int F>
inline void
BufferStorage<T,F>::Notify(...)
{
    // Note: If you get a compile warning on this line with something like:
    //
    //      warning: cannot pass objects of non-POD type xxxxxx through `...'
    //
    // The class xxxxx MUST inherit from ASIM_ITEM_CLASS if you want to
    // obtain events or inherit from ASIM_SILENT_ITEM_CLASS if you
    // don't want events.
    // This error may also manifest itself by a SEGFLT or ILLEGAL INSTRUCTION.
}

template <class T, int F>
inline void
BufferStorage<T,F>::Notify(const T& data)
{
    // This should cover non ASIM_ITEM_CLASS and ASIM_SILENT_ITEM_CLASS cases
    // and save users the misery caused by those ILLEGAL INSTRUCTION errors and
    // segfaults.
}

template<class T, int S>
inline bool
BufferStorage<T,S>::PeekNext(T& data, UINT64 cycle)
{
  // move PeekReadIndex to the first location that might have data.
  while ((PeekStart == Store[PeekReadIndex].End) && (PeekReadIndex != WriteIndex)) {
    PeekStart = 0;  
    if (++PeekReadIndex >= (BufferSize))
      PeekReadIndex = 0;
  }

  // if no data, return false.  Can't use IsEmpty, because we're not
  // reading items out of buffer, and therefore it's never empty.
  if (PeekStart == Store[PeekReadIndex].End)
      return false;

  CycleEntry &entry = Store[PeekReadIndex];
  UINT64 ReadCycle = entry.CycleWritten + Latency;

  // if next thing to read isn't ready to be read, return false
  if (ReadCycle > cycle)
    return false;
  
  // read Data
  data = entry.Data[PeekStart];
  VERIFYX(data);
  PeekStart++;

  return true;
}

/////////////////////
// class ConfigPort
//
inline BasePort::PortType
ConfigPort::GetType() const
{ return ConfigType; }

///////////////////////////
// class ReadPort<T>
//
template <class T>
inline bool
ReadPort<T>::Read(T& data, UINT64 cycle)
{ return Buffer.Read(data, cycle, GetName()); }

template <class T>
inline BasePort::PortType
ReadPort<T>::GetType() const
{ return ReadType; }

template <class T>
inline void *
ReadPort<T>::GetBuffer()
{   
    return reinterpret_cast<void *>(&Buffer); 
}

template <class T>
inline bool
ReadPort<T>::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
    return Buffer.CreateStorage(latency, bandwidth);
}

template <class T>
inline bool
ReadPort<T>::DeleteStorage()
{
    //cout << "ReadPort::DeleteStorage() called for " << Name << endl;
    return Buffer.DeleteStorage();
}

template <class T>
inline void
ReadPort<T>::SetBufferInfo()
{
  ASSERT(Bandwidth > 0,
    "Port " << GetName() << " bandwidth not set.");
  ASSERT(Latency >= 0,
    "Port " << GetName() << " latency not set.");

  Buffer.SetEnable(Bandwidth, Latency, GetName());
}

template <class T>
inline INT16
ReadPort<T>::GetEventEdgeId()
{ return Buffer.GetEventEdgeId(); }

template <class T>
inline void
ReadPort<T>::Clear()
{ Buffer.Clear(); }


///////////////////////////
// class ReadSkidPort<T, Storage>
//
template <class T, int S>
inline bool
ReadSkidPort<T,S>::Read(T& data, UINT64 cycle)
{ return Buffer.Read(data, cycle, GetName(), true); }

template <class T, int S>
inline bool
ReadSkidPort<T,S>::SomethingToRead(UINT64 cycle) const
{ return Buffer.SomethingToRead(cycle); }

template <class T, int S>
inline BasePort::PortType
ReadSkidPort<T,S>::GetType() const
{ return ReadType; }

template <class T, int S>
inline void *
ReadSkidPort<T,S>::GetBuffer()
{   
    return reinterpret_cast<void *>(&Buffer); }

template <class T, int S>
inline bool
ReadSkidPort<T,S>::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
    return Buffer.CreateStorage(latency, bandwidth);
}

template <class T, int S>
inline bool
ReadSkidPort<T,S>::DeleteStorage()
{
    //cout << "ReadSkidPort::DeleteStorage() called for " << Name << endl;
    return Buffer.DeleteStorage();
}

template <class T, int S>
inline void
ReadSkidPort<T,S>::SetBufferInfo()
{
  ASSERT(Bandwidth > 0,
    "Port " << GetName() << " bandwidth not set.");
  ASSERT(Latency >= 0,
    "Port " << GetName() << " latency not set.");

  Buffer.SetEnable(Bandwidth, Latency, GetName());
}

template <class T, int S>
inline INT16
ReadSkidPort<T,S>::GetEventEdgeId()
{ return Buffer.GetEventEdgeId(); }

template <class T, int S>
inline void
ReadSkidPort<T,S>::Clear()
{ Buffer.Clear(); }


///////////////////////////
// class ReadStallPort<T, Storage>
//
template <class T>
inline bool
ReadStallPort<T>::Read(T& data, UINT64 cycle)
{
    //  we need to relax the asserts here since this port
    //  does not need to be read every cycle
    return Buffer.Read(data, cycle, GetName(), true); 
}

template <class T>
inline bool
ReadStallPort<T>::Look(T& data, UINT64 cycle)
{
    //  we need to relax the asserts here since this port
    //  does not need to be read every cycle
    return Buffer.Look(data, cycle, GetName(), true); 
}

template <class T>
inline void
ReadStallPort<T>::Stall(bool s)
{
    if (s)
    {
        Buffer.SetStalled(true);
        Buffer.Delay(1, GetName(), true); 
    }
    else
    {
        Buffer.SetStalled(false);
    }
}

template <class T>
inline BasePort::PortType
ReadStallPort<T>::GetType() const
{ return ReadType; }

template <class T>
inline void
ReadStallPort<T>::SetStalled(bool s)
{ return Buffer.SetStalled(s); }

template <class T>
inline bool
ReadStallPort<T>::IsStalled()
{ return Buffer.IsStalled(); }

template <class T>
inline void *
ReadStallPort<T>::GetBuffer()
{  
    return reinterpret_cast<void *>(&Buffer); 
}

template <class T>
inline bool
ReadStallPort<T>::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
    return Buffer.CreateStorage(latency, bandwidth);
}

template <class T>
inline bool
ReadStallPort<T>::DeleteStorage()
{
    //cout << "ReadStallPort::DeleteStorage() called for " << Name << endl;
    return Buffer.DeleteStorage();
}

template <class T>
inline void
ReadStallPort<T>::SetBufferInfo()
{
  ASSERT(Bandwidth > 0,
    "Port " << GetName() << " bandwidth not set.");
  ASSERT(Latency >= 0,
    "Port " << GetName() << " latency not set.");

  Buffer.SetEnable(Bandwidth, Latency, GetName());
}

template <class T>
inline INT16
ReadStallPort<T>::GetEventEdgeId()
{ return Buffer.GetEventEdgeId(); }

template <class T>
inline void
ReadStallPort<T>::Clear()
{ Buffer.Clear(); }

///////////////////////////
// class ReadRemotePort<T, Storage>
//

template <class T>
inline bool
ReadPort<T>::IsActive()
{ return Buffer.IsActive(); }

template <class T>
inline void
ReadPort<T>::Deactivate()
{ Buffer.SetActive(false); }

template <class T>
inline UINT64
ReadPort<T>::GetLastAccessed()
{ return Buffer.GetLastAccessed(); }

template <class T>
inline bool
ReadPort<T>::ReadRemote(T& data, UINT64 cycle)
{ 
    // Wait for producer to store something.  This will block if nothing ever
    // becomes available.
    bool firstPass = true;
    while(!Buffer.SomethingToRead(cycle))
    {
        if (firstPass)
        {
            T1("Not ready at cycle " << cycle);
            firstPass = false;
        }
        //
        // We need some exponential backoff here...
        //
    }

    T1("Ready at cycle " << cycle);
    return Buffer.Read(data, cycle, GetName()); 
}

////////////////////////////
// class WritePhasePort<T,F>
//
template <class T, int F>
inline
WritePhasePort<T,F>::WritePhasePort()
  : Buffer(), Fanout(F)
    // TO DO: Do we need Fanout above?  I don't think so, only setfanout below
    // TO DO: init Buffer array to NULL - Eric
{
  SetFanout(F);
}

template <class T, int F>
inline bool
WritePhasePort<T,F>::Write(T data, UINT64 cycle)
{ 
  VERIFYX(IsConnected());
  UINT64 internal_cycle = cycle*2;
  for (int f = 0; f < Fanout; f++) {
    Buffer[f]->Write(data, internal_cycle, GetName()); 
  }  
  // assertions above and in Write method will never allow them to
  // return false, so...return true, or else make this a void function
  return true;
}

template <class T, int F>
inline bool
WritePhasePort<T,F>::Write(T data, PHASE ph)
{ 
  VERIFYX(IsConnected());
  UINT64 internal_cycle = ph.getPhaseNum();
  for (int f = 0; f < Fanout; f++) {
    Buffer[f]->Write(data, internal_cycle, GetName()); 
  }  
  // assertions above and in Write method will never allow them to
  // return false, so...return true, or else make this a void function
  return true;
}

template <class T, int F>
inline bool
WritePhasePort<T,F>::Write(T data, UINT64 cycle, CLK_EDGE ed)
{ 
  VERIFYX(IsConnected());
  UINT64 internal_cycle = cycle*2 + ed;
  for (int f = 0; f < Fanout; f++) {
    Buffer[f]->Write(data, internal_cycle, GetName()); 
  }  
  // assertions above and in Write method will never allow them to
  // return false, so...return true, or else make this a void function
  return true;
}

template <class T, int F>
inline BasePort::PortType
WritePhasePort<T,F>::GetType() const
{ return WritePhaseType; }

template <class T, int F>
inline void
WritePhasePort<T,F>::SetBuffer(void *buf, int rdPortNum)
{ VERIFYX(rdPortNum <= Fanout); Buffer[rdPortNum] = reinterpret_cast< Storage<T>* >(buf); }

template <class T, int F>
inline void
WritePhasePort<T,F>::EventConnect(int bufNum, int destination)
{
    if (runWithEventsOn)
    {
        EVENT(Buffer[bufNum]->SetEventEdgeId(DRALEVENT(NewEdge(node, destination, 
                  Buffer[bufNum]->GetBandwidth(), Buffer[bufNum]->GetLatency(),
                  GetName(), true))));   
    }
}

template <class T, int F>
inline int
WritePhasePort<T,F>::GetLatency() const
{
  // If this assert fails it may mean:
  // - You are asking the latency of a 
  //   multicast (fanout > 1) multilatency port on the write end of the port.
  VERIFYX(Latency >= 0); 
  return Latency; 
}

template <class T, int F>
inline INT16
WritePhasePort<T,F>::GetEventEdgeId()
{ return Buffer[0]->GetEventEdgeId(); }

template <class T, int F>
bool
WritePhasePort<T,F>::SetLatency(int lat)
{ return BasePort::SetLatency(lat*2); }

template <class T, int F>
bool
WritePhasePort<T,F>::SetLatencyPhases(int lat)
{ return BasePort::SetLatency(lat); }

template <class T, int F>
bool
WritePhasePort<T,F>::Config(int bw, int lat)
{ return BasePort::Config(bw, 2*lat); }

template <class T, int F>
bool
WritePhasePort<T,F>::InitConfig(const char *name, int bw, int lat, int nodeId)
{ return BasePort::InitConfig(name, bw, 2*lat, nodeId); }

template <class T, int F>
bool
WritePhasePort<T,F>::InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId)
{ return WritePhasePort<T,F>::InitConfig(name, bw, lat, nodeId); }

template <class T, int F>
inline void
WritePhasePort<T,F>::Clear()
{ Buffer.Clear(); }


///////////////////////////
// class ReadPhasePort<T>
//
template <class T>
inline bool
ReadPhasePort<T>::Read(T& data, UINT64 cycle)
{ 
    UINT64 internal_cycle = cycle * 2;
    return Buffer.Read(data, internal_cycle, GetName());
}

template <class T>
inline bool
ReadPhasePort<T>::Read(T& data, PHASE ph)
{ 
    UINT64 internal_cycle = ph.getPhaseNum();
    return Buffer.Read(data, internal_cycle, GetName());
}

template <class T>
inline bool
ReadPhasePort<T>::Read(T& data, UINT64 cycle, CLK_EDGE ed)
{ 
    UINT64 internal_cycle = cycle * 2 + ed;
    return Buffer.Read(data, internal_cycle, GetName());
}

template <class T>
inline BasePort::PortType
ReadPhasePort<T>::GetType() const
{ return ReadPhaseType; }

template <class T>
inline void *
ReadPhasePort<T>::GetBuffer()
{  return reinterpret_cast<void *>(&Buffer); }

template <class T>
inline bool
ReadPhasePort<T>::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
    // Previously, it used DEFAULT_MAX_LATENCY * 2 to size the array, but when
    // done dynamically, this wasn't used.  What should it be?  Eric
    return Buffer.CreateStorage(latency * 2, bandwidth);
}

template <class T>
inline bool
ReadPhasePort<T>::DeleteStorage()
{
    //cout << "ReadPhasePort::DeleteStorage() called for " << Name << endl;
    return Buffer.DeleteStorage();
}

template <class T>
inline void
ReadPhasePort<T>::SetBufferInfo()
{
  ASSERT(Bandwidth > 0,
    "Port " << GetName() << " bandwidth not set.");
  ASSERT(Latency >= 0,
    "Port " << GetName() << " latency not set.");

  Buffer.SetEnable(Bandwidth, Latency, GetName());
}

template <class T>
inline INT16
ReadPhasePort<T>::GetEventEdgeId()
{ return Buffer.GetEventEdgeId(); }

template <class T>
bool
ReadPhasePort<T>::SetLatency(int lat)
{ return BasePort::SetLatency(lat*2); }

template <class T>
bool
ReadPhasePort<T>::SetLatencyPhases(int lat)
{ return BasePort::SetLatency(lat); }

template <class T>
bool
ReadPhasePort<T>::Config(int bw, int lat)
{ return BasePort::Config(bw, 2*lat); }

template <class T>
bool
ReadPhasePort<T>::InitConfig(const char *name, int bw, int lat, int nodeId)
{ return BasePort::InitConfig(name, bw, 2*lat, nodeId); }

template <class T>
bool
ReadPhasePort<T>::InitConfig(ASIM_CLOCKABLE m, const char *name, int bw, int lat, int nodeId)
{ return ReadPhasePort<T>::InitConfig(name, bw, lat, nodeId); }

template <class T>
inline void
ReadPhasePort<T>::Clear()
{ Buffer.Clear(); }

////////////////////////////
// class WritePort<T,F>
//
template <class T, int F>
inline
WritePort<T,F>::WritePort()
  : Buffer(), Fanout(F)
    // TO DO: Do we need Fanout above?  I don't think so, only setfanout below
    // TO DO: init Buffer array to NULL - Eric
{
  SetFanout(F);
}

template <class T, int F>
inline bool
WritePort<T,F>::Write(T data, UINT64 cycle)
{ 
  VERIFYX(IsConnected()); 
  for (int f = 0; f < Fanout; f++) {
    Buffer[f]->Write(data, cycle, GetName()); 
  }  
  // assertions above and in Write method will never allow them to
  // return false, so...return true, or else make this a void function
  return true;
}

template <class T, int F>
inline bool
WritePort<T,F>::WriteRemote(T data, UINT64 cycle)
{
  VERIFYX(IsConnected()); 
  for (int f = 0; f < Fanout; f++) {
      int timeout=1000000;
      int wi = 
          (Buffer[f]->GetWriteIndex() == (Buffer[f]->GetBufferSize() - 1)) ? 0 : Buffer[f]->GetWriteIndex() + 1;
      while(! (Buffer[f]->IsEmpty(wi))){
          //this assert is actually in the Buffer[f]->Write()
          //function, but in threaded mode it may fire due to excess slippage
          //ASSERT(timeout--, "not draining this port fast enough.  writer will overflow it");
      }

      Buffer[f]->Write(data, cycle, GetName()); 


  }  
  // assertions above and in Write method will never allow them to
  // return false, so...return true, or else make this a void function
  return true;
}

template <class T, int F>
inline INT64
WritePort<T,F>::LatestWrite()
{
  VERIFYX(IsConnected()); 
  return (Buffer[0]->LatestWrite());
}



template <class T, int F>
inline void
WritePort<T,F>::Deactivate()
{
  Buffer[0]->SetActive(false); 
}

    
template <class T, int F>
inline void
WritePort<T,F>::SetLastAccessed(UINT64 c)
{
  VERIFYX(IsConnected()); 
  Buffer[0]->SetLastAccessed(c);
}


template <class T, int F>
inline BasePort::PortType
WritePort<T,F>::GetType() const
{ return WriteType; }

template <class T, int F>
inline void
WritePort<T,F>::SetBuffer(void *buf, int rdPortNum)
{ VERIFYX(buf);
 VERIFYX(rdPortNum <= Fanout); Buffer[rdPortNum] = reinterpret_cast< Storage<T>* >(buf); }

template <class T, int F>
inline void
WritePort<T,F>::EventConnect(int bufNum, int destination)
{
    if (runWithEventsOn)
    {
        EVENT(Buffer[bufNum]->SetEventEdgeId(DRALEVENT(NewEdge(node, destination, 
                  Buffer[bufNum]->GetBandwidth(), Buffer[bufNum]->GetLatency(),
                  GetName(), true))));   
    }
}

template <class T, int F>
inline int
WritePort<T,F>::GetLatency() const
{
  // If this assert fails it may mean:
  // - You are asking the latency of a 
  //   multicast (fanout > 1) multilatency port on the write end of the port.
  VERIFYX(Latency >= 0); 
  return Latency; 
}

template <class T, int F>
inline INT16
WritePort<T,F>::GetEventEdgeId()
{ return Buffer[0]->GetEventEdgeId(); }

template <class T, int F>
inline void
WritePort<T,F>::Clear()
{
  for (int f = 0; f < Fanout; f++) {
    Buffer[f]->Clear();
  }
}

////////////////////////////
// class WriteSkidPort<T,S>
//
template <class T, int S>
inline
WriteSkidPort<T,S>::WriteSkidPort()
  : Buffer(NULL)
{
  // can't have a Fanout with Skid port, but need to specify a value
  // for everything to work
  SetFanout(1);
}

template <class T, int S>
inline bool
WriteSkidPort<T,S>::Write(T data, UINT64 cycle)
{ return IsConnected() && Buffer->Write(data, cycle, GetName()); }

template <class T, int S>
inline BasePort::PortType
WriteSkidPort<T,S>::GetType() const
{ return WriteType; }

template <class T, int S>
inline void
WriteSkidPort<T,S>::SetBuffer(void *buf, int rdPortNum)
{ Buffer = reinterpret_cast< Storage<T,S>* >(buf); }

template  <class T, int S>
inline void
WriteSkidPort<T,S>::EventConnect(int bufNum, int destination)
{
    if (runWithEventsOn)
    {
        EVENT(Buffer->SetEventEdgeId(DRALEVENT(NewEdge(node, destination, 
                  Buffer->GetBandwidth(), Buffer->GetLatency(), GetName()))));   
    }
}

template <class T, int S>
inline void
WriteSkidPort<T,S>::Clear()
{ Buffer->Clear(); }

////////////////////////////
// class WriteStallPort<T>
//
template <class T>
inline
WriteStallPort<T>::WriteStallPort()
  : Buffer(NULL)
{
  // can't have a Fanout with Stall port, but need to specify a value
  // for everything to work
  SetFanout(1);
}

template <class T>
inline bool
WriteStallPort<T>::Write(T data, UINT64 cycle)
{ return IsConnected() && Buffer->Write(data, cycle, GetName()); }

template <class T>
inline bool
WriteStallPort<T>::IsStalled()
{ return Buffer->IsStalled(); }

template <class T>
inline BasePort::PortType
WriteStallPort<T>::GetType() const
{ return WriteType; }

template <class T>
inline void
WriteStallPort<T>::SetBuffer(void *buf, int rdPortNum)
{ Buffer = reinterpret_cast< Storage<T>* >(buf); }

template <class T>
inline void
WriteStallPort<T>::EventConnect(int bufNum, int destination)
{
    if (runWithEventsOn)
    {
        EVENT(Buffer->SetEventEdgeId(DRALEVENT(NewEdge(node, destination, 
                  Buffer->GetBandwidth(), Buffer->GetLatency(), GetName()))));   
    }
}

template <class T>
inline void
WriteStallPort<T>::Clear()
{ Buffer->Clear(); }


///////////////////////////
// class PeekPort<T>
//
template <class T>
inline void
PeekPort<T>::PeekReset()
{ VERIFYX(Buffer); Buffer->PeekReset(); }

template <class T>
inline bool
PeekPort<T>::PeekNext(T& data, UINT64 cycle)
{ return Buffer->PeekNext(data, cycle); }

template <class T>
inline BasePort::PortType
PeekPort<T>::GetType() const
{ return PeekType; }

template <class T>
inline void *
PeekPort<T>::GetBuffer()
{ return reinterpret_cast<void *>(&Buffer); }

template <class T>
inline bool
PeekPort<T>::CreateStorage(UINT32 latency, UINT32 bandwidth)
{
    return Buffer->CreateStorage(latency, bandwidth);
}

template <class T>
inline bool
PeekPort<T>::DeleteStorage()
{
    return Buffer->DeleteStorage();
}

template <class T>
inline INT16
PeekPort<T>::GetEventEdgeId()
{ return Buffer->GetEventEdgeId(); }

template <class T>
inline void
PeekPort<T>::SetBuffer(void *buf, int rdPortNum)
{ Buffer = reinterpret_cast< Storage<T>* >(buf); }

template <class T>
inline void
PeekPort<T>::Clear()
{ Buffer->Clear(); }



// Orig. PeekPort
///////////////////////////////////////////////////////////////////
// class PeekPort<T> : public BasePort
//
template<class T>
class OrigPeekPort : public BasePort
{
private:
  const Storage<T> *Buffer;

public:
  class ConstIterator
  {
  private:
  public:
    // Default constructor initializes the iterator.  When creating an
    // iterator with the default constructor, we are unable to call
    // any function other than operator=.  This is because the
    // iterator does not actually point to anything and is not
    // dereferencable.
    ConstIterator();

    // These two copy constructors will simply duplicate the position
    // of an iterator.  The second copy constructor can also be used
    // for implicit casting from an Iterator to a Const Iterator.
    ConstIterator(const ConstIterator& iter);

    // No destructor is needed for the iterator.
    ~ConstIterator() {}

    // Preincrement, postincrement, predecrement, postdecrement.
    // One unfortunate side affect of the way that this code must be
    // written is that preincrement (and predecrement) is faster that
    // postincrement (and postdecrement).  So, unless you need the
    // functionality, favor the pre* version rather than the post*
    // version.
    // e.g.
    //   use:
    //     ++iter;
    //   in place of:
    //     iter++;
    //
    const ConstIterator& operator++();
    const ConstIterator operator++(int);
    const ConstIterator& operator--();
    const ConstIterator operator--(int);

    // Since Iterators are supposed to behave like pointers, we must
    // overload these operators so that we can add an offset.
    const ConstIterator& operator+(int offset);
    const ConstIterator& operator-(int offset);


    // Changes what the iterator points to.
    const ConstIterator& operator=(const ConstIterator& iter);

    // Simple comparison.
#if 0
    friend bool operator==(const ConstIterator& l, const ConstIterator& r);
    friend bool operator!=(const ConstIterator& l, const ConstIterator& r);
    friend bool operator< (const ConstIterator& l, const ConstIterator& r);
    friend bool operator> (const ConstIterator& l, const ConstIterator& r);
    friend bool operator<=(const ConstIterator& l, const ConstIterator& r);
    friend bool operator>=(const ConstIterator& l, const ConstIterator& r);
#endif

    // operator* is used to dereference the iterator.  When the
    // iterator is dereferenced, what is returned is the data that is
    // at the node that the iterator is pointing to.
    // e.g.
    //     asim::Vector<int>::Iterator iter = vector.Begin();
    //     int x = *iter;
    const T& operator*() const;

    // Since we are talking about a vector here, we should be able to
    // use random access to get to anywhere we want to in the vector in
    // O(1) time.
    const T& operator[](int index);

    // operator-> is used to point to data members of the data that
    // the iterator is pointing to.  N.B. if the iterator data type is 
    // a pointer. operator-> does not make sense because it would be
    // working on a pointer to a pointer.
    // e.g.  THIS IS BAD!
    //     struct my_struct {
    //       int x;
    //       int y;
    //       int z;
    //     };
    //     ...
    //     asim::Vector<my_struct*>::Iterator iter = vector.Begin();
    //     int i = iter->z;    // BAD!  iter-> returns a my_struct**
    //     int j = (*iter)->z  // OK!   iter* returns a my_struct*
    const T* operator->() const;
  };

public:
  OrigPeekPort();

  ConstIterator& Begin(UINT64 cycle) const;
  virtual PortType GetType() const;
};

#endif //_PORT_
