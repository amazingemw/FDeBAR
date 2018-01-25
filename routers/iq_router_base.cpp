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

#include "iq_router_base.hpp"

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cassert>
#include<math.h>

#include "globals.hpp"
#include "random_utils.hpp"
#include "vc.hpp"
#include "outputset.hpp"
#include "buffer_state.hpp"

IQRouterBase::IQRouterBase( const Configuration& config,
		    Module *parent, const string & name, int id,
		    int inputs, int outputs )
  : Router( config, parent, name, id, inputs, outputs ), 
    bufferMonitor(inputs), 
    switchMonitor(inputs, outputs) 
{
  ostringstream vc_name;
  
  _vcs         = config.GetInt( "num_vcs" );
  _vc_size     = config.GetInt( "vc_buf_size" );

  _routing_delay    = config.GetInt( "routing_delay" );
  _vc_alloc_delay   = config.GetInt( "vc_alloc_delay" );
  _sw_alloc_delay   = config.GetInt( "sw_alloc_delay" );
  
  // Routing

   if(!_bless)
   {

	   _rf = (tRoutingFunction)GetRoutingFunction( config );
   }
   else
   {
	   _rb=(bRoutingFunction)GetRoutingFunction( config );
   }
  // Alloc VC's
  _vc.resize(_inputs);
  for ( int i = 0; i < _inputs; ++i )
  {
	_vc[i].resize(_vcs);
	for (int j = 0; j < _vcs; ++j ) 
	{
	      _vc[i][j] = new VC(config, _outputs);
	      vc_name << "vc_i" << i << "_v" << j;
	      _vc[i][j]->SetName( this, vc_name.str( ) );
	      vc_name.str("");
  	}
  }

  // Alloc next VCs' buffer state
  _next_vcs.resize(_outputs);
  for (int j = 0; j < _outputs; ++j)
 {
    _next_vcs[j] = new BufferState( config );
    vc_name << "next_vc_o" << j;
    _next_vcs[j]->SetName( this, vc_name.str( ) );
    vc_name.str("");
  }

  // Alloc pipelines (to simulate processing/transmission delays)
  _crossbar_pipe = 
    new PipelineFIFO<Flit>( this, "crossbar_pipeline", _outputs*_output_speedup, 
			    _st_prepare_delay + _st_final_delay );

  _credit_pipe =
    new PipelineFIFO<Credit>( this, "credit_pipeline", _inputs,
			      _credit_delay );

  // Input and output queues
  //_input_buffer.resize(_inputs); 
  _output_buffer.resize(_outputs); 

  _in_cred_buffer.resize(_inputs); 
  //_out_cred_buffer.resize(_outputs);

  // Switch configuration (when held for multiple cycles)
  _hold_switch_for_packet = config.GetInt( "hold_switch_for_packet" );
  _switch_hold_in.resize(_inputs*_input_speedup, -1);
  _switch_hold_out.resize(_outputs*_output_speedup, -1);
  _switch_hold_vc.resize(_inputs*_input_speedup, -1);

  _received_flits.resize(_inputs);
  _sent_flits.resize(_outputs);
  ResetFlitStats();
}

IQRouterBase::~IQRouterBase( )
{
  if(_print_activity){
    cout << Name() << ".bufferMonitor:" << endl ; 
    cout << bufferMonitor << endl ;
    
    cout << Name() << ".switchMonitor:" << endl ; 
    cout << "Inputs=" << _inputs ;
    cout << "Outputs=" << _outputs ;
    cout << switchMonitor << endl ;
  }

  for ( int i = 0; i < _inputs; ++i )
    for (int j = 0; j < _vcs; ++j )
      delete _vc[i][j];
  
  for (int j = 0; j < _outputs; ++j)
    delete _next_vcs[j];

  delete _crossbar_pipe;
  delete _credit_pipe;

}


void IQRouterBase::Eject()
{
int Eject_band=2, t=0;
//cout<<"Ejction";

if(_Mod_MinBD == true && eject_flag == 1)	//check for ejection buffer has flit or not, if it has eject it first.
{
//cout<<sideeject_flit->id;getchar();
  (*_output_channels)[4]->Send( sideeject_flit );
  eject_flag = 0;
  t++;
}
   for(int i = 0; i<4; i++)
   {
	 VC * cur_vc=_vc[i][0];
	  if(cur_vc->GetStateTime() >= _routing_delay)
	  {
	    Flit * f = cur_vc->FrontFlit( );

	    if(f)
	    {
	    	 //f->hops++;
	   	 if( f->dest==this->GetID() && t<Eject_band)
	    	 {	
//cout<<"Ejecting:"<<f->id<<" S:"<<f->src<<" D:"<<f->dest<<endl;
			t++;
			f->cur_port=4;

			this->nonempty_vc[i] = false; 
	     
	     		f = cur_vc->RemoveFlit();

			if(_Mod_MinBD == true && t==2 && eject_flag==0 )//If two flits want to eject send second flit to sideeject
			{
//cout<<"side"<<f->id<<endl;
				sideeject_flit = f;
				eject_flag = 1;
			}
			else
			(*_output_channels)[4]->Send( f );
	    	  }   	   
	    }
	 }
   }//for loop

}

void IQRouterBase::Redirect()
{
int i;
VC * cur_vc;

for( i=0;i<4;i++ )
{

cur_vc=_vc[i][0];

if(cur_vc->Empty())
break;
}

if( i==4 && reinject_flag >=c_threshold )
{
//cout<<reinject_flag<<"****************************************";
int rand_vc=RandomInt(3);
cur_vc = _vc[rand_vc][0];
redirect_flag = 1;
redirect_flit = cur_vc->RemoveFlit();	//Redirect_flit contains the flit removed from pipeline, will be added to side buffer after reinjection.
this->nonempty_vc[i] = false; 
}

}

void IQRouterBase::BufferReinject()	//For MinBD
{
int port;
Flit *f;
int i;



if( !sidebuff.empty() )
{
f = sidebuff.front();

  if( f )
  {

	reinject_flag++;	// Redirection checking flag incremented

	int x = this->field.x&15;
	if( x>0)
	{

	   for( i=0;i<4;i++ )
	   {	
//		if( this->nonempty_vc[i] == false )
VC * cur_vc=_vc[i][0];if(cur_vc->Empty())		
		{
		port = i;
		int k = pow(2,port);
		int z = this->use_const;
		z = z&15;
		  if( k==(k&z) && port!=2*gN )
		  {
//cout<<"At "<<this->GetID()<<" :: "<<f->id<<" Reinjected";//getchar();
			//allocate now 
			VC * cur_vc=_vc[i][0];
			++_received_flits[i];	
			
			f->input = i;

			f->port = i;	//current port
//cout<<f->id<<" reinject"<<endl;
			cur_vc->AddFlit(f);
	
			reinject_flag = 0;	//Reinjection successful so flag is clear to zero ( Redirection check flag )
			
			cur_vc->SetActive();

			this->nonempty_vc[i]=true;
			sidebuff.pop();	

//cout<<" Size:"<<sidebuff.size();

			switch(port)
	    		{
	    		case 0:this->field.x = this->field.x&30;break;//13; break;
	    		case 1:this->field.x = this->field.x&29;break;//14; break;
	    		case 2:this->field.x = this->field.x&27;break;//7; break;
	    		case 3:this->field.x = this->field.x&23;break;//11; break;
	    		}
			break;			
		   }

		 }
	     }

	  }
	
  }
}	//else if(GetSimTime() == 2245 && this->GetID()==13 ) {cout<<"Sachindi";getchar();}

if( redirect_flag == 1)	//Injecting the redirected flit to side buffer
{
redirect_flag = 0;
this->sidebuff.push(redirect_flit);
}

}

void IQRouterBase::ReadInputs( )
{
  _ReceiveFlits( );
  if(!_bless)
  _ReceiveCredits( );
}

void IQRouterBase::InternalStep( )
{
  //  _InputQueuing( );
Flit *f = (_input_channels)[4]->Receive();
    if ( f )
    {
//	cout<<"New flit:"<<f->id<<"|"<<this->GetID()<<endl;

	VC * cur_vc=_vc[f->cur_port][0];
	++_received_flits[4];	
	f->input = 4;
	cur_vc->AddFlit(f);
	cur_vc->SetActive();
    }
    

  _Route( );

if(!_bless)
{	  _Alloc( );

 for ( int input = 0; input < _inputs; ++input ) {
    for ( int vc = 0; vc < _vcs; ++vc ) {
      _vc[input][vc]->AdvanceTime( );
    }
  }
}
if(!_bless)
{
  _crossbar_pipe->Advance( );
  _credit_pipe->Advance( );
 _OutputQueuing( );
}
}

void IQRouterBase::_BlessWrite()
{
//cout<<"Router"<<this->GetID()<<endl;
	int i,output;
	Flit *f;
	VC *cur_vc;


 if(!flitbuff3.empty())
  {  
  list<Flit *>::iterator it;

  for ( it=flitbuff3.begin() ; it!=flitbuff3.end(); it++ )
  {
  	f = *it;
	f->from_router = this->GetID();
	output=f->cur_port;
	++_sent_flits[output];
	if(gTrace && f)
	{cout<<"Outport "<<output<<endl;cout<<"Stop Mark"<<endl;}
//	cout<<f->id<<" Sending to port:"<<output<<endl;
	(*_output_channels)[output]->Send( f );
	
  }
  flitbuff3.clear();
  }

  if(!flitbuff2.empty())
  {

  list<Flit *>::iterator it;
  for ( it=flitbuff2.begin() ; it!= flitbuff2.end(); it++ )
  {
  	   	flitbuff3.push_back(*it);   
  }
  flitbuff2.clear();
  }

}

void IQRouterBase::WriteOutputs( )
{
  _SendFlits( );
  _SendCredits( );
}


void IQRouterBase::_ReceiveFlits( )
{
//cout<<"Receving flits";getchar();
  Flit *f, *f1;
  bufferMonitor.cycle() ;
for(int i=0;i<_inputs; i++)
this->nonempty_vc[i] = false;

  for ( int input = 0; input < _inputs; ++input )
  {
    f = (_input_channels)[input]->Receive();
    if ( f )
    {

this->nonempty_vc[input] = true;

		f->port = input;	//current port

		++_received_flits[input];	
		VC * cur_vc = _vc[input][f->vc];

		if (( cur_vc->GetState( ) == VC::idle) && !_bless)
        	{
    	  		if ( !f->head )
    	  		{
    		  		Error( "Received non-head flit at idle VC" );
    	  		}
		  	cur_vc->SetState( VC::routing );
		  	_routing_vcs.push(f->input*_vcs+f->vc);			//why this....?
        	}

        	if(_bless)
        	{
			cur_vc->SetState(VC::routing);	// now VC is ready for routing
        	}

	 	if(f->watch)
         	{
    	  		*gWatchOut << GetSimTime() << " | " << FullName() << " | "
		   	<< "Adding flit " << f->id
		   	<< " to VC " << f->vc
		   	<< " at input " << f->input
		   	<< " (state: " << VC::VCSTATE[cur_vc->GetState()];
    	   	
			if(cur_vc->Empty())
    	   		{
    		   	*gWatchOut << ", empty";
    	   		}
    	   		else
    	   		{
    		   		assert(cur_vc->FrontFlit());
    		   		*gWatchOut << ", front: " << cur_vc->FrontFlit()->id;
    	   		}
	 			*gWatchOut << ")." << endl;
    	   			*gWatchOut << GetSimTime() << " | " << FullName() << " | "
		   		<< "Received flit " << f->id
		   		<< " from channel at input " << f->input
				<< "." << endl;
      		}

      		if ( !cur_vc->AddFlit( f ) )
      		{
    	  		Error( "VC buffer overflow" );
      		}
				
	  
	}
	}
}
bool IQRouterBase::is4free()
{
	if( _vc[4][0]->Empty())
		return true;
	return false;
}
void IQRouterBase::_ReceiveCredits( )
{
  Credit *c;

  for ( int output = 0; output < _outputs; ++output ) {  
    c = (*_output_credits)[output]->Receive();
    if ( c ) {
      _next_vcs[output]->ProcessCredit( c );
      delete c;
    }
  }
}

void IQRouterBase::_InputQueuing( )
{
  /*
  for ( int input = 0; input < _inputs; ++input ) {
    for ( int vc = 0; vc < _vcs; ++vc ) {

      VC * cur_vc = &_vc[input][vc];
      
      if ( cur_vc->GetState( ) == VC::idle ) {
	Flit * f = cur_vc->FrontFlit( );

	if ( f ) {
	  if ( !f->head ) {
	    Error( "Received non-head flit at idle VC" );
	  }
o
	  cur_vc->SetState( VC::routing );
	}
      }
    }
  } 
  */ 
}

//this function relies on the fact that _routing delay is constant for all packets
//if the packet at the head of the queue is <routing_delay, then all other vc in the queue
//will be <routing delay
#include<fstream>
ofstream out1("priority__");
priority_queue<order> order_flits;
void IQRouterBase::_Route( )
{
//cout<<"\nRouter:"<<this->GetID()<<endl;
	int i,output, x1,y1,x2,y2;
	Flit *f;
	VC *cur_vc;
x1 = this->GetID()/gK;
y1 = this->GetID()%gK;

if(_bless)
{ 
  if(!flitbuff1.empty())
  {

    int silv = RandomInt(flitbuff1.size());   
    int cnt = 0;
    list<Flit *>::iterator it;
    for ( it=flitbuff1.begin() ; it!= flitbuff1.end(); it++ )
    {
    	 f = *it;
  	   if( cnt == silv && _Mod_MinBD == false )
  	   f->silver = true;

  	   VC *fake;

       x2 = f->dest/gK;
       y2 = f->dest%gK;
       if(_Mod_MinBD == true) 
       f->hop_dist = -(abs(x2-x1)+abs(y2-y1));
       else
       f->hop_dist = -1;

       //cout<<"\nHurreee"<<f->id<<" Port:"<<f->port<<endl;
  	   flitbuff2.push_back(f);

  	   //order_flits.push( order (f->hop_dist, f, fake) );  //##INFD 
       order_flits.push( order (f->ndefl, f, fake) ); //##INFD
       
       cnt++;
    }

    flitbuff1.clear();
	  _rb(this, &order_flits);
  } 


	for(i=0;i<4;i++)	//For each virtual channel
	{
		cur_vc=_vc[i][0];

		if(!cur_vc->Empty())
		{
		      	f = cur_vc->RemoveFlit();
		      	assert(f);
			f->port = i;	//Incomming port number
			flitbuff1.push_back(f);
		}
	}
}
			
  else
  {
	  int size = _routing_vcs.size();
	  for(int i = 0; i<size; i++)
	  {
		  int vc_encode = _routing_vcs.front();
		  VC * cur_vc = _vc[vc_encode/_vcs][vc_encode%_vcs];
		  if(cur_vc->GetStateTime( ) >= _routing_delay)
		  {
			  Flit * f = cur_vc->FrontFlit( );
			  cur_vc->Route(_rf,this,f,vc_encode/_vcs);	//added shankar
			  //commented shankar  cur_vc->Route( _rf, this, f,  vc_encode/_vcs);// calls the routing function
			  cur_vc->SetState( VC::vc_alloc ) ;
			  _vcalloc_vcs.insert(vc_encode);
			  _routing_vcs.pop();
		  }
		  else
		  {
			  break;
		  }
	  }
	}

}

void IQRouterBase::_OutputQueuing( )
{

  for ( int output = 0; output < _outputs; ++output ) {
    for ( int t = 0; t < _output_speedup; ++t ) {
      int expanded_output = _outputs*t + output;
      Flit * f = _crossbar_pipe->Read( expanded_output );

      if ( f ) {
	_output_buffer[output].push( f );
	if(f->watch)
	  *gWatchOut << GetSimTime() << " | " << FullName() << " | "
		      << "Buffering flit " << f->id
		      << " at output " << output
		      << "." << endl;
      }
    }
  }  

  for ( int input = 0; input < _inputs; ++input ) {
    Credit * c = _credit_pipe->Read( input );

    if ( c ) {
      _in_cred_buffer[input].push( c );
    }
  }
}

void IQRouterBase::_SendFlits( )
{
  Flit *f;

  for ( int output = 0; output < _outputs; ++output ) {
    if ( !_output_buffer[output].empty( ) ) {
      f = _output_buffer[output].front( );
      f->from_router = this->GetID();
      _output_buffer[output].pop( );
      ++_sent_flits[output];
      if(f->watch)
	*gWatchOut << GetSimTime() << " | " << FullName() << " | "
		    << "Sending flit " << f->id
		    << " to channel at output " << output
		    << "." << endl;
    } else {
      f = 0;
    }
    if(gTrace && f){cout<<"Outport "<<output<<endl;cout<<"Stop Mark"<<endl;}
   // cout<<"Outport "<<output<<"    Stop Mark"<<endl;
    (*_output_channels)[output]->Send( f );
  }
}

void IQRouterBase::_SendCredits( )
{
  Credit *c;

  for ( int input = 0; input < _inputs; ++input ) {
    if ( !_in_cred_buffer[input].empty( ) ) {
      c = _in_cred_buffer[input].front( );
      _in_cred_buffer[input].pop( );
    } else {
      c = 0;
    }

    (*_input_credits)[input]->Send( c );
  }
}



void IQRouterBase::Display( ) const
{
  for ( int input = 0; input < _inputs; ++input ) {
    for ( int v = 0; v < _vcs; ++v ) {
      _vc[input][v]->Display( );
    }
  }
}

int IQRouterBase::GetCredit(int out, int vc_begin, int vc_end ) const
{
 

  const BufferState *dest_vc;
  int    tmpsum = 0;
  int cnt = 0;
  
  if (out >= _outputs ) {
    cout << " ERROR  - big output  GetCredit : " << out << endl;
    exit(-1);
  }
  
  dest_vc = _next_vcs[out];
  //dest_vc_tmp = &_next_vcs_tmp[out];
  
  if (vc_begin == -1) {
    for (int v =0;v<_vcs;v++){
      tmpsum+= dest_vc->Size(v);
    }
    return tmpsum;
  }  else if (vc_begin != -1) {
    assert(vc_begin >= 0);
    for (int v =vc_begin;v<= vc_end ;v++)  {
      tmpsum+= dest_vc->Size(v);
      cnt++;
    }
    return tmpsum;
  }
  assert(0); // Should never reach here.
  return -5;
}

int IQRouterBase::GetBuffer(int i) const {
  int size = 0;
  int i_start = (i >= 0) ? i : 0;
  int i_end = (i >= 0) ? i : (_inputs - 1);
  for(int input = i_start; input <= i_end; ++input) {
    for(int vc = 0; vc < _vcs; ++vc) {
      size += _vc[input][vc]->GetSize();
    }
  }
  return size;
}

int IQRouterBase::GetReceivedFlits(int i) const {
  int count = 0;
  int i_start = (i >= 0) ? i : 0;
  int i_end = (i >= 0) ? i : (_inputs - 1);
  for(int input = i_start; input <= i_end; ++input)
    count += _received_flits[input];
  return count;
}

int IQRouterBase::GetSentFlits(int o) const {
  int count = 0;
  int o_start = (o >= 0) ? o : 0;
  int o_end = (o >= 0) ? o : (_outputs - 1);
  for(int output = o_start; output <= o_end; ++output)
    count += _sent_flits[output];
  return count;
}

void IQRouterBase::ResetFlitStats() {
  for(int i = 0; i < _inputs; ++i)
    _received_flits[i] = 0;
  for(int o = 0; o < _outputs; ++o)
    _sent_flits[o] = 0;
}


// ----------------------------------------------------------------------
//
//   Switch Monitor
//
// ----------------------------------------------------------------------
SwitchMonitor::SwitchMonitor( int inputs, int outputs ) {
  // something is stomping on the arrays, so padding is applied
  const int Offset = 16 ;
  _cycles  = 0 ;
  _inputs  = inputs ;
  _outputs = outputs ;
  const int n = 2 * Offset + (inputs+1) * (outputs+1) * Flit::NUM_FLIT_TYPES ;
  _event = new int [ n ] ;
  for ( int i = 0 ; i < n ; i++ ) {
	_event[i] = 0 ;
  }
  _event += Offset ;
}

int SwitchMonitor::index( int input, int output, int flitType ) const {
  return flitType + Flit::NUM_FLIT_TYPES * ( output + _outputs * input ) ;
}

void SwitchMonitor::cycle() {
  _cycles++ ;
}

void SwitchMonitor::traversal( int input, int output, Flit* flit ) {
  _event[ index( input, output, flit->type) ]++ ;
}

ostream& operator<<( ostream& os, const SwitchMonitor& obj ) {
  for ( int i = 0 ; i < obj._inputs ; i++ ) {
    for ( int o = 0 ; o < obj._outputs ; o++) {
      os << "[" << i << " -> " << o << "] " ;
      for ( int f = 0 ; f < Flit::NUM_FLIT_TYPES ; f++ ) {
	os << f << ":" << obj._event[ obj.index(i,o,f)] << " " ;
      }
      os << endl ;
    }
  }
  return os ;
}

// ----------------------------------------------------------------------
//
//   Flit Buffer Monitor
//
// ----------------------------------------------------------------------
BufferMonitor::BufferMonitor( int inputs ) {
  // something is stomping on the arrays, so padding is applied
  const int Offset = 16 ;
  _cycles = 0 ;
  _inputs = inputs ;

  const int n = 2*Offset + 4 * inputs  * Flit::NUM_FLIT_TYPES ;
  _reads  = new int [ n ] ;
  _writes = new int [ n ] ;
  for ( int i = 0 ; i < n ; i++ ) {
    _reads[i]  = 0 ; 
    _writes[i] = 0 ;
  }
  _reads += Offset ;
  _writes += Offset ;
}

int BufferMonitor::index( int input, int flitType ) const {
  if ( input < 0 || input > _inputs ) 
    cerr << "ERROR: input out of range in BufferMonitor" << endl ;
  if ( flitType < 0 || flitType> Flit::NUM_FLIT_TYPES ) 
    cerr << "ERROR: flitType out of range in flitType" << endl ;
  return flitType + Flit::NUM_FLIT_TYPES * input ;
}

void BufferMonitor::cycle() {
  _cycles++ ;
}

void BufferMonitor::write( int input, Flit* flit ) {
  _writes[ index(input, flit->type) ]++ ;
}

void BufferMonitor::read( int input, Flit* flit ) {
  _reads[ index(input, flit->type) ]++ ;
}

ostream& operator<<( ostream& os, const BufferMonitor& obj ) {
  for ( int i = 0 ; i < obj._inputs ; i++ ) {
    os << "[ " << i << " ] " ;
    for ( int f = 0 ; f < Flit::NUM_FLIT_TYPES ; f++ ) {
      os << "Type=" << f
	 << ":(R#" << obj._reads[ obj.index( i, f) ]  << ","
	 << "W#" << obj._writes[ obj.index( i, f) ] << ")" << " " ;
    }
    os << endl ;
  }
  return os ;
}

