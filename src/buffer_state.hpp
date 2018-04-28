// $Id$

/*
 Copyright (c) 2007-2015, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _BUFFER_STATE_HPP_
#define _BUFFER_STATE_HPP_

#include <vector>
#include <queue>

#include "module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "config_utils.hpp"

class BufferState : public Module {
  
  class BufferPolicy : public Module {
  protected:
    BufferState const * const _buffer_state;
  public:
    BufferPolicy(Configuration const & config, BufferState * parent, 
		 const string & name);
    virtual void SetMinLatency(int min_latency) {}
    virtual void TakeBuffer(int vc = 0);
    virtual void SendingFlit(Flit const * const f);
    virtual void FreeSlotFor(int vc = 0);
    virtual bool IsFullFor(int vc = 0) const = 0;
    virtual int AvailableFor(int vc = 0) const = 0;
    virtual int LimitFor(int vc = 0) const = 0;

    static BufferPolicy * New(Configuration const & config, 
			      BufferState * parent, const string & name);
  };
  
  class PrivateBufferPolicy : public BufferPolicy {
  protected:
    int _vc_buf_size;
    int _duty_buf_size;
  public:
    PrivateBufferPolicy(Configuration const & config, BufferState * parent, 
			const string & name);
    virtual void SendingFlit(Flit const * const f);
    virtual bool IsFullFor(int vc = 0) const;
    virtual int AvailableFor(int vc = 0) const;
    virtual int LimitFor(int vc = 0) const;
  };
  
  class SharedBufferPolicy : public BufferPolicy {
  protected:
    int _buf_size;
    vector<int> _private_buf_vc_map;
    vector<int> _private_buf_size;
    vector<int> _private_buf_occupancy;
    int _shared_buf_size;
    int _shared_buf_occupancy;
    vector<int> _reserved_slots;
    void ProcessFreeSlot(int vc = 0);
  public:
    SharedBufferPolicy(Configuration const & config, BufferState * parent, 
		       const string & name);
    virtual void SendingFlit(Flit const * const f);
    virtual void FreeSlotFor(int vc = 0);
    virtual bool IsFullFor(int vc = 0) const;
    virtual int AvailableFor(int vc = 0) const;
    virtual int LimitFor(int vc = 0) const;
  };

  class LimitedSharedBufferPolicy : public SharedBufferPolicy {
  protected:
    int _vcs;
    int _active_vcs;
    int _max_held_slots;
  public:
    LimitedSharedBufferPolicy(Configuration const & config, 
			      BufferState * parent,
			      const string & name);
    virtual void TakeBuffer(int vc = 0);
    virtual void SendingFlit(Flit const * const f);
    virtual bool IsFullFor(int vc = 0) const;
    virtual int AvailableFor(int vc = 0) const;
    virtual int LimitFor(int vc = 0) const;
  };
    
  class DynamicLimitedSharedBufferPolicy : public LimitedSharedBufferPolicy {
  public:
    DynamicLimitedSharedBufferPolicy(Configuration const & config, 
				     BufferState * parent,
				     const string & name);
    virtual void TakeBuffer(int vc = 0);
    virtual void SendingFlit(Flit const * const f);
  };
  
  class ShiftingDynamicLimitedSharedBufferPolicy : public DynamicLimitedSharedBufferPolicy {
  public:
    ShiftingDynamicLimitedSharedBufferPolicy(Configuration const & config, 
					     BufferState * parent,
					     const string & name);
    virtual void TakeBuffer(int vc = 0);
    virtual void SendingFlit(Flit const * const f);
  };
  
  class FeedbackSharedBufferPolicy : public SharedBufferPolicy {
  protected:
    int _ComputeRTT(int vc, int last_rtt) const;
    int _ComputeLimit(int rtt) const;
    int _ComputeMaxSlots(int vc) const;
    int _vcs;
    vector<int> _occupancy_limit;
    vector<int> _round_trip_time;
    vector<queue<int> > _flit_sent_time;
    int _min_latency;
    int _total_mapped_size;
    int _aging_scale;
    int _offset;
  public:
    FeedbackSharedBufferPolicy(Configuration const & config, 
			       BufferState * parent, const string & name);
    virtual void SetMinLatency(int min_latency);
    virtual void SendingFlit(Flit const * const f);
    virtual void FreeSlotFor(int vc = 0);
    virtual bool IsFullFor(int vc = 0) const;
    virtual int AvailableFor(int vc = 0) const;
    virtual int LimitFor(int vc = 0) const;
  };
  
  class SimpleFeedbackSharedBufferPolicy : public FeedbackSharedBufferPolicy {
  protected:
    vector<int> _pending_credits;
  public:
    SimpleFeedbackSharedBufferPolicy(Configuration const & config, 
				     BufferState * parent, const string & name);
    virtual void SendingFlit(Flit const * const f);
    virtual void FreeSlotFor(int vc = 0);
  };
  
  bool _wait_for_tail_credit;
  int  _size;
  int  _occupancy;
  vector<int> _vc_occupancy;
  int  _vcs;



  BufferPolicy * _buffer_policy;
  
  vector<int> _in_use_by;
  vector<bool> _tail_sent;
  vector<int> _last_id;
  vector<int> _last_pid;

#ifdef TRACK_BUFFERS
  int _classes;
  vector<queue<int> > _outstanding_classes;
  vector<int> _class_occupancy;
#endif

public:

//_vcs包含了duty buffer，这里用DutyVC表示duty buffer，也是最后一条vc
    int dutyVC;

//buffer 状态
    enum _states{idle, active, sleeping, wakingup};
    _states _state;

//持续时间
    int _wakingup_time;
    int _idle_time;
    int const WAKINGUP = 10;
    int const IDLEDETECT = 10;

  BufferState( const Configuration& config, 
	       Module *parent, const string& name );

  ~BufferState();

  inline void SetMinLatency(int min_latency) {
    _buffer_policy->SetMinLatency(min_latency);
  }

  void ProcessCredit( Credit const * const c );
  void SendingFlit( Flit const * const f );

  void TakeBuffer( int vc = 0, int tag = 0 );

  inline bool IsFull() const {
    assert(_occupancy <= _size);
    return (_occupancy == _size);
  }
  inline bool IsFullFor( int vc = 0 ) const {
    return _buffer_policy->IsFullFor(vc);
  }
  inline int AvailableFor( int vc = 0 ) const {
    return _buffer_policy->AvailableFor(vc);
  }
  inline int LimitFor( int vc = 0 ) const {
    return _buffer_policy->LimitFor(vc);
  }
  inline bool IsEmptyFor(int vc = 0) const {
    assert((vc >= 0) && (vc < _vcs));
    return (_vc_occupancy[vc] == 0);
  }
  inline bool IsAvailableFor( int vc = 0 ) const {
    assert( ( vc >= 0 ) && ( vc < _vcs ) );
    return _in_use_by[vc] < 0;
  }
  inline int UsedBy(int vc = 0) const {
    assert( ( vc >= 0 ) && ( vc < _vcs ) );
    return _in_use_by[vc];
  }
    
  inline int Occupancy() const {
    return _occupancy;
  }

  inline int OccupancyFor( int vc = 0 ) const {
    assert((vc >= 0) && (vc < _vcs));
    return _vc_occupancy[vc];
  }

//buffer state get&set
  inline _states GetState() {
        return _state;
    }
  inline void SetState(_states s) {
        _state = s;
  }
//DB get&set
    inline int GetDutyVC() const{
        return dutyVC;
    }
    inline void SetDutyVC(int size) {
        dutyVC = size;
    }
//waking time
    inline const int GetWakingTimeout() {
        return WAKINGUP;
    }
    inline int GetWakingTime() {
        return _wakingup_time;
    }
    inline void SetWakingTime(int wakingtime) {
        _wakingup_time = wakingtime;
    }
    inline void AddWakingTime() {
        ++ _wakingup_time;
    }
//idle time
    inline const int GetIdleTimeout() {
        return IDLEDETECT;
    }
    inline int GetIdleTime() {
        return _idle_time;
    }
    inline void SetIdleTime(int idletime) {
        _idle_time = idletime;
    }
    inline void AddIdleTime() {
        ++ _idle_time;
    }

    /**
 * buffer state changed without flit
 * @param id
 * @param subnet
 * @param nextBuf
 */
    inline void nextBufWithoutHeadFlit(){
        if(this->GetState() == BufferState::idle){
            this->AddIdleTime();
            if(this->GetIdleTime() >= this->GetIdleTimeout()){
                this->SetState(BufferState::sleeping);
                this->SetIdleTime(0);
            }
        }
        if(this->GetState() == BufferState::wakingup){
            this->AddWakingTime();
            if(this->GetWakingTime() >= this->GetWakingTimeout()){
                this->SetState(BufferState::active);
                this->SetWakingTime(0);
            }
        }
    }

/**
 * buffer state changed with flit
 * @param nextBuf
 * @param vc
 * @return
 */
    inline int nextBufWithHeadFlit(int vc){
        if(this->GetState() == BufferState::idle){
            this->SetState(BufferState::active);
            this->SetIdleTime(0);
        }
        if(this->GetState() == BufferState::sleeping){
            this->SetState(BufferState::wakingup);
            vc = this->GetDutyVC();
        }
        if(this->GetState() == BufferState::wakingup){
            this->AddWakingTime();
            vc = this->GetDutyVC();
            if(this->GetWakingTime() >= this->GetWakingTimeout()){
                this->SetState(BufferState::active);
                this->SetWakingTime(0);
            }
        }
        return vc;
    }

#ifdef TRACK_BUFFERS
  inline int OccupancyForClass(int c) const {
    assert((c >= 0) && (c < _classes));
    return _class_occupancy[c];
  }
#endif

  void Display( ostream & os = cout ) const;
};

#endif 
