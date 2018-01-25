// $Id$

/*
  Copyright (c) 2007-2009, Trustees of The Leland Stanford Junior University
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list
  of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this 
  list of conditions and the following disclaimer in the documentation and/or 
  other materials provided with the distribution.
  Neither the name of the Stanford University nor the names of its contributors 
  may be used to endorse or promote products derived from this software without 
  specific prior written permission.

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

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <assert.h>

#include "globals.hpp"
#include "random_utils.hpp"
#include "vc.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"
#include "buffer_state.hpp"
#include "pipefifo.hpp"
#include "allocator.hpp"
#include "iq_router_baseline.hpp"
#include "ringSlot.hpp"

IQRouterBaseline::IQRouterBaseline( const Configuration& config,
				    Module *parent, const string & name, int id,
				    int inputs, int outputs )
  : IQRouterBase( config, parent, name, id, inputs, outputs )
{
  string alloc_type;
  string arb_type;
  int iters;

  slot = new slot_states[outputs-1];
  for(int i=0;i<outputs-1;i++)
	  slot[i] = WAIT;

  portDir = new int [_outputs-gEjPorts];
  nodeID = new int [_outputs-gEjPorts];
  ringID = new int [_outputs-gEjPorts];
  for(int i=0;i<_outputs-gEjPorts;i++) {
	  portDir[i] = i;
	  ringID[i] = -1;
	  nodeID[i] = -1;
  }


	if (_id%gK == gK-1) {
		portDir[E0] = 2;
		portDir[E1] = 7;
	}
	if(_id%gK == 0) {
		portDir[W0] = 3;
		portDir[W1] = 6;
	}
	if(_id/gK == gK-1) {
		portDir[N0] = 0;
		portDir[N1] = 5;
	}
	if(_id/gK == 0) {
		portDir[S0] = 1;
		portDir[S1] = 4;
	}
	if (_id%gK == gK-1) {
		portDir[N0] = -1;
		portDir[S1] = -1;
	}
	if(_id%gK == 0) {
		portDir[S0] = -1;
		portDir[N1] = -1;
	}
	if(_id/gK == gK-1) {
		portDir[E0] = -1;
		portDir[W1] = -1;
	}
	if(_id/gK == 0) {
		portDir[W0] = -1;
		portDir[E1] = -1;
	}

  for(int j=0;j<_outputs-gEjPorts;j++) {

	int i = portDir[j];

	  if(i==-1)
		  continue;

//	  j = i;
//  if(portDir[i]!=i) {
//	  switch(i%4) {
//		  case 0: if(_id%gK == gK-1)
//			  continue;
//		  break;
//		  case 1: if(_id%gK == 0)
//			  continue;
//		  break;
//		  case 2: if(_id/gK == gK-1)
//			  continue;
//		  break;
//		  case 3: if(_id/gK == 0)
//			  continue;
//		  break;
//		  default:
//		  assert(false);
//	  }
//		  j = portDir[i];
//	  }

	  ringID[i] = dimRings*(j/2);

	  switch(j%(2*gDim)) {
		  case 0: nodeID[i] = _id%gK;
					 ringID[i] += (_id/gK);//dimRings);
					 break;
		  case 1: nodeID[i] = gK+((gK-1)-_id%gK);
					 ringID[i] += (_id/gK)-1;//dimRings)-1;
					 break;
		  case 2: nodeID[i] = _id/gK;
					 ringID[i] += (_id%gK);//dimRings);
					 break;
		  case 3: nodeID[i] = gK+((gK-1)-_id/gK);
					 ringID[i] += (_id%gK)-1;//dimRings)-1;
					 break;
		  case 4: nodeID[i] = gK+(_id%gK);
					 ringID[i] += (_id/gK)-1;//dimRings)-1;
					 break;
		  case 5: nodeID[i] = (gK-1)-_id%gK;
					 ringID[i] += (_id/gK);//dimRings);
					 break;
		  case 6: nodeID[i] = gK+_id/gK;
					 ringID[i] += (_id%gK)-1;//dimRings)-1;
					 break;
		  case 7: nodeID[i] = (gK-1)-_id/gK;
					 ringID[i] += (_id%gK);//dimRings);
					 break;
		  default:assert(false);
	  }

  }
//  cout << _id << ":" ;
//  for(int i=0;i<_outputs-gEjPorts;i++) {
//	  cout << " | (" << nodeID[i] << "," <<ringID[i] << ")" ; 
//  }
//  cout << endl;
 
//  cout << "Node: " << _id << endl << "=========" << endl;
//  for(int i=0;i<_outputs-gEjPorts;i++) {
//	  cout << "Port:" << i << " nodeID:" << nodeID[i] << " ringID:" << ringID[i] << endl;
//  }
//  cout << "=========" << endl;

	nc=0;
  sc=0;
  _wait_after_slots = config.GetInt("wait_after_slots");
//  cout << "wait_after: " << _wait_after_slots << endl;


  reservedOutput = new bool * [_outputs-gEjPorts];
  waitedForOutput = new int * [_outputs-gEjPorts];
  for(int i=0;i<_outputs-gEjPorts;i++) {
	  reservedOutput[i] = new bool [3];
	  waitedForOutput[i] = new int [3];
	  for(int j=0;j<3;j++) {
		  reservedOutput[i][j] = false;
		  waitedForOutput[i][j] = 0;
	  }
  }

  // starvation monitor
  strv_mon = new struct starvation [_outputs-gEjPorts];

  // Alloc allocators
  config.GetStr( "vc_allocator", alloc_type );
  config.GetStr( "vc_alloc_arb_type", arb_type );
  iters = config.GetInt( "vc_alloc_iters" );
  if(iters == 0) iters = config.GetInt("alloc_iters");
  _vc_allocator = Allocator::NewAllocator( this, "vc_allocator",
					   alloc_type,
					   _vcs*_inputs,
					   _vcs*_outputs,
					   iters, arb_type );

  if ( !_vc_allocator ) {
    cout << "ERROR: Unknown vc_allocator type " << alloc_type << endl;
    exit(-1);
  }

  config.GetStr( "sw_allocator", alloc_type );
  config.GetStr( "sw_alloc_arb_type", arb_type );
  iters = config.GetInt("sw_alloc_iters");
  if(iters == 0) iters = config.GetInt("alloc_iters");
  _sw_allocator = Allocator::NewAllocator( this, "sw_allocator",
					   alloc_type,
					   _vcs*_inputs*_input_speedup, 
					   _outputs*_output_speedup,
					   iters, arb_type );

  if ( !_sw_allocator ) {
    cout << "ERROR: Unknown sw_allocator type " << alloc_type << endl;
    exit(-1);
  }
  
  _speculative = config.GetInt( "speculative" ) ;
  
  if ( _speculative >= 2 ) {
    
    string filter_spec_grants;
    config.GetStr("filter_spec_grants", filter_spec_grants);
    if(filter_spec_grants == "any_nonspec_gnts") {
      _filter_spec_grants = 0;
    } else if(filter_spec_grants == "confl_nonspec_reqs") {
      _filter_spec_grants = 1;
    } else if(filter_spec_grants == "confl_nonspec_gnts") {
      _filter_spec_grants = 2;
    } else assert(false);
    
    _spec_sw_allocator = Allocator::NewAllocator( this, "spec_sw_allocator",
						  alloc_type,
						  _inputs*_input_speedup, 
						  _outputs*_output_speedup,
						  iters, arb_type );
    if ( !_spec_sw_allocator ) {
      cout << "ERROR: Unknown sw_allocator type " << alloc_type << endl;
      exit(-1);
    }

  }

  _sw_rr_offset.resize(_inputs*_input_speedup);
  
}

IQRouterBaseline::~IQRouterBaseline( )
{
  delete _vc_allocator;
  delete _sw_allocator;

  if ( _speculative >= 2 )
    delete _spec_sw_allocator;
}
  
void IQRouterBaseline::_Alloc( )
{
  _VCAlloc( );
  _SWAlloc( );
}

void IQRouterBaseline::_VCAlloc( )
{
	VC          *cur_vc;
	BufferState *dest_vc;
	int         input_and_vc;
	int         match_input;
	int         match_vc;

	Flit        *f;
	bool        watched = false;


	bool * incremented = new bool [_outputs-gEjPorts];
	for(int i=0;i<_outputs-gEjPorts;i++) {
		incremented[i]=false;
	}



	for(int i=0;i<_outputs-gEjPorts;i++) {
//		if(portDir[i] != -1) {
		if(ringID[i] != -1) {
//			slot[portDir[i]] = ringSet[ringID[i]]->getState(nodeID[i]);
			slot[i] = ringSet[ringID[i]]->getState(nodeID[i]);
			if(false) {
			cout << GetSimTime() << " | " << _id << " | " ;
				switch (slot[i]) {
					case SHORT: cout << i << ": SHORT" ;
									break;
					case LONG: cout << i << ": LONG" ;
								  break;
					case CONST: cout<< i << ": CONST" ;
								  break;
					case WAIT: cout<< i << ": WAIT" ;
								  break;
				}
			cout <<endl;
			}
		}
	}


	_vc_allocator->Clear( );

	if(false)
		if(!_vcalloc_vcs.empty()) {
			cout << GetSimTime() << " | " << FullName() << " | ";
			for(int i=0;i<_outputs-gEjPorts;i++) {
				switch (slot[i]) {
					case SHORT: cout << i << ": SHORT" ;
									break;
					case LONG: cout << i << ": LONG" ;
								  break;
					case CONST: cout<< i << ": CONST" ;
								  break;
					case WAIT: cout<< i << ": WAIT" ;
								  break;
				}
				cout << " | " ;
			}
			cout <<endl;
		}

	for ( set<int>::iterator item = _vcalloc_vcs.begin(); item!=_vcalloc_vcs.end(); ++item ) {
		int vc_encode = *item;
		int input =  vc_encode/_vcs;
		int vc =vc_encode%_vcs;
		bool rs_vc=false;
	  if(input != _inputs-1)	
		  rs_vc= _vcs==2?(vc==1?true:false):(vc%2==1?true:false);
		
	  cur_vc = _vc[input][vc];
		if ( ( _speculative > 0 ) && ( cur_vc->GetState( ) == VC::vc_alloc )){
			cur_vc->SetState( VC::vc_spec ) ;
		}

		if (  cur_vc->GetStateTime( ) >= _vc_alloc_delay  ) {
			f = cur_vc->FrontFlit( );

			const OutputSet *route_set    = cur_vc->GetRouteSet( );
			int out_priority = cur_vc->GetPriority( );
			const list<OutputSet::sSetElement>* setlist = route_set ->GetSetList();
			//cout<<setlist->size()<<endl;
			list<OutputSet::sSetElement>::const_iterator iset = setlist->begin( );
			while(iset!=setlist->end( )){

				// Do allocation based on slot start on each ring.
				// Ejection port can be allocated anytime
				// DO IT ONLY FOR INJECTION AND RING SWITCHING
			if(iset->output_port < (_outputs-gEjPorts)  && (input==_inputs-1 || f->_switch_ring_now)) {

				// trying out a way to prevent deadlock with single buffer
				// Everyone will wait whne a const slot arrives, or else it can send
				if(slot[iset->output_port]==CONST) {
						iset++;
						continue;
				}
				
//				if(input == _inputs-1 || rs_vc) {
					// if reserved cannot inject
					if( !( ((f->type==Flit::READ_REQUEST || f->type==Flit::WRITE_REPLY) && slot[iset->output_port]==SHORT )
						|| ((f->type==Flit::READ_REPLY || f->type==Flit::WRITE_REQUEST ) && slot[iset->output_port]==LONG)
						||	 (f->type==Flit::ANY_TYPE && slot[iset->output_port]==CONST)) )
					{
						int o = iset->output_port;
//						if(slot[o] )
//						if(f->watch) {
//							if(ringSet[ringID[o]]->isReserved(nodeID[o],slot[o])==-1)
//								*gWatchOut << GetSimTime() << " | " << FullName() << " | "
//									<< "Slot :" << slot[iset->output_port] << " for output " << iset->output_port 
//									<< " is unavailable." << endl;
//							else
//								*gWatchOut << GetSimTime() << " | " << FullName() << " | "
//									<< "Slot :" << slot[iset->output_port] << " at node :" << nodeID[o] << " for output " << iset->output_port 
//									<< " is reserved by :" << ringSet[ringID[o]]->isReserved(nodeID[o],slot[o]) << endl;
//						}
//						iset++;
//						continue;
					}
				}
//				}
				else if(iset->output_port >= (_outputs-gEjPorts)) {
					int o = bi_ring_forward(_id,input);
					if( !( ((f->type==Flit::READ_REQUEST || f->type==Flit::WRITE_REPLY) && slot[o]==SHORT )
						|| ((f->type==Flit::READ_REPLY || f->type==Flit::WRITE_REQUEST ) && slot[o]==LONG)
						||	 (f->type==Flit::ANY_TYPE && slot[o]==CONST)) )
					{
//						iset++;
//						continue;
					}

				// you can forward on a reserved slot
//				else {
//					if( !( ((f->type==Flit::READ_REQUEST || f->type==Flit::WRITE_REPLY) 
//							&& (slot[iset->output_port]==SHORT || slot[iset->output_port]==R_SHORT))
//						|| ((f->type==Flit::READ_REPLY || f->type==Flit::WRITE_REQUEST )
//						  	&& (slot[iset->output_port]==LONG || slot[iset->output_port]==R_LONG))
//						||	 (f->type==Flit::ANY_TYPE && (slot[iset->output_port]==CONST || slot[iset->output_port]==R_CONST))) )
//					{
//						iset++;
//						continue;
//					}
//				}	
			}

				if(f->watch) {
					*gWatchOut << GetSimTime() << " | " << FullName() << " | " 
						<< "VC " << vc << " at input " << input
						<< " is requesting VC allocation for flit " << f->id
						<< "." << endl;
					watched = true;
				}

						// increment starvation counter
						if(input == _inputs-1  || rs_vc) {
							assert(iset->output_port < (_outputs-gEjPorts));
//							if(!incremented[iset->output_port]) {
//								(waitedForOutput[iset->output_port][slot[iset->output_port]]) ++;
//								incremented[iset->output_port] = true;
//								cout << GetSimTime() << " | " << _id << " | starvation counter for output:" << iset->output_port
//									<< " slot:" << slot[iset->output_port] << " nodeID:" << nodeID[iset->output_port] 
//									<< " ringID" << ringID[iset->output_port] 
//									<< " = " << (waitedForOutput[iset->output_port][slot[iset->output_port]])
//									<< endl;
//							}
						}

				BufferState *dest_vc = _next_vcs[iset->output_port];

				for ( int out_vc = iset->vc_start; out_vc <= iset->vc_end; ++out_vc ) {
					int in_priority = iset->pri;
					// On the input input side, a VC might request several output 
					// VCs.  These VCs can be prioritized by the routing function
					// and this is reflected in "in_priority".  On the output,
					// if multiple VCs are requesting the same output VC, the priority
					// of VCs is based on the actual packet priorities, which is
					// reflected in "out_priority".

					//	    cout<<
					if(dest_vc->IsAvailableFor(out_vc)
							// this check is needed to make sure that VC alloc only happens if buffer is available
							// this is needed in multiring scenario to avoid deadlock
							&& !dest_vc->IsFullFor(out_vc) ) {

						// starvation
//						if(input == _inputs-1)
//							strv_mon[iset->output_port].starv_count ++;


						if(f->watch){
							*gWatchOut << GetSimTime() << " | " << FullName() << " | "
								<< "Requesting VC " << out_vc
								<< " at output " << iset->output_port 
								<< " with priorities " << in_priority
								<< " and " << out_priority
								<< "." << endl;
						}
						if(f->_switch_ring_now)
							_vc_allocator->AddRequest(input*_vcs + vc, iset->output_port*_vcs + out_vc, 
									out_vc, in_priority, -1);
						else {
//							if(strv_mon[iset->output_port].hold_credit)
//								_vc_allocator->AddRequest(input*_vcs + vc, iset->output_port*_vcs + out_vc, 
//									out_vc, in_priority, input==(2*gDim)?1:out_priority);
//							else
								_vc_allocator->AddRequest(input*_vcs + vc, iset->output_port*_vcs + out_vc, 
									out_vc, in_priority, input==(2*gDim)?-2:out_priority);
						}
					} else {
						if(f->watch)
							*gWatchOut << GetSimTime() << " | " << FullName() << " | "
								<< "VC " << out_vc << " at output " << iset->output_port 
								<< " is unavailable." << endl;
					}
				}
				//go to the next item in the outputset
				iset++;
			}
		}
	}
	//  watched = true;
	if ( watched ) {
		*gWatchOut << GetSimTime() << " | " << _vc_allocator->FullName() << " | ";
		_vc_allocator->PrintRequests( gWatchOut );
	}

	_vc_allocator->Allocate( );

	// Winning flits get a VC

	for ( int output = 0; output < _outputs; ++output ) {
		for ( int vc = 0; vc < _vcs; ++vc ) {
			input_and_vc = _vc_allocator->InputAssigned( output*_vcs + vc );

			if ( input_and_vc != -1 ) {
				assert(input_and_vc >= 0);
				match_input = input_and_vc / _vcs;
				match_vc    = input_and_vc - match_input*_vcs;

				cur_vc  = _vc[match_input][match_vc];
				dest_vc = _next_vcs[output];

				if ( _speculative > 0 )
					cur_vc->SetState( VC::vc_spec_grant );
				else
					cur_vc->SetState( VC::active );
				_vcalloc_vcs.erase(match_input*_vcs+match_vc);

				cur_vc->SetOutput( output, vc );
				dest_vc->TakeBuffer( vc );

				f = cur_vc->FrontFlit( );

				if(f->watch)
					*gWatchOut << GetSimTime() << " | " << FullName() << " | "
						<< "Granted VC " << vc << " at output " << output
						<< " to VC " << match_vc << " at input " << match_input
						<< " (flit: " << f->id << " dest: " << f->dest << ")." << endl;


				if(match_input == _inputs-1) {
					if(output%gDim == 0 || output%gDim == 1)
						f->x0_y1 = 0;
					else if(output%gDim == 2 || output%gDim == 3)
						f->x0_y1 = 1;
					else
						assert(false);
				}

				bool rs_vc=false;
			  if(match_input != _inputs-1)	
				  rs_vc= _vcs==2?(match_vc==1?true:false):(match_vc%2==1?true:false);

//				if( (match_input == _inputs-1 || rs_vc) && slot[output]<=CONST) { 
//					waitedForOutput[output][slot[output]] = 0;
//					if(reservedOutput[output][slot[output]]) {
//						reservedOutput[output][slot[output]] = false;
//						ringSet[ringID[output]]->freeReservedSlot(nodeID[output],slot[output]);
//						cout << GetSimTime() << " | " << _id << " | freeing reservation for output:" << output << " slot:" << slot[output] << " nodeID:" << nodeID[output] << endl;
//					}
//				}

//			// starvation
//			if(match_input == _inputs-1) {
//				if(strv_mon[output].hold_credit) {
//					strv_mon[output].done = true;
//					//cout << GetSimTime() << " | " << _id << " | Done halting. Release credits for output:" << output << endl;
//				}
//				else {
//					strv_mon[output].reset();
//					//cout << GetSimTime() << " | " << _id << " | Reset starvation counter for output:" << output << endl;
//				}
//			}

				// reset once flit routed and granted VC on destination ring
				if(f->_switch_ring_now) {
					f->_switch_ring_now = false;
					assert(f->rs_time != -1);
					f->rs_time = GetSimTime() - f->rs_time;
				}

			}
		}
	}

//	for(int i=0;i<_outputs-gEjPorts;i++) {
//		if(strv_mon[i].starv_count >= _wait_after_slots) {
//			if(_complex_router && !strv_mon[i].done) {
				//cout << GetSimTime() << " | " << _id << " | Halting credits for output:" << i << endl;
				//strv_mon[i].hold_credit = true;
//			}
//		}
//	}
	
	for(int i=0;i<_outputs-gEjPorts;i++) {
//		if(slot[i] <= CONST)
//		if(waitedForOutput[i][slot[i]] > _wait_after_slots)
//			if(!reservedOutput[i][slot[i]]) {
//				reservedOutput[i][slot[i]] = ringSet[ringID[i]]->reserveSlot(nodeID[i],slot[i]);
//				cout << GetSimTime() << " | " << _id << " | requesting reservation for output:" << i << " slot:" << slot[i] << " nodeID:" << nodeID[i] << endl;
//				if(reservedOutput[i][slot[i]]) 
//					cout << GetSimTime() << " | " << _id << " | Reserved for output:" << i << " slot:" << slot[i] << " nodeID:" << nodeID[i] << endl;
//			}
	}

}

void IQRouterBaseline::_SWAlloc( )
{
	Flit        *f;
	Credit      *c;

	VC          *cur_vc;
	BufferState *dest_vc;

	int input;
	int output;
	int vc;

	int expanded_input;
	int expanded_output;

	bool        watched = false;

	bool any_nonspec_reqs = false;
	bool any_nonspec_output_reqs[_outputs*_output_speedup];
	memset(any_nonspec_output_reqs, 0, _outputs*_output_speedup*sizeof(bool));

	_sw_allocator->Clear( );
	if ( _speculative >= 2 )
		_spec_sw_allocator->Clear( );

	for ( input = 0; input < _inputs; ++input ) {
		int vc_ready_nonspec = 0;
		int vc_ready_spec = 0;
		for ( int s = 0; s < _input_speedup; ++s ) {
			expanded_input  = s*_inputs + input;

			// Arbitrate (round-robin) between multiple 
			// requesting VCs at the same input (handles 
			// the case when multiple VC's are requesting
			// the same output port)
			vc = _sw_rr_offset[ expanded_input ];

			for ( int v = 0; v < _vcs; ++v ) {

				// This continue acounts for the interleaving of 
				// VCs when input speedup is used
				// dub: Essentially, this skips loop iterations corresponding to those 
				// VCs not in the current speedup set. The skipped iterations will be 
				// handled in a different iteration of the enclosing loop over 's'.
				if ( ( vc % _input_speedup ) != s ) {
					vc = ( vc + 1 ) % _vcs;
					continue;
				}


				cur_vc = _vc[input][vc];

				if(!cur_vc->Empty() &&
						(cur_vc->GetStateTime() >= _sw_alloc_delay)) {

					switch(cur_vc->GetState()) {

						case VC::active:
							{
								output = cur_vc->GetOutputPort( );

								dest_vc = _next_vcs[output];

								if ( !dest_vc->IsFullFor( cur_vc->GetOutputVC( ) ) ) {

									// When input_speedup > 1, the virtual channel buffers are 
									// interleaved to create multiple input ports to the switch. 
									// Similarily, the output ports are interleaved based on their 
									// originating input when output_speedup > 1.

									assert( expanded_input == (vc%_input_speedup)*_inputs + input );
									expanded_output = 
										(input%_output_speedup)*_outputs + output;

									if(_switch_hold_out[expanded_output] != -1) {
										int check_input_vc = _switch_hold_out[expanded_output];
										int check_input = check_input_vc/_vcs;
										int check_vc;// = check_input_vc%_vcs;

										//										assert(check_vc == _switch_hold_vc[check_input_vc]);
										check_vc = _switch_hold_vc[check_input_vc];

										if(dest_vc->IsFullFor( (_vc[check_input][check_vc])->GetOutputVC() ) ) {
											_switch_hold_out[expanded_output] = -1;
											_switch_hold_in[check_input_vc] = -1;
											_switch_hold_vc[check_input_vc] = -1;
										}
									}

									int expanded_input_vc;
									//									if(input != _inputs-1) {
									if( ( (_vcs == 4 && vc%2 == 0) || (_vcs == 2 && vc < _vcs/2) ) ) // normal ring forwarding VC
									{
										expanded_input_vc = (expanded_input*_vcs);
									}
									else // dimension switching vc
									{
										//											assert (_complex_router);
										//										if(_id%gK == 0)
										//											assert (input == 2 || input == 0);
										//										else if(_id%gK == gK-1)
										//											assert (input == 3 || input == 1);
										//										else if (_id / gK == 0)
										//											assert (input == 1);
										//										else if(_id / gK == gK-1)
										//											assert(input == 0);
										//										else 
										//											assert(input == 0 || input == 1);

										expanded_input_vc = (expanded_input*_vcs)+1;
									}
									//									}
									//									else {
									//										expanded_input_vc = (expanded_input*_vcs);
									//									}

									// check for competing input ports as well.
									// see if it is blocked
									// currently only happens for injection port
									// might give problems in REQ REP traffic
									//								if(_switch_hold_in[expanded_input_vc] != -1) {
									//									int check_vc = _switch_hold_vc[expanded_input_vc];
									//									if(vc != check_vc) {
									//										VC * c_cur_vc = _vc[input][check_vc];
									//										int c_output = c_cur_vc->GetOutputPort( );
									//										BufferState *c_dest_vc = _next_vcs[c_output];
									//										if(c_dest_vc->IsFullFor( (_vc[input][check_vc])->GetOutputVC() ) ) {
									//											_switch_hold_out[c_output] = -1;
									//											_switch_hold_in[expanded_input_vc] = -1;
									//											_switch_hold_vc[expanded_input_vc] = -1;
									//										}
									//									}
									//								
									//								}


									if ( ( _switch_hold_in[expanded_input_vc] == -1 ) && 
											( _switch_hold_out[expanded_output] == -1 ) ) {
										//				if(_id==30 && GetSimTime() > 2490 && input == _inputs-1 && vc == 1) {
										//					cout << " Yes switch is not held by anyone." << endl;
										//				}

										// We could have requested this same input-output pair in a 
										// previous iteration; only replace the previous request if 
										// the current request has a higher priority (this is default 
										// behavior of the allocators).  Switch allocation priorities 
										// are strictly determined by the packet priorities.

										Flit * f = cur_vc->FrontFlit();
										assert(f);
										if(f->watch) {
											*gWatchOut << GetSimTime() << " | " << FullName() << " | "
												<< "VC " << vc << " at input " << input 
												<< " requested output " << output 
												<< " (non-spec., exp. input: " << expanded_input
												<< ", exp. output: " << expanded_output
												<< ", flit: " << f->id
												<< ", prio: " << cur_vc->GetPriority()
												<< ")." << endl;
											watched = true;
										}

										// dub: for the old-style speculation implementation, we 
										// overload the packet priorities to prioritize 
										// non-speculative requests over speculative ones
										if( _speculative == 1 )
											_sw_allocator->AddRequest(expanded_input_vc, expanded_output, 
													vc, 1, 1);
										else
											_sw_allocator->AddRequest(expanded_input_vc, expanded_output, 
													vc, cur_vc->GetPriority( ), 
													cur_vc->GetPriority( ));
										any_nonspec_reqs = true;
										any_nonspec_output_reqs[expanded_output] = true;
										vc_ready_nonspec++;
									}

									//				if(_id==30 && GetSimTime() > 2490 && input == _inputs-1 && vc == 1) {
									//					cout << " Yes switch is not held by anyone." << endl;
									//							if ( ( _switch_hold_in[expanded_input_vc] != -1 ) )
									//								cout << "in:" << expanded_input_vc << " is holding output: " << _switch_hold_in[expanded_input_vc] << endl;
									//							if		( _switch_hold_out[expanded_output] != -1 ) 
									//								cout << "out:" << expanded_output << " is held by: " << _switch_hold_out[expanded_output] << endl;
									//				}
								}
							}
							break;
					}
				}
				vc = ( vc + 1 ) % _vcs;
			}
		}
	}

	if(watched) {
		*gWatchOut << GetSimTime() << " | " << _sw_allocator->FullName() << " | ";
		_sw_allocator->PrintRequests( gWatchOut );
		if(_speculative >= 2) {
			*gWatchOut << GetSimTime() << " | " << _spec_sw_allocator->FullName() << " | ";
			_spec_sw_allocator->PrintRequests( gWatchOut );
		}
	}

	_sw_allocator->Allocate();
	if(_speculative >= 2)
		_spec_sw_allocator->Allocate();

	// Winning flits cross the switch

	_crossbar_pipe->WriteAll( 0 );

	//////////////////////////////
	// Switch Power Modelling
	//  - Record Total Cycles
	//
	switchMonitor.cycle() ;

	for ( int input = 0; input < _inputs; ++input ) {
		c = 0;

		int vc_grant_nonspec = 0;
		int vc_grant_spec = 0;

		for ( int s = 0; s < _input_speedup; ++s ) {

			bool use_spec_grant = false;

			expanded_input  = s*_inputs + input;

			for(int vc_i=0;vc_i<_vcs;vc_i++) {

				int expanded_input_vc = (expanded_input*_vcs)+vc_i;



				if ( _switch_hold_in[expanded_input_vc] != -1 ) {
					assert(_switch_hold_in[expanded_input_vc] >= 0);
					expanded_output = _switch_hold_in[expanded_input_vc];
					vc = _switch_hold_vc[expanded_input_vc];
					assert(vc >= 0);
					cur_vc = _vc[input][vc];

					if ( cur_vc->Empty( ) ) { // Cancel held match if VC is empty
						expanded_output = -1;
					}
				} else {
					expanded_output = _sw_allocator->OutputAssigned( expanded_input_vc );
					if ( ( _speculative >= 2 ) && ( expanded_output < 0 ) ) {
						expanded_output = _spec_sw_allocator->OutputAssigned(expanded_input);
						if ( expanded_output >= 0 ) {
							assert(_spec_sw_allocator->InputAssigned(expanded_output) >= 0);
							assert(_spec_sw_allocator->ReadRequest(expanded_input, expanded_output) >= 0);
							switch ( _filter_spec_grants ) {
								case 0:
									if ( any_nonspec_reqs )
										expanded_output = -1;
									break;
								case 1:
									if ( any_nonspec_output_reqs[expanded_output] )
										expanded_output = -1;
									break;
								case 2:
									if ( _sw_allocator->InputAssigned(expanded_output) >= 0 )
										expanded_output = -1;
									break;
								default:
									assert(false);
							}
						}
						use_spec_grant = (expanded_output >= 0);
					}
				}

				if ( expanded_output >= 0 ) {
					output = expanded_output % _outputs;

					if ( _switch_hold_in[expanded_input_vc] == -1 ) {
						if(use_spec_grant) {
							assert(_spec_sw_allocator->OutputAssigned(expanded_input) >= 0);
							assert(_spec_sw_allocator->InputAssigned(expanded_output) >= 0);
							vc = _spec_sw_allocator->ReadRequest(expanded_input_vc, 
									expanded_output);
						} else {
							assert(_sw_allocator->OutputAssigned(expanded_input_vc) >= 0);
							assert(_sw_allocator->InputAssigned(expanded_output) >= 0);
							vc = _sw_allocator->ReadRequest(expanded_input_vc, expanded_output);
						}
						assert(vc >= 0);
						cur_vc = _vc[input][vc];
					}

					// Detect speculative switch requests which succeeded when VC 
					// allocation failed and prevenet the switch from forwarding;
					// also, in case the routing function can return multiple outputs, 
					// check to make sure VC allocation and speculative switch allocation 
					// pick the same output port.
					if ( ( ( cur_vc->GetState() == VC::vc_spec_grant ) ||
								( cur_vc->GetState() == VC::active ) ) &&
							( cur_vc->GetOutputPort() == output ) ) {

						if(use_spec_grant) {
							vc_grant_spec++;
						} else {
							vc_grant_nonspec++;
						}

						if ( _hold_switch_for_packet ) {
							_switch_hold_in[expanded_input_vc] = expanded_output;
							_switch_hold_vc[expanded_input_vc] = vc;
							_switch_hold_out[expanded_output] = expanded_input_vc;
						}

						assert((cur_vc->GetState() == VC::vc_spec_grant) ||
								(cur_vc->GetState() == VC::active));
						assert(!cur_vc->Empty());
						assert(cur_vc->GetOutputPort() == output);

						dest_vc = _next_vcs[output];

						if ( dest_vc->IsFullFor( cur_vc->GetOutputVC( ) ) )
							continue ;

						// Forward flit to crossbar and send credit back
						f = cur_vc->RemoveFlit( );
						assert(f);
						if(f->watch) {
							*gWatchOut << GetSimTime() << " | " << FullName() << " | "
								<< "Output " << output
								<< " granted to VC " << vc << " at input " << input;
							if(cur_vc->GetState() == VC::vc_spec_grant)
								*gWatchOut << " (spec";
							else
								*gWatchOut << " (non-spec";
							*gWatchOut << ", exp. input: " << expanded_input
								<< ", exp. output: " << expanded_output
								<< ", flit: " << f->id << ")." << endl;
						}

						f->hops++;

						//
						// Switch Power Modelling
						//
						switchMonitor.traversal( input, output, f) ;
						bufferMonitor.read(input, f) ;

						if(f->watch)
							*gWatchOut << GetSimTime() << " | " << FullName() << " | "
								<< "Forwarding flit " << f->id << " through crossbar "
								<< "(exp. input: " << expanded_input
								<< ", exp. output: " << expanded_output
								<< ")." << endl;


						if( (input==_inputs-1 || output>=(_outputs-gEjPorts)) || ( !(strv_mon[output].hold_credit)) || (_vcs==4?(vc%2==1):(vc==1)) )  {
							if ( !c ) {
								c = _NewCredit( _vcs );
							}

							assert(vc == f->vc);

							c->vc[c->vc_cnt] = f->vc;
							c->vc_cnt++;
							c->dest_router = f->from_router;
						} else {
							cout << GetSimTime() << " | " << _id << " | Not sending credit for input:" << input << endl;
							strv_mon[output].credits_held ++;
						}

						f->vc = cur_vc->GetOutputVC( );
						dest_vc->SendingFlit( f );

						_crossbar_pipe->Write( f, expanded_output );

						if(f->tail) {
							if(cur_vc->Empty()) {
								cur_vc->SetState(VC::idle);
							} else if(_routing_delay > 0) {
								cur_vc->SetState(VC::routing);
								_routing_vcs.push(input*_vcs+vc);
							} else {
								cur_vc->Route(_rf, this, cur_vc->FrontFlit(), input);
								cur_vc->SetState(VC::vc_alloc);
								_vcalloc_vcs.insert(input*_vcs+vc);
							}
							_switch_hold_in[expanded_input_vc]   = -1;
							_switch_hold_vc[expanded_input_vc]   = -1;
							_switch_hold_out[expanded_output] = -1;

						} else {
							// reset state timer for next flit
							cur_vc->SetState(VC::active);
						}

						_sw_rr_offset[expanded_input] = ( vc + 1 ) % _vcs;
					} else {
						assert(cur_vc->GetState() == VC::vc_spec);
						Flit * f = cur_vc->FrontFlit();
						assert(f);
						if(f->watch)
							*gWatchOut << GetSimTime() << " | " << FullName() << " | "
								<< "Speculation failed at output " << output
								<< "(exp. input: " << expanded_input
								<< ", exp. output: " << expanded_output
								<< ", flit: " << f->id << ")." << endl;
					} 
				}
			}
		}

		// Promote all other virtual channel grants marked as speculative to active.
		for ( int vc = 0 ; vc < _vcs ; vc++ ) {
			cur_vc = _vc[input][vc] ;
			if ( cur_vc->GetState() == VC::vc_spec_grant ) {
				cur_vc->SetState( VC::active ) ;	
			} 
		}

		_credit_pipe->Write( c, input );
	}

	// Release all held credits if done
	//for(int i=0;i<_inputs-1;i++) {
	//	int output = ring_forward_i(_id, i);
	//	
	//	if(output == -1)
	//		continue;
	//
	//	if(strv_mon[output].done) {
	//		Credit *c;
	//		c = _credit_pipe->Read( i );
	//		if(c) {
	//			c->NewBunch(0,strv_mon[output].credits_held);
	//		} else {
	//			c = new Credit();
	//			c->NewBunch(0,strv_mon[output].credits_held);
	//			_credit_pipe->Write( c, i );
	//		}
	//		//cout << GetSimTime() << " | " << _id << " | Sending bundled credits for input:" << i << endl;
	//		//cout << GetSimTime() << " | " << _id << " | Sending bundle_vc:" << 0 << "num:" << strv_mon[output].credits_held << " for input:" << i << endl;
	//		strv_mon[output].reset();
	//	}
	//	}

}

starvation :: starvation() {
	hold_credit = false;
	credits_held = 0;
	held_for = 0;
	starv_count = 0;
	done = false;
}

void starvation :: reset() {
	hold_credit = false;
	credits_held = 0;
	held_for = 0;
	starv_count = 0;
	done = false;
}
