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

#ifndef _ROUTER_HPP_
#define _ROUTER_HPP_

#include <string>
#include <vector>

#include "timed_module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "flitchannel.hpp"
#include "channel.hpp"
#include "config_utils.hpp"
#include "buffer_state.hpp"

typedef Channel<Credit> CreditChannel;

class Router : public TimedModule {

protected:

  static int const STALL_BUFFER_BUSY;
  static int const STALL_BUFFER_CONFLICT;
  static int const STALL_BUFFER_FULL;
  static int const STALL_BUFFER_RESERVED;
  static int const STALL_CROSSBAR_CONFLICT;

  int _id;
  
  int _inputs;
  int _outputs;
  
  int _classes;

  int _input_speedup;
  int _output_speedup;
  
  double _internal_speedup;
  double _partial_internal_cycles;

  int _crossbar_delay;
  int _credit_delay;
  
  vector<FlitChannel *>   _input_channels;
  vector<CreditChannel *> _input_credits;
  vector<FlitChannel *>   _output_channels;
  vector<CreditChannel *> _output_credits;
  vector<bool>            _channel_faults;

#ifdef TRACK_FLOWS
  vector<vector<int> > _received_flits;
  vector<vector<int> > _stored_flits;
  vector<vector<int> > _sent_flits;
  vector<vector<int> > _outstanding_credits;
  vector<vector<int> > _active_packets;
#endif

#ifdef TRACK_STALLS
  vector<int> _buffer_busy_stalls;
  vector<int> _buffer_conflict_stalls;
  vector<int> _buffer_full_stalls;
  vector<int> _buffer_reserved_stalls;
  vector<int> _crossbar_conflict_stalls;
#endif

  virtual void _InternalStep(int subnet, TrafficManager* trafficmanager) = 0;

public:
  Router( const Configuration& config,
	  Module *parent, const string & name, int id,
	  int inputs, int outputs );

  static Router *NewRouter( const Configuration& config,
			    Module *parent, const string & name, int id,
			    int inputs, int outputs );

  virtual void AddInputChannel( FlitChannel *channel, CreditChannel *backchannel );
  virtual void AddOutputChannel( FlitChannel *channel, CreditChannel *backchannel );
 
  inline FlitChannel * GetInputChannel( int input ) const {
    assert((input >= 0) && (input < _inputs));
    return _input_channels[input];
  }
  inline FlitChannel * GetOutputChannel( int output ) const {
    assert((output >= 0) && (output < _outputs));
    return _output_channels[output];
  }

  virtual void ReadInputs( ) = 0;
  virtual void Evaluate( int subnet, TrafficManager* trafficmanager);
  virtual void WriteOutputs( ) = 0;
//需要调用子类iqrouter的方法，所以这里声明虚函数，并在子类实现
  virtual void SetNextBufState(int output, BufferState::_states s){}
  virtual BufferState * GetNextBuf(int output){return 0;}

  void OutChannelFault( int c, bool fault = true );
  bool IsFaultyOutput( int c ) const;

  inline int GetID( ) const {return _id;}
//根据路由器当前的输入端口找到上一个路由器的id，不包括PE
  inline int GetLastID(int input){
      int cur_id = this->_id;
        switch(input){
            case 0:
                return rightNode(cur_id, 0);
            case 1:
                return leftNode(cur_id, 0);
            case 2:
                return leftNode(cur_id, 1);
            case 3:
                return rightNode(cur_id, 1);
            default:
                return -1;
        }
    }
//power
    int pow(int k, int dim){
      if(dim == 0)
          return 1;
      else if(dim == 1)
          return k;
      else if(dim % 2 == 1)
          return pow(pow(k, dim / 2), 2) * k;
      else
          return pow(pow(k, dim / 2), 2);
  }
//left node and up node
    inline int leftNode(int node, int dim){
      int k = gK;
      int k_dim = pow(k, dim);
      int dim_location = (node / k_dim) % k;
      if(dim_location == 0)
          return (node + k_dim * (k - 1));
      else
          return (node - k_dim);
  }
//right node and down node
    inline int rightNode(int node, int dim){
        int k = gK;
        int k_dim = pow(k, dim);
        int dim_location = (node / k_dim) % k;
        if(dim_location == k - 1)
            return (node - k_dim * (k - 1));
        else
            return (node + k_dim);
  }
//根据当前输入端口找到上一个路由器的输出端口
  inline int GetLastOutport(int input){
        switch(input){
            case 0:
                return 1;
            case 1:
                return 0;
            case 2:
                return 3;
            case 3:
                return 2;
            default:
                return -1;
        }
    }
  virtual int GetUsedCredit(int o) const = 0;
  virtual int GetBufferOccupancy(int i) const = 0;

#ifdef TRACK_BUFFERS
  virtual int GetUsedCreditForClass(int output, int cl) const = 0;
  virtual int GetBufferOccupancyForClass(int input, int cl) const = 0;
#endif

#ifdef TRACK_FLOWS
  inline vector<int> const & GetReceivedFlits(int c) const {
    assert((c >= 0) && (c < _classes));
    return _received_flits[c];
  }
  inline vector<int> const & GetStoredFlits(int c) const {
    assert((c >= 0) && (c < _classes));
    return _stored_flits[c];
  }
  inline vector<int> const & GetSentFlits(int c) const {
    assert((c >= 0) && (c < _classes));
    return _sent_flits[c];
  }
  inline vector<int> const & GetOutstandingCredits(int c) const {
    assert((c >= 0) && (c < _classes));
    return _outstanding_credits[c];
  }

  inline vector<int> const & GetActivePackets(int c) const {
    assert((c >= 0) && (c < _classes));
    return _active_packets[c];
  }

  inline void ResetFlowStats(int c) {
    assert((c >= 0) && (c < _classes));
    _received_flits[c].assign(_received_flits[c].size(), 0);
    _sent_flits[c].assign(_sent_flits[c].size(), 0);
  }
#endif

  virtual vector<int> UsedCredits() const = 0;
  virtual vector<int> FreeCredits() const = 0;
  virtual vector<int> MaxCredits() const = 0;

#ifdef TRACK_STALLS
  inline int GetBufferBusyStalls(int c) const {
    assert((c >= 0) && (c < _classes));
    return _buffer_busy_stalls[c];
  }
  inline int GetBufferConflictStalls(int c) const {
    assert((c >= 0) && (c < _classes));
    return _buffer_conflict_stalls[c];
  }
  inline int GetBufferFullStalls(int c) const {
    assert((c >= 0) && (c < _classes));
    return _buffer_full_stalls[c];
  }
  inline int GetBufferReservedStalls(int c) const {
    assert((c >= 0) && (c < _classes));
    return _buffer_reserved_stalls[c];
  }
  inline int GetCrossbarConflictStalls(int c) const {
    assert((c >= 0) && (c < _classes));
    return _crossbar_conflict_stalls[c];
  }

  inline void ResetStallStats(int c) {
    assert((c >= 0) && (c < _classes));
    _buffer_busy_stalls[c] = 0;
    _buffer_conflict_stalls[c] = 0;
    _buffer_full_stalls[c] = 0;
    _buffer_reserved_stalls[c] = 0;
    _crossbar_conflict_stalls[c] = 0;
  }
#endif

  inline int NumInputs() const {return _inputs;}
  inline int NumOutputs() const {return _outputs;}
};

#endif
