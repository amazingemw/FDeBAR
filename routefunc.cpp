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

/*routefunc.cpp
 *
 *This is where most of the routing functions reside. Some of the topologies
 *has their own "register routing functions" which must be called to access
 *those routing functions. 
 *
 *After writing a routing function, don't forget to register it. The reg 
 *format is rfname_topologyname. 
 *
 */

#include "booksim.hpp"
#include <map>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "routefunc.hpp"
#include "kncube.hpp"
#include "random_utils.hpp"
#include "misc_utils.hpp"
//####INFD CODE
#include "fault.hpp"
//####INFD ENDS

#include "fattree.hpp"
#include "tree4.hpp"
#include "qtree.hpp"
#include "cmesh.hpp"



map<string, tRoutingFunction> gRoutingFunctionMap;
map<string, bRoutingFunction> bRoutingFunctionMap;//added shankar
bool _out_alloc[5];
bool _out_pri[5];
/* Global information used by routing functions */

int gNumVCS;
int sb_size;
string _Mod_MinBD;
/* Add more functions here
 *
 */

// ============================================================
//  Balfour-Schultz
int gReadReqBeginVC, gReadReqEndVC;
int gWriteReqBeginVC, gWriteReqEndVC;
int gReadReplyBeginVC, gReadReplyEndVC;
int gWriteReplyBeginVC, gWriteReplyEndVC;
int memo_log2gC = 0 ;

// ----------------------------------------------------------------------
//
//   Crossbar Network 
//
// ----------------------------------------------------------------------
void dest_tag_crossbar( const Router *r, 
		const Flit *f, 
		int in_channel, 
		OutputSet *outputs, 
		bool inject ) {

	outputs->Clear() ;

	// Output port determined by those bits of destination above 
	//  concentration bits
	int out_port = f->dest >> memo_log2gC ;
	switch(f->type) {
		case Flit::READ_REQUEST:
			outputs->AddRange( out_port, gReadReqBeginVC, gReadReqEndVC );
			break;
		case Flit::WRITE_REQUEST:
			outputs->AddRange( out_port, gWriteReqBeginVC, gWriteReqEndVC );
			break;
		case Flit::READ_REPLY:
			outputs->AddRange( out_port, gReadReplyBeginVC, gReadReplyEndVC );
			break;
		case Flit::WRITE_REPLY:
			outputs->AddRange( out_port, gWriteReplyBeginVC, gWriteReplyEndVC );
			break;
		case Flit::ANY_TYPE:
			outputs->AddRange( out_port, 0, gNumVCS-1 );
			break;
	}

}

// ============================================================
//  QTree: Nearest Common Ancestor
// ===
void qtree_nca( const Router *r, const Flit *f,
		int in_channel, OutputSet* outputs, bool inject)
{
	int out_port = 0;
	outputs->Clear( );

	int height = QTree::HeightFromID( r->GetID() );
	int pos    = QTree::PosFromID( r->GetID() );

	int dest   = f->dest;

	for (int i = height+1; i < gN; i++) 
		dest /= gK;
	if ( pos == dest / gK ) 
		// Route down to child
		out_port = dest % gK ; 
	else
		// Route up to parent
		out_port = gK;        

	switch(f->type) {
		case Flit::READ_REQUEST:
			outputs->AddRange( out_port, gReadReqBeginVC, gReadReqEndVC );
			break;
		case Flit::WRITE_REQUEST:
			outputs->AddRange( out_port, gWriteReqBeginVC, gWriteReqEndVC );
			break;
		case Flit::READ_REPLY:
			outputs->AddRange( out_port, gReadReplyBeginVC, gReadReplyEndVC );
			break;
		case Flit::WRITE_REPLY:
			outputs->AddRange( out_port, gWriteReplyBeginVC, gWriteReplyEndVC );
			break;
		case Flit::ANY_TYPE:
			outputs->AddRange( out_port, 0, gNumVCS-1 );
			break;
	}
}

// ============================================================
//  Tree4: Nearest Common Ancestor w/ Adaptive Routing Up
// ===
void tree4_anca( const Router *r, const Flit *f,
		int in_channel, OutputSet* outputs, bool inject)
{
	const int NPOS = 16;

	int out_port = 0;
	int dest     = f->dest;

	int rH = r->GetID( ) / NPOS;
	int rP = r->GetID( ) % NPOS;

	int range = 1;

	outputs->Clear( );

	if ( rH == 0 ) {
		dest /= 16;
		out_port = 2 * dest + RandomInt(1);
	} else if ( rH == 1 ) {
		dest /= 4;
		if ( dest / 4 == rP / 2 )
			out_port = dest % 4;
		else {
			out_port = gK;
			range = gK;
		}
	} else {
		if ( dest/4 == rP )
			out_port = dest % 4;
		else {
			out_port = gK;
			range = 2;
		}
	}

	//  cout << "Router("<<rH<<","<<rP<<"): id= " << f->id << " dest= " << f->dest << " out_port = "
	//       << out_port << endl;

	int vcBegin = 0, vcEnd = 0;
	switch(f->type) {
		case Flit::READ_REQUEST:
			vcBegin = gReadReqBeginVC;
			vcEnd   = gReadReqEndVC;
			break;
		case Flit::WRITE_REQUEST:
			vcBegin = gWriteReqBeginVC;
			vcEnd   = gWriteReqEndVC;
			break;
		case Flit::READ_REPLY:
			vcBegin = gReadReplyBeginVC;
			vcEnd   = gReadReplyEndVC;
			break;
		case Flit::WRITE_REPLY:
			vcBegin = gWriteReplyBeginVC;
			vcEnd   = gWriteReplyEndVC;
			break;
		case Flit::ANY_TYPE:
			vcBegin = 0;
			vcEnd   = gNumVCS-1;
	}

	for (int i = 0; i < range; ++i) 
		outputs->AddRange( out_port + i, vcBegin, vcEnd );
}

// ============================================================
//  Tree4: Nearest Common Ancestor w/ Random Routing Up
// ===
void tree4_nca( const Router *r, const Flit *f,
		int in_channel, OutputSet* outputs, bool inject)
{
	const int NPOS = 16;

	int out_port = 0;
	int dest     = f->dest;

	int rH = r->GetID( ) / NPOS;
	int rP = r->GetID( ) % NPOS;

	outputs->Clear( );

	if ( rH == 0 ) {
		dest /= 16;
		out_port = 2 * dest + RandomInt(1);
	} else if ( rH == 1 ) {
		dest /= 4;
		if ( dest / 4 == rP / 2 )
			out_port = dest % 4;
		else
			out_port = gK + RandomInt(gK-1);
	} else {
		if ( dest/4 == rP )
			out_port = dest % 4;
		else
			out_port = gK + RandomInt(1);
	}

	//  cout << "Router("<<rH<<","<<rP<<"): id= " << f->id << " dest= " << f->dest << " out_port = "
	//       << out_port << endl;

	switch(f->type) {
		case Flit::READ_REQUEST:
			outputs->AddRange( out_port, gReadReqBeginVC, gReadReqEndVC );
			break;
		case Flit::WRITE_REQUEST:
			outputs->AddRange( out_port, gWriteReqBeginVC, gWriteReqEndVC );
			break;
		case Flit::READ_REPLY:
			outputs->AddRange( out_port, gReadReplyBeginVC, gReadReplyEndVC );
			break;
		case Flit::WRITE_REPLY:
			outputs->AddRange( out_port, gWriteReplyBeginVC, gWriteReplyEndVC );
			break;
		case Flit::ANY_TYPE:
			outputs->AddRange( out_port, 0, gNumVCS-1 );
			break;
	}
}

// ============================================================
//  FATTREE: Nearest Common Ancestor w/ Random  Routing Up
// ===
void fattree_nca( const Router *r, const Flit *f,
		int in_channel, OutputSet* outputs, bool inject)
{
	int out_port = 0;
	int dest     = f->dest;
	int router_id = r->GetID();
	short router_depth, router_port;



	short routers_so_far = 0, routers;
	for (short depth = gN - 1; depth >= 0; --depth) // We want to find out where the router is. At which level.
	{
		routers = powi(gK, depth);
		if (router_id - routers_so_far < routers) {
			router_depth = depth;
			router_port = router_id - routers_so_far;
			break;
		}
		routers_so_far += routers;
	}

	outputs->Clear( );

	int fatness_factor = powi(gK, gN - router_depth ); // This is the fatness factor for when going upwards.
	int destinations = powi(gK, gN); // According to the depth and port of the current router, we know which destinations are reachable by it by going down.
	int temp = powi(gK, router_depth); // (destinations / (powi(gK, router_depth) ) are the number of destinations below the current router.
	if ((destinations / (temp) ) * router_port <= dest && (destinations / (temp) ) * (router_port + 1) > dest)
	{
		out_port = (dest - (destinations / (temp) ) * router_port) / (fatness_factor / gK); // This is the direction to go. We multiply this by the fatness factor of that link.
		out_port *= fatness_factor / gK; // Because we are going downwards. Now we point to the first link that is going downwards, among all the same.
		if (router_depth != gN - 1 )
			out_port += RandomInt(fatness_factor / gK - 1); // Choose at random from the possible choices.
		else
			out_port = dest % gK; // If we are going to a final destination, only one link to choose.
	}
	else // We need to go up. Choose one of the links at random.
	{
		out_port = fatness_factor /*upwards ports are the last indexed */ + RandomInt(fatness_factor - 1); // Chose one of the going upwards at random.
	}

	/*if (rH == 0) {
	  out_port = dest / (gK*gK);
	  }
	  if (rH == 1) {
	  if ( dest / (gK*gK)  == rP / gK )
	  out_port = (dest/gK) % gK;
	  else
	  out_port = gK + RandomInt(gK-1);
	  }
	  if (rH == 2) {
	  if ( dest / gK == rP )
	  out_port = dest % gK;
	  else
	  out_port =  gK + RandomInt(gK-1);
	  }*/
	//  cout << "Router("<<rH<<","<<rP<<"): id= " << f->id << " dest= " << f->dest << " out_port = "
	//       << out_port << endl;

	switch(f->type) {
		case Flit::READ_REQUEST:
			outputs->AddRange( out_port, gReadReqBeginVC, gReadReqEndVC );
			break;
		case Flit::WRITE_REQUEST:
			outputs->AddRange( out_port, gWriteReqBeginVC, gWriteReqEndVC );
			break;
		case Flit::READ_REPLY:
			outputs->AddRange( out_port, gReadReplyBeginVC, gReadReplyEndVC );
			break;
		case Flit::WRITE_REPLY:
			outputs->AddRange( out_port, gWriteReplyBeginVC, gWriteReplyEndVC );
			break;
		case Flit::ANY_TYPE:
			outputs->AddRange( out_port, 0, gNumVCS-1 );
			break;
	}
}

// ============================================================
//  FATTREE: Nearest Common Ancestor w/ Adaptive Routing Up
// ===
void fattree_anca( const Router *r, const Flit *f,
		int in_channel, OutputSet* outputs, bool inject)
{

	int out_port = 0;
	int dest     = f->dest;
	int router_id = r->GetID();
	short router_depth, router_port;

	short routers_so_far = 0, routers;
	for (short depth = gN - 1; depth >= 0; --depth) // We want to find out where the router is. At which level.
	{
		routers = powi(gK, depth);
		if (router_id - routers_so_far < routers) {
			router_depth = depth;
			router_port = router_id - routers_so_far;
			break;
		}
		routers_so_far += routers;
	}

	outputs->Clear( );

	int fatness_factor = powi(gK, gN - router_depth ); // This is the fatness factor for when going upwards.
	int destinations = powi(gK, gN); // According to the depth and port of the current router, we know which destinations are reachable by it by going down.
	int temp = powi(gK, router_depth);
	int range; // (destinations / (powi(gK, router_depth) ) are the number of destinations below the current router.
	if ((destinations / (temp) ) * router_port <= dest && (destinations / (temp) ) * (router_port + 1) > dest)
	{
		out_port = (dest - (destinations / (temp)) * router_port) / (fatness_factor / gK); // This is the direction to go. We multiply this by the fatness factor of that link.
		out_port *= fatness_factor / gK; // Because we are going downwards. Now we point to the first link that is going downwards, among all the same.
		if (router_depth != gN - 1 )
		{
			range = fatness_factor / gK;
		}
		else
		{
			out_port = dest % gK; // If we are going to a final destination, only one link to choose.
			range = 1;
		}
	}
	else // We need to go up. Choose one of the links at random.
	{
		range = fatness_factor;
		out_port = fatness_factor /*upwards ports are the last indexed */ ;
	}

	// r->GetCredit(tmp_out_port, vcBegin, vcEnd);

	/*if (rH == 0) {
	  out_port = dest / (gK*gK);
	  }
	  if (rH == 1) {
	  if ( dest / (gK*gK)  == rP / gK )
	  out_port = (dest/gK) % gK;
	  else {
	  out_port = gK;
	  range    = gK;
	  }

	  }
	  if (rH == 2) {
	  if ( dest / gK == rP )
	  out_port = dest % gK;
	  else {
	  out_port = gK;
	  range    = gK;
	  }
	  }*/

	//  cout << "Router("<<rH<<","<<rP<<"): id= "
	//       << f->id << " dest= " << f->dest << " out_port = "
	//       << out_port << endl;

	int vcBegin = 0, vcEnd = 0;
	switch(f->type) {
		case Flit::READ_REQUEST:
			vcBegin = gReadReqBeginVC;
			vcEnd   = gReadReqEndVC;
			break;
		case Flit::WRITE_REQUEST:
			vcBegin = gWriteReqBeginVC;
			vcEnd   = gWriteReqEndVC;
			break;
		case Flit::READ_REPLY:
			vcBegin = gReadReplyBeginVC;
			vcEnd   = gReadReplyEndVC;
			break;
		case Flit::WRITE_REPLY:
			vcBegin = gWriteReplyBeginVC;
			vcEnd   = gWriteReplyEndVC;
			break;
		case Flit::ANY_TYPE:
			vcBegin = 0;
			vcEnd   = gNumVCS-1;
	}

	if (range > 1) {
		/*for (int i = 0; i < range; ++i)
		  outputs->AddRange( out_port + i, vcBegin, vcEnd );*/
		short random1 = RandomInt(range-1); // Chose two ports out of the possible at random, compare loads, choose one.
		short random2 = RandomInt(range-1);
		if (r->GetCredit(out_port + random1, vcBegin, vcEnd) > r->GetCredit(out_port + random2, vcBegin, vcEnd))
			outputs->AddRange( out_port + random2, vcBegin, vcEnd );
		else
			outputs->AddRange( out_port + random1, vcBegin, vcEnd );
	}
	else
		outputs->AddRange( out_port , vcBegin, vcEnd );
}




// ============================================================
//  Mesh: Dimension-order w/ VC restrictions
// ====
int dor_next_mesh(int, int);
void dor_mesh( const Router *r, const Flit *f, 
		int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;

	outputs->Clear( );

	out_port = dor_next_mesh( r->GetID( ), f->dest );
	if (f->type == Flit::READ_REQUEST)
		outputs->AddRange( out_port, gReadReqBeginVC, gReadReqEndVC );
	else if (f->type == Flit::WRITE_REQUEST)
		outputs->AddRange( out_port, gWriteReqBeginVC, gWriteReqEndVC );
	else if (f->type ==  Flit::READ_REPLY)
		outputs->AddRange( out_port, gReadReplyBeginVC, gReadReplyEndVC );
	else if (f->type ==  Flit::WRITE_REPLY)
		outputs->AddRange( out_port, gWriteReplyBeginVC, gWriteReplyEndVC );
	else if (f->type ==  Flit::ANY_TYPE)
		outputs->AddRange( out_port, 0, gNumVCS-1 );
}

// ============================================================
//  Mesh - Random XY,YX Routing 
//         Traffic Class Virtual Channel assignment Enforced 
// ===
int route_xy( int router_id, int dest_id );
int route_yx( int router_id, int dest_id );

void xy_yx_mesh( const Router *r, const Flit *f, 
		int in_channel, OutputSet *outputs, bool inject )
{
	int  out_port = 0;
	outputs->Clear();

	// Route order (XY or YX) determined when packet is injected
	//  into the network
	if (in_channel == 4) {
		if ( RandomInt(1) ) {
			out_port = route_xy( r->GetID(), f->dest );
			f->x_then_y = true;
		} else {
			out_port = route_yx( r->GetID(), f->dest );
			f->x_then_y = false;
		}
	} else {
		if ( f->x_then_y )
			out_port = route_xy( r->GetID(), f->dest );
		else
			out_port = route_yx( r->GetID(), f->dest );
	}

	// ( Traffic Class , Routing Order ) -> Virtual Channel Range
	int vcBegin = 0, vcEnd = gNumVCS-1;
	int available_vcs = 0;
	//each class must have ast east 2 vcs assigned or else xy_yx will deadlock
	if ( f->type == Flit::READ_REQUEST ) {
		available_vcs = (gReadReqEndVC-gReadReqBeginVC)+1;
		vcBegin = gReadReqBeginVC;
	} else if ( f->type == Flit::WRITE_REQUEST ) {
		available_vcs = (gWriteReqEndVC-gWriteReqBeginVC)+1;
		vcBegin = gWriteReqBeginVC;
	} else if ( f->type ==  Flit::READ_REPLY ) {
		available_vcs = (gReadReplyEndVC-gReadReplyBeginVC)+1;
		vcBegin = gReadReplyBeginVC;
	} else if ( f->type ==  Flit::WRITE_REPLY ) {
		available_vcs = (gWriteReplyEndVC-gWriteReplyBeginVC)+1;      
		vcBegin = gWriteReplyBeginVC;
	} else if ( f->type ==  Flit::ANY_TYPE ) {
		available_vcs = gNumVCS;
		vcBegin = 0;
	}
	assert( available_vcs>=2);
	if(f->x_then_y){
		vcEnd   =vcBegin +(available_vcs>>1)-1;
	}else{
		vcEnd   = vcBegin+(available_vcs-1);
		vcBegin = vcBegin+(available_vcs>>1);
	} 
	outputs->AddRange( out_port , vcBegin, vcEnd );

}

int route_xy( int router_id, int dest_id ) {

	int router_x = router_id % gK;
	int router_y = router_id / gK;
	int dest_x = dest_id % gK;
	int dest_y = dest_id / gK;
	int out_port = 0;

	if ( router_x < dest_x ) 
		out_port = 0;
	else if ( router_x > dest_x )
		out_port = 1;
	else {
		if ( router_y < dest_y )
			out_port = 2;
		else if ( router_y > dest_y )
			out_port = 3;
		else
			out_port = 2*gN;
	}
	return out_port;
}

int route_yx( int router_id, int dest_id ) {

	int router_x = router_id % gK;
	int router_y = router_id / gK;
	int dest_x = dest_id % gK;
	int dest_y = dest_id / gK;
	int out_port = 0;

	if ( router_y < dest_y )
		out_port = 2;
	else if ( router_y > dest_y )
		out_port = 3;
	else  {
		if ( router_x < dest_x ) 
			out_port = 0;
		else if ( router_x > dest_x )
			out_port = 1;
		else 
			out_port = 2*gN;
	}
	return out_port;
}

//
// End Balfour-Schultz
//=============================================================

//=============================================================




void singlerf( const Router *, const Flit *f, int, OutputSet *outputs, bool inject )
{
	outputs->Clear( );
	outputs->Add( f->dest, f->dest % gNumVCS ); // VOQing
}

//=============================================================
#include<fstream>
//ofstream out2("outport__");
int dor_next_mesh( int cur, int dest )
{
	int dim_left;
	int out_port;
	// out2<<"\n curr= "<<cur<<"      dest="<<dest;
	for ( dim_left = 0; dim_left < gN; ++dim_left )
	{
		if ( ( cur % gK ) != ( dest % gK ) ) { break; }
		cur /= gK; dest /= gK;
	}

	if ( dim_left < gN ) {
		cur %= gK; dest %= gK;

		if ( cur < dest ) {
			out_port = 2*dim_left;     // Right
		} else {
			out_port = 2*dim_left + 1; // Left
		}
	} else {
		out_port = 2*gN;  // Eject
	}
	//out2<<"    outport="<<out_port<<"";
	return out_port;
}

//=============================================================

void dor_next_torus( int cur, int dest, int in_port,
		int *out_port, int *partition,
		bool balance = false )
{
	int dim_left;
	int dir;
	int dist2;

	for ( dim_left = 0; dim_left < gN; ++dim_left ) {
		if ( ( cur % gK ) != ( dest % gK ) ) { break; }
		cur /= gK; dest /= gK;
	}

	if ( dim_left < gN ) {

		if ( (in_port/2) != dim_left ) {
			// Turning into a new dimension

			cur %= gK; dest %= gK;
			dist2 = gK - 2 * ( ( dest - cur + gK ) % gK );

			if ( ( dist2 > 0 ) || 
					( ( dist2 == 0 ) && ( RandomInt( 1 ) ) ) ) {
				*out_port = 2*dim_left;     // Right
				dir = 0;
			} else {
				*out_port = 2*dim_left + 1; // Left
				dir = 1;
			}

			if ( balance ) {
				// Cray's "Partition" allocation
				// Two datelines: one between k-1 and 0 which forces VC 1
				//                another between ((k-1)/2) and ((k-1)/2 + 1) which forces VC 0
				//                otherwise any VC can be used

				if ( ( ( dir == 0 ) && ( cur > dest ) ) ||
						( ( dir == 1 ) && ( cur < dest ) ) ) {
					*partition = 1;
				} else if ( ( ( dir == 0 ) && ( cur <= (gK-1)/2 ) && ( dest >  (gK-1)/2 ) ) ||
						( ( dir == 1 ) && ( cur >  (gK-1)/2 ) && ( dest <= (gK-1)/2 ) ) ) {
					*partition = 0;
				} else {
					*partition = RandomInt( 1 ); // use either VC set
				}
			} else {
				// Deterministic, fixed dateline between nodes k-1 and 0

				if ( ( ( dir == 0 ) && ( cur > dest ) ) ||
						( ( dir == 1 ) && ( dest < cur ) ) ) {
					*partition = 1;
				} else {
					*partition = 0;
				}
			}
		} else {
			// Inverting the least significant bit keeps
			// the packet moving in the same direction
			*out_port = in_port ^ 0x1;
		}    

	} else {
		*out_port = 2*gN;  // Eject
	}
}

//=============================================================

void dim_order_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;

	outputs->Clear( );

	if ( inject ) { // use any VC for injection
		outputs->AddRange( 0, 0, gNumVCS - 1 );
	} else {
		out_port = dor_next_mesh( r->GetID( ), f->dest );

		if ( f->watch ) {
			*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
				<< "Adding VC range [" 
				<< 0 << "," 
				<< gNumVCS - 1 << "]"
				<< " at output port " << out_port
				<< " for flit " << f->id
				<< " (input port " << in_channel
				<< ", destination " << f->dest << ")"
				<< "." << endl;
		}

		outputs->AddRange( out_port, 0, gNumVCS - 1 );
	}
}

//=============================================================

void dim_order_ni_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int vcs_per_dest = gNumVCS / gNodes;

	outputs->Clear( );
	out_port = dor_next_mesh( r->GetID( ), f->dest );

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< f->dest*vcs_per_dest << "," 
			<< (f->dest+1)*vcs_per_dest - 1 << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	outputs->AddRange( out_port, f->dest*vcs_per_dest, (f->dest+1)*vcs_per_dest - 1 );
}

//=============================================================

// Random intermediate in the minimal quadrant defined
// by the source and destination
int rand_min_intr_mesh( int src, int dest )
{
	int dist;

	int intm = 0;
	int offset = 1;

	for ( int n = 0; n < gN; ++n ) {
		dist = ( dest % gK ) - ( src % gK );

		if ( dist > 0 ) {
			intm += offset * ( ( src % gK ) + RandomInt( dist ) );
		} else {
			intm += offset * ( ( dest % gK ) + RandomInt( -dist ) );
		}

		offset *= gK;
		dest /= gK; src /= gK;
	}

	return intm;
}

//=============================================================

void romm_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		f->ph   = 1;  // Phase 1
		f->intm = rand_min_intr_mesh( f->src, f->dest );
	} 

	if ( ( f->ph == 1 ) && ( r->GetID( ) == f->intm ) ) {
		f->ph = 2; // Go to phase 2
	}

	if ( f->ph == 1 ) { // In phase 1
		out_port = dor_next_mesh( r->GetID( ), f->intm );
	} else { // In phase 2
		out_port = dor_next_mesh( r->GetID( ), f->dest );
	}


	//each class must have ast east 2 vcs assigned or else valiant valiant will deadlock
	int available_vcs = 0;
	if ( f->type == Flit::READ_REQUEST ) {
		available_vcs = (gReadReqEndVC-gReadReqBeginVC)+1;
		vc_min = gReadReqBeginVC;
	} else if ( f->type == Flit::WRITE_REQUEST ) {
		available_vcs = (gWriteReqEndVC-gWriteReqBeginVC)+1;
		vc_min = gWriteReqBeginVC;
	} else if ( f->type ==  Flit::READ_REPLY ) {
		available_vcs = (gReadReplyEndVC-gReadReplyBeginVC)+1;
		vc_min = gReadReplyBeginVC;
	} else if ( f->type ==  Flit::WRITE_REPLY ) {
		available_vcs = (gWriteReplyEndVC-gWriteReplyBeginVC)+1;      
		vc_min = gWriteReplyBeginVC;
	} else if ( f->type ==  Flit::ANY_TYPE ) {
		available_vcs = gNumVCS;
		vc_min = 0;
	}
	assert( available_vcs>=2);
	if(f->ph==1){
		vc_max   =vc_min +(available_vcs>>1)-1;
	}else{
		vc_max   = vc_min+(available_vcs-1);
		vc_min = vc_min+(available_vcs>>1);
	} 


	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void romm_ni_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int vcs_per_dest = gNumVCS / gNodes;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		f->ph   = 1;  // Phase 1
		f->intm = rand_min_intr_mesh( f->src, f->dest );
	} 

	if ( ( f->ph == 1 ) && ( r->GetID( ) == f->intm ) ) {
		f->ph = 2; // Go to phase 2
	}

	if ( f->ph == 1 ) { // In phase 1
		out_port = dor_next_mesh( r->GetID( ), f->intm );
	} else { // In phase 2
		out_port = dor_next_mesh( r->GetID( ), f->dest );
	}

	outputs->AddRange( out_port, f->dest*vcs_per_dest, (f->dest+1)*vcs_per_dest - 1 );
}

//=============================================================

void min_adapt_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int cur, dest;
	int in_vc;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		in_vc = gNumVCS - 1; // ignore the injection VC
	} else {
		in_vc = f->vc;
	}

	// DOR for the escape channel (VC 0), low priority 
	out_port = dor_next_mesh( r->GetID( ), f->dest );    
	outputs->AddRange( out_port, 0, 0, 0 );

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< 0 << "," 
			<< 0 << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	if ( in_vc != 0 ) { // If not in the escape VC
		// Minimal adaptive for all other channels
		cur = r->GetID( ); dest = f->dest;

		for ( int n = 0; n < gN; ++n ) {
			if ( ( cur % gK ) != ( dest % gK ) ) { 
				// Add minimal direction in dimension 'n'
				if ( ( cur % gK ) < ( dest % gK ) ) { // Right
					if ( f->watch ) {
						*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
							<< "Adding VC range [" 
							<< 1 << "," 
							<< gNumVCS - 1 << "]"
							<< " at output port " << 2*n
							<< " with priority " << 1
							<< " for flit " << f->id
							<< " (input port " << in_channel
							<< ", destination " << f->dest << ")"
							<< "." << endl;
					}
					outputs->AddRange( 2*n, 1, gNumVCS - 1, 1 ); 
				} else { // Left
					if ( f->watch ) {
						*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
							<< "Adding VC range [" 
							<< 1 << "," 
							<< gNumVCS - 1 << "]"
							<< " at output port " << 2*n+1
							<< " with priority " << 1
							<< " for flit " << f->id
							<< " (input port " << in_channel
							<< ", destination " << f->dest << ")"
							<< "." << endl;
					}
					outputs->AddRange( 2*n + 1, 1, gNumVCS - 1, 1 ); 
				}
			}
			cur  /= gK;
			dest /= gK;
		}
	} 
}

//=============================================================

void planar_adapt_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int cur, dest;
	int vc_mult;
	int vc_min, vc_max;
	int d1_min_c;
	int in_vc;
	int n;

	bool increase;
	bool fault;
	bool atedge;

	outputs->Clear( );

	cur     = r->GetID( ); 
	dest    = f->dest;
	in_vc   = f->vc;
	vc_mult = gNumVCS / 3;

	if ( cur != dest ) {

		// Find the first unmatched dimension -- except
		// for when we're in the first dimension because
		// of misrouting in the last adaptive plane.
		// In this case, go to the last dimension instead.

		for ( n = 0; n < gN; ++n ) {
			if ( ( ( cur % gK ) != ( dest % gK ) ) &&
					!( ( in_channel/2 == 0 ) &&
						( n == 0 ) &&
						( in_vc < 2*vc_mult ) ) ) {
				break;
			}

			cur  /= gK;
			dest /= gK;
		}

		assert( n < gN );

		if ( f->watch ) {
			*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
				<< "PLANAR ADAPTIVE: flit " << f->id 
				<< " in adaptive plane " << n << "." << endl;
		}

		// We're in adaptive plane n

		// Can route productively in d_{i,2}
		if ( ( cur % gK ) < ( dest % gK ) ) { // Increasing
			increase = true;
			if ( !r->IsFaultyOutput( 2*n ) ) {
				outputs->AddRange( 2*n, 2*vc_mult, gNumVCS - 1 );
				fault = false;

				if ( f->watch ) {
					*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
						<< "PLANAR ADAPTIVE: increasing in dimension " << n
						<< "." << endl;
				}
			} else {
				fault = true;
			}
		} else { // Decreasing
			increase = false;
			if ( !r->IsFaultyOutput( 2*n + 1 ) ) {
				outputs->AddRange( 2*n + 1, 2*vc_mult, gNumVCS - 1 ); 
				fault = false;

				if ( f->watch ) {
					*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
						<< "PLANAR ADAPTIVE: decreasing in dimension " << n
						<< "." << endl;
				}
			} else {
				fault = true;
			}
		}

		n = ( n + 1 ) % gN;
		cur  /= gK;
		dest /= gK;

		if ( increase ) {
			vc_min = 0;
			vc_max = vc_mult - 1;
		} else {
			vc_min = vc_mult;
			vc_max = 2*vc_mult - 1;
		}

		if ( ( cur % gK ) < ( dest % gK ) ) { // Increasing in d_{i+1}
			d1_min_c = 2*n;
		} else if ( ( cur % gK ) != ( dest % gK ) ) {  // Decreasing in d_{i+1}
			d1_min_c = 2*n + 1;
		} else {
			d1_min_c = -1;
		}

		// do we want to 180?  if so, the last
		// route was a misroute in this dimension,
		// if there is no fault in d_i, just ignore
		// this dimension, otherwise continue to misroute
		if ( d1_min_c == in_channel ) { 
			if ( fault ) {
				d1_min_c = in_channel ^ 1;
			} else {
				d1_min_c = -1;
			}

			if ( f->watch ) {
				*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
					<< "PLANAR ADAPTIVE: avoiding 180 in dimension " << n
					<< "." << endl;
			}
		}

		if ( d1_min_c != -1 ) {
			if ( !r->IsFaultyOutput( d1_min_c ) ) {
				outputs->AddRange( d1_min_c, vc_min, vc_max );
			} else if ( fault ) {
				// major problem ... fault in d_i and d_{i+1}
				r->Error( "There seem to be faults in d_i and d_{i+1}" );
			}
		} else if ( fault ) { // need to misroute!
			if ( cur % gK == 0 ) {
				d1_min_c = 2*n;
				atedge = true;
			} else if ( cur % gK == gK - 1 ) {
				d1_min_c = 2*n + 1;
				atedge = true;
			} else {
				d1_min_c = 2*n + RandomInt( 1 ); // random misroute

				if ( d1_min_c  == in_channel ) { // don't 180
					d1_min_c = in_channel ^ 1;
				}
				atedge = false;
			}

			if ( !r->IsFaultyOutput( d1_min_c ) ) {
				outputs->AddRange( d1_min_c, vc_min, vc_max );
			} else if ( !atedge && !r->IsFaultyOutput( d1_min_c ^ 1 ) ) {
				outputs->AddRange( d1_min_c ^ 1, vc_min, vc_max );
			} else {
				// major problem ... fault in d_i and d_{i+1}
				r->Error( "There seem to be faults in d_i and d_{i+1}" );
			}
		}
	} else {
		outputs->AddRange( 2*gN, 0, gNumVCS - 1 ); 
	}
}

//=============================================================

void limited_adapt_mesh_old( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int in_vc;
	int in_dim;

	int min_port;

	bool dor_dim;
	bool equal;

	int cur, dest;

	outputs->Clear( );

	if ( inject ) {
		outputs->AddRange( 0, 0, gNumVCS - 1 );
		f->ph = 0; // zero dimension reversals
	} else {

		cur = r->GetID( ); dest = f->dest;
		if ( cur != dest ) {

			if ( f->ph == 0 ) {
				f->ph = 1;

				in_vc  = 0;
				in_dim = 0;
			} else {
				in_vc  = f->vc;
				in_dim = in_channel/2;
			}

			// The first remaining is the DOR escape path
			dor_dim = true;

			for ( int n = 0; n < gN; ++n ) {
				if ( ( cur % gK ) != ( dest % gK ) ) { 
					if ( ( cur % gK ) < ( dest % gK ) ) { 
						min_port = 2*n; // Right
					} else {
						min_port = 2*n + 1; // Left
					}

					if ( dor_dim ) {
						// Low priority escape path
						outputs->AddRange( min_port, gNumVCS - 1, gNumVCS - 1, 0 ); 
						dor_dim = false;
					}

					equal = false;
				} else {
					equal = true;
					min_port = 2*n;
				}

				if ( in_vc < gNumVCS - 1 ) {  // adaptive VC's left?
					if ( n < in_dim ) {
						// Productive (minimal) direction, with reversal
						if ( in_vc == gNumVCS - 2 ) {
							outputs->AddRange( min_port, in_vc + 1, in_vc + 1, equal ? 1 : 2 ); 
						} else {
							outputs->AddRange( min_port, in_vc + 1, gNumVCS - 2, equal ? 1 : 2 ); 
						}

						// Unproductive (non-minimal) direction, with reversal
						if ( in_vc <  gNumVCS - 2 ) {
							if ( in_vc == gNumVCS - 3 ) {
								outputs->AddRange( min_port ^ 0x1, in_vc + 1, in_vc + 1, 1 );
							} else {
								outputs->AddRange( min_port ^ 0x1, in_vc + 1, gNumVCS - 3, 1 );
							}
						}
					} else if ( n == in_dim ) {
						if ( !equal ) {
							// Productive (minimal) direction, no reversal
							outputs->AddRange( min_port, in_vc, gNumVCS - 2, 4 ); 
						}
					} else {
						// Productive (minimal) direction, no reversal
						outputs->AddRange( min_port, in_vc, gNumVCS - 2, equal ? 1 : 3 ); 
						// Unproductive (non-minimal) direction, no reversal
						if ( in_vc <  gNumVCS - 2 ) {
							outputs->AddRange( min_port ^ 0x1, in_vc, gNumVCS - 2, 1 );
						}
					}
				}

				cur  /= gK;
				dest /= gK;
			}
		} else { // at destination
			outputs->AddRange( 2*gN, 0, gNumVCS - 1 ); 
		}
	} 
}

void limited_adapt_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int min_port;

	int cur, dest;

	outputs->Clear( );

	if ( inject ) {
		outputs->AddRange( 0, 0, gNumVCS - 2 );
		f->dr = 0; // zero dimension reversals
	} else {
		cur = r->GetID( ); dest = f->dest;

		if ( cur != dest ) {
			if ( ( f->vc != gNumVCS - 1 ) && 
					( f->dr != gNumVCS - 2 ) ) {

				for ( int n = 0; n < gN; ++n ) {
					if ( ( cur % gK ) != ( dest % gK ) ) { 
						if ( ( cur % gK ) < ( dest % gK ) ) { 
							min_port = 2*n; // Right
						} else {
							min_port = 2*n + 1; // Left
						}

						// Go in a productive direction with high priority
						outputs->AddRange( min_port, 0, gNumVCS - 2, 2 );

						// Go in the non-productive direction with low priority
						outputs->AddRange( min_port ^ 0x1, 0, gNumVCS - 2, 1 );
					} else {
						// Both directions are non-productive
						outputs->AddRange( 2*n, 0, gNumVCS - 2, 1 );
						outputs->AddRange( 2*n+1, 0, gNumVCS - 2, 1 );
					}

					cur  /= gK;
					dest /= gK;
				}

			} else {
				outputs->AddRange( dor_next_mesh( cur, dest ),
						gNumVCS - 1, gNumVCS - 1, 0 );
			}

		} else { // at destination
			outputs->AddRange( 2*gN, 0, gNumVCS - 1 ); 
		}
	} 
}

//=============================================================

void valiant_mesh( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );


	if ( in_channel == 2*gN ) {
		f->ph   = 1;  // Phase 1
		f->intm = RandomInt( gNodes - 1 );
	}

	if ( ( f->ph == 1 ) && ( r->GetID( ) == f->intm ) ) {
		f->ph = 2; // Go to phase 2
	}

	if ( f->ph == 1 ) { // In phase 1
		out_port = dor_next_mesh( r->GetID( ), f->intm );
	} else { // In phase 2
		out_port = dor_next_mesh( r->GetID( ), f->dest );
	}
	//each class must have ast east 2 vcs assigned or else valiant valiant will deadlock
	int available_vcs = 0;
	if ( f->type == Flit::READ_REQUEST ) {
		available_vcs = (gReadReqEndVC-gReadReqBeginVC)+1;
		vc_min = gReadReqBeginVC;
	} else if ( f->type == Flit::WRITE_REQUEST ) {
		available_vcs = (gWriteReqEndVC-gWriteReqBeginVC)+1;
		vc_min = gWriteReqBeginVC;
	} else if ( f->type ==  Flit::READ_REPLY ) {
		available_vcs = (gReadReplyEndVC-gReadReplyBeginVC)+1;
		vc_min = gReadReplyBeginVC;
	} else if ( f->type ==  Flit::WRITE_REPLY ) {
		available_vcs = (gWriteReplyEndVC-gWriteReplyBeginVC)+1;      
		vc_min = gWriteReplyBeginVC;
	} else if ( f->type ==  Flit::ANY_TYPE ) {
		available_vcs = gNumVCS;
		vc_min = 0;
	}
	assert( available_vcs>=2);
	if(f->ph==1){
		vc_max   =vc_min +(available_vcs>>1)-1;
	}else{
		vc_max   = vc_min+(available_vcs-1);
		vc_min = vc_min+(available_vcs>>1);
	}     

	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void valiant_torus( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		f->ph   = 1;  // Phase 1
		f->intm = RandomInt( gNodes - 1 );
	}

	if ( ( f->ph == 1 ) && ( r->GetID( ) == f->intm ) ) {
		f->ph = 2; // Go to phase 2
		in_channel = 2*gN; // ensures correct vc selection at the beginning of phase 2
	}

	if ( f->ph == 1 ) { // In phase 1
		dor_next_torus( r->GetID( ), f->intm, in_channel,
				&out_port, &f->ring_par, false );
	} else { // In phase 2
		dor_next_torus( r->GetID( ), f->dest, in_channel,
				&out_port, &f->ring_par, false );
	}

	//each class must have ast east 4 vcs assigned or else xy_yx + min+ nonmin will deadlock
	int begin_vcs = -1;
	int available_vcs = 0;
	if ( f->type == Flit::READ_REQUEST ) {
		available_vcs = (gReadReqEndVC-gReadReqBeginVC)+1;
		begin_vcs = gReadReqBeginVC;
	} else if ( f->type == Flit::WRITE_REQUEST ) {
		available_vcs = (gWriteReqEndVC-gWriteReqBeginVC)+1;
		begin_vcs = gWriteReqBeginVC;
	} else if ( f->type ==  Flit::READ_REPLY ) {
		available_vcs = (gReadReplyEndVC-gReadReplyBeginVC)+1;
		begin_vcs = gReadReplyBeginVC;
	} else if ( f->type ==  Flit::WRITE_REPLY ) {
		available_vcs = (gWriteReplyEndVC-gWriteReplyBeginVC)+1;
		begin_vcs = gWriteReplyBeginVC;
	} else if ( f->type ==  Flit::ANY_TYPE ) {
		available_vcs = (gNumVCS);
		begin_vcs = 0;
	}

	assert( available_vcs>=4);
	int half_ava = available_vcs>>1;
	int quarter_ava = available_vcs>>2;
	if(f->ph ==1){
		vc_min = begin_vcs;
		vc_max =   begin_vcs+quarter_ava-1;
	}else{
		vc_min = begin_vcs+(quarter_ava);
		vc_max   = begin_vcs+half_ava-1;
	} 
	if(f->ring_par == 0){
		vc_min +=half_ava;
		vc_max += half_ava;
	}    
	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void valiant_ni_torus( const Router *r, const Flit *f, int in_channel, 
		OutputSet *outputs, bool inject )
{
	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		f->ph   = 1;  // Phase 1
		f->intm = RandomInt( gNodes - 1 );
	}

	if ( ( f->ph == 1 ) && ( r->GetID( ) == f->intm ) ) {
		f->ph = 2; // Go to phase 2
		in_channel = 2*gN; // ensures correct vc selection at the beginning of phase 2
	}

	if ( f->ph == 1 ) { // In phase 1
		dor_next_torus( r->GetID( ), f->intm, in_channel,
				&out_port, &f->ring_par, false );

		if ( f->ring_par == 0 ) {
			vc_min = f->dest;
			vc_max = f->dest;
		} else {
			vc_min = f->dest + gNodes;
			vc_max = f->dest + gNodes;
		}

	} else { // In phase 2
		dor_next_torus( r->GetID( ), f->dest, in_channel,
				&out_port, &f->ring_par, false );

		if ( f->ring_par == 0 ) {
			vc_min = f->dest + 2*gNodes;
			vc_max = f->dest + 2*gNodes;
		} else {
			vc_min = f->dest + 3*gNodes;
			vc_max = f->dest + 3*gNodes;
		}
	}

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< vc_min << "," 
			<< vc_max << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void dim_order_torus( const Router *r, const Flit *f, int in_channel, 
		OutputSet *outputs, bool inject )
{
	int cur;
	int dest;

	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );

	cur  = r->GetID( );
	dest = f->dest;

	dor_next_torus( cur, dest, in_channel,
			&out_port, &f->ring_par, false );

	int vc_class_min, vc_class_max ;
	switch(f->type) {
		case Flit::READ_REQUEST:
			vc_class_min = gReadReqBeginVC;
			vc_class_max = gReadReqEndVC;
			break;
		case Flit::WRITE_REQUEST:
			vc_class_min = gWriteReqBeginVC;
			vc_class_max = gWriteReqEndVC;
			break;
		case Flit::READ_REPLY:
			vc_class_min = gReadReplyBeginVC;
			vc_class_max = gReadReplyEndVC;
			break;
		case Flit::WRITE_REPLY:
			vc_class_min = gWriteReplyBeginVC;
			vc_class_max = gWriteReplyEndVC;
			break;
		case Flit::ANY_TYPE:
			vc_class_min = 0;
			vc_class_max = gNumVCS-1;
	}

	int vc_class_size = vc_class_max - vc_class_min ;
	if ( f->ring_par == 0 ) {
		vc_min = vc_class_min ;
		vc_max = vc_class_min + vc_class_size/2 ;
	} else {
		vc_min = vc_class_min + vc_class_size/2 + 1 ;
		vc_max = vc_class_min + vc_class_size ;
	} 

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< vc_min << "," 
			<< vc_max << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void dim_order_ni_torus( const Router *r, const Flit *f, int in_channel, 
		OutputSet *outputs, bool inject )
{
	int cur;
	int dest;

	int out_port;
	int vcs_per_dest = gNumVCS / gNodes;

	outputs->Clear( );

	cur  = r->GetID( );
	dest = f->dest;

	outputs->Clear( );
	dor_next_torus( cur, dest, in_channel,
			&out_port, &f->ring_par, false );

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< f->dest*vcs_per_dest << "," 
			<< (f->dest+1)*vcs_per_dest << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	outputs->AddRange( out_port, f->dest*vcs_per_dest, (f->dest+1)*vcs_per_dest - 1 );
}

//=============================================================

void dim_order_bal_torus( const Router *r, const Flit *f, int in_channel, 
		OutputSet *outputs, bool inject )
{
	int cur;
	int dest;

	int out_port;
	int vc_min, vc_max;

	int vc_class_min, vc_class_max ;

	outputs->Clear( );

	cur  = r->GetID( );
	dest = f->dest;

	dor_next_torus( cur, dest, in_channel,
			&out_port, &f->ring_par, true );

	// ( Traffic Class ) -> Virtual Channel Range
	switch(f->type) {
		case Flit::READ_REQUEST:
			vc_class_min = gReadReqBeginVC ;
			vc_class_max = gReadReqEndVC ;
			break;
		case Flit::WRITE_REQUEST:
			vc_class_min = gWriteReqBeginVC ;
			vc_class_max = gWriteReqEndVC ;
			break;
		case Flit::READ_REPLY:
			vc_class_min = gReadReplyBeginVC ;
			vc_class_max = gReadReplyEndVC ;
			break;
		case Flit::WRITE_REPLY:
			vc_class_min = gWriteReplyBeginVC ;
			vc_class_max = gWriteReplyEndVC ;
			break;
		case Flit::ANY_TYPE:
			vc_class_min = 0;
			vc_class_max = gNumVCS-1;
	}

	int vc_class_size = vc_class_max - vc_class_min ;
	if ( f->ring_par == 0 ) {
		vc_min = vc_class_min ;
		vc_max = vc_class_min + vc_class_size/2 ;
	} else {
		vc_min = vc_class_min + vc_class_size/2 + 1 ;
		vc_max = vc_class_min + vc_class_size ;
	} 

	if ( f->watch ) {
		*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
			<< "Adding VC range [" 
			<< vc_min << "," 
			<< vc_max << "]"
			<< " at output port " << out_port
			<< " for flit " << f->id
			<< " (input port " << in_channel
			<< ", destination " << f->dest << ")"
			<< "." << endl;
	}

	outputs->AddRange( out_port, vc_min, vc_max );
}

//=============================================================

void min_adapt_torus( const Router *r, const Flit *f, int in_channel, OutputSet *outputs, bool inject )
{
	int cur, dest, dist2;
	int in_vc;
	int out_port;

	outputs->Clear( );

	if ( in_channel == 2*gN ) {
		in_vc = gNumVCS - 1; // ignore the injection VC
	} else {
		in_vc = f->vc;
	}

	if ( in_vc > 1 ) { // If not in the escape VCs
		// Minimal adaptive for all other channels
		cur = r->GetID( ); dest = f->dest;

		for ( int n = 0; n < gN; ++n ) {
			if ( ( cur % gK ) != ( dest % gK ) ) {
				dist2 = gK - 2 * ( ( ( dest % gK ) - ( cur % gK ) + gK ) % gK );

				if ( dist2 > 0 ) { /*) || 
						     ( ( dist2 == 0 ) && ( RandomInt( 1 ) ) ) ) {*/
					outputs->AddRange( 2*n, 3, 3, 1 ); // Right
				} else {
					outputs->AddRange( 2*n + 1, 3, 3, 1 ); // Left
				}
				}

				cur  /= gK;
				dest /= gK;
			}

			// DOR for the escape channel (VCs 0-1), low priority --- 
			// trick the algorithm with the in channel.  want VC assignment
			// as if we had injected at this node
			dor_next_torus( r->GetID( ), f->dest, 2*gN,
					&out_port, &f->ring_par, false );
		} else {
			// DOR for the escape channel (VCs 0-1), low priority 
			dor_next_torus( r->GetID( ), f->dest, in_channel,
					&out_port, &f->ring_par, false );
		}

		if ( f->ring_par == 0 ) {
			outputs->AddRange( out_port, 0, 0, 0 );
		} else  {
			outputs->AddRange( out_port, 1, 1, 0 );
		} 

		if ( f->watch ) {
			*gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
				<< "Adding VC range [" 
				<< 0 << "," 
				<< gNumVCS - 1 << "]"
				<< " at output port " << out_port
				<< " for flit " << f->id
				<< " (input port " << in_channel
				<< ", destination " << f->dest << ")"
				<< "." << endl;
		}


	}

	//=============================================================

	void dest_tag( const Router *r, const Flit *f, int in_channel, 
			OutputSet *outputs, bool inject )
	{
		outputs->Clear( );

		int stage = ( r->GetID( ) * gK ) / gNodes;
		int dest  = f->dest;

		while( stage < ( gN - 1 ) ) {
			dest /= gK;
			++stage;
		}

		int out_port = dest % gK;

		outputs->AddRange( out_port, 0, gNumVCS - 1 );
	}



	//=============================================================

	void chaos_torus( const Router *r, const Flit *f, 
			int in_channel, OutputSet *outputs, bool inject )
	{
		int cur, dest;
		int dist2;

		outputs->Clear( );

		cur = r->GetID( ); dest = f->dest;

		if ( cur != dest ) {
			for ( int n = 0; n < gN; ++n ) {

				if ( ( cur % gK ) != ( dest % gK ) ) { 
					dist2 = gK - 2 * ( ( ( dest % gK ) - ( cur % gK ) + gK ) % gK );

					if ( dist2 >= 0 ) {
						outputs->AddRange( 2*n, 0, 0 ); // Right
					} 

					if ( dist2 <= 0 ) {
						outputs->AddRange( 2*n + 1, 0, 0 ); // Left
					}
				}

				cur  /= gK;
				dest /= gK;
			}
		} else {
			outputs->AddRange( 2*gN, 0, 0 ); 
		}
	}


	//=============================================================

	void chaos_mesh( const Router *r, const Flit *f, 
			int in_channel, OutputSet *outputs, bool inject )
	{
		int cur, dest;

		outputs->Clear( );

		cur = r->GetID( ); dest = f->dest;

		if ( cur != dest ) {
			for ( int n = 0; n < gN; ++n ) {
				if ( ( cur % gK ) != ( dest % gK ) ) { 
					// Add minimal direction in dimension 'n'
					if ( ( cur % gK ) < ( dest % gK ) ) { // Right
						outputs->AddRange( 2*n, 0, 0 ); 
					} else { // Left
						outputs->AddRange( 2*n + 1, 0, 0 ); 
					}
				}
				cur  /= gK;
				dest /= gK;
			}
		} else {
			outputs->AddRange( 2*gN, 0, 0 ); 
		}
	}
	//added shankar
	/*
	   /
	   void dim_order_torus( Router *r, priority_queue<order> * flit_order)
	//const Router *r, const Flit *f, int in_channel,
	//   OutputSet *outputs, bool inject )
	{
	int cur;
	int dest;

	int out_port;
	int vc_min, vc_max;

	outputs->Clear( );

	cur  = r->GetID( );
	dest = f->dest;

	dor_next_torus( cur, dest, in_channel,
	&out_port, &f->ring_par, false );

	int vc_class_min, vc_class_max ;
	switch(f->type) {
	case Flit::READ_REQUEST:
	vc_class_min = gReadReqBeginVC;
	vc_class_max = gReadReqEndVC;
	break;Measured flits: 2685 2687 2690 2703 2727 2737 2738 2776 2784 2793 (10 flits, 10 packets)
WARNING: Possible network deadlock     time=144105.
WARNING: Possible network deadlock     time=144361.
WARNING: Possible network deadlock     time=144617.

case Flit::WRITE_REQUEST:
vc_class_min = gWriteReqBeginVC;
vc_class_max = gWriteReqEndVC;
break;
case Flit::READ_REPLY:
vc_class_min = gReadReplyBeginVC;
vc_class_max = gReadReplyEndVC;
break;
case Flit::WRITE_REPLY:
vc_class_min = gWriteReplyBeginVC;
vc_class_max = gWriteReplyEndVC;
break;
case Flit::ANY_TYPE:
vc_class_min = 0;
vc_class_max = gNumVCS-1;
}

int vc_class_size = vc_class_max - vc_class_min ;
if ( f->ring_par == 0 ) {
vc_min = vc_class_min ;
vc_max = vc_class_min + vc_class_size/2 ;
} else {
vc_min = vc_class_min + vc_class_size/2 + 1 ;
vc_max = vc_class_min + vc_class_size ;
}

if ( f->watch ) {
	 *gWatchOut << GetSimTime() << " | " << r->FullName() << " | "
	 << "Adding VC range ["
	 << vc_min << ","
	 << vc_max << "]"
	 << " at output port " << out_port
	 << " for flit " << f->id
	 << " (input port " << in_channel
	 << ", destination " << f->dest << ")"
	 << "." << endl;
	 }

	 outputs->AddRange( out_port, vc_min, vc_max );
	 }
	 void dor_next_torus( int cur, int dest, int in_port,
int *out_port, int *partition,
    bool balance = false )
{
	int dim_left;
	int dir;
	int dist2;

	for ( dim_left = 0; dim_left < gN; ++dim_left ) {
		if ( ( cur % gK ) != ( dest % gK ) ) { break; }
		cur /= gK; dest /= gK;
	}

	if ( dim_left < gN ) {

		if ( (in_port/2) != dim_left ) {
			// Turning into a new dimension

			cur %= gK; dest %= gK;
			dist2 = gK - 2 * ( ( dest - cur + gK ) % gK );

			if ( ( dist2 > 0 ) ||
					( ( dist2 == 0 ) && ( RandomInt( 1 ) ) ) ) {
				*out_port = 2*dim_left;     // Right
				dir = 0;
			} else {
				*out_port = 2*dim_left + 1; // Left
				dir = 1;
			}

			if ( balance ) {
				// Cray's "Partition" allocation
				// Two datelines: one between k-1 and 0 which forces VC 1
				//                another between ((k-1)/2) and ((k-1)/2 + 1) which forces VC 0
				//                otherwise any VC can be used

				if ( ( ( dir == 0 ) && ( cur > dest ) ) ||
						( ( dir == 1 ) && ( cur < dest ) ) ) {
					*partition = 1;
				} else if ( ( ( dir == 0 ) && ( cur <= (gK-1)/2 ) && ( dest >  (gK-1)/2 ) ) ||
						( ( dir == 1 ) && ( cur >  (gK-1)/2 ) && ( dest <= (gK-1)/2 ) ) ) {
					*partition = 0;
				} else {
					*partition = RandomInt( 1 ); // use either VC set
				}
			} else {
				// Deterministic, fixed dateline between nodes k-1 and 0

				if ( ( ( dir == 0 ) && ( cur > dest ) ) ||
						( ( dir == 1 ) && ( dest < cur ) ) ) {
					*partition = 1;
				} else {
					*partition = 0;
				}
			}
		} else {
			// Inverting the least significant bit keeps
			// the packet moving in the same direction
			*out_port = in_port ^ 0x1;
		}

	} else {
		*out_port = 2*gN;  // Eject
	}
}
*/ 
/*
   struct assign
   {
   int p1;
   int p2;
   int p3;
   int p4;
   };
   typedef struct assign assign;
   assign bless_next_mesh(int cur,int dest, int from_router, Router* r, int from_port)
   {
   struct assign A;
   int c=cur,d=dest;
   int out_port;
   int dim_left;
   if(cur==dest)
   {
   A.p1=4;
   return A;
   }
   int k,z;
   int x1,y1,x2,y2;	//(x1,y1) curr, // (x2,y2) dest
   x1=cur%gK;
   y1=cur/gK;
   x2=dest%gK;
   y2=dest/gK;
   int xdiff,ydiff;
   xdiff=x1-x2;
   ydiff=y1-y2;
   if(from_port==0 || from_port==1)
   A.p4=abs(from_port-1);
   else if(from_port==2)
   A.p4=3;
   else if(from_port==3)
   A.p4=2;
   else 
   A.p4=-1;
   if(abs(xdiff)>abs(ydiff))
   {
   if((x1-x2)<0)
   A.p1=0;
   else
   A.p1=1;
   if((y1-y2)<0)
   A.p2=2;
   else if((y1-y2)>0)
   A.p2=3;
   else
   A.p2=-1;
   }
   else
   {
   if((y1-y2)<0)
   A.p1=2;
   else
   A.p1=3;
   if((x1-x2)<0)
   A.p2=0;
   else 
   A.p2=1;
   }
   A.p3=6-(A.p1+A.p2+A.p4);

   k=pow(2,A.p3);
   z=r->use_const;
   z=z&15;
   if(k!=(k&z))
   A.p3=-1;
   k=pow(2,A.p1);
   z=r->use_const;
   z=z&15;
if(k!=(k&z))
	A.p1=-1;
	k=pow(2,A.p2);
	z=r->use_const;
	z=z&15;
if(k!=(k&z))
	A.p2=-1;	
	_out_pri[A.p1]=true;	  
	return A;
	}
//flit bless added shankar

priority_queue<order> second_iter;
void dor_bless_mesh( Router *r, priority_queue<order> * flit_order)
{

	int _alloc;
	for(int i=0;i<5;i++)
	{
		_out_alloc[i]=false;
		_out_pri[i]=false;
	}
	struct assign a1[4];
	int j=0;
	while(!flit_order->empty())
	{
		order o=flit_order->top();
		a1[j++]=bless_next_mesh(r->GetID(),o.f->dest,o.f->from_router,r,o.f->cur_port);
		flit_order->pop();
	}
	int k=0;
	struct assign a2;
	while(!second_iter.empty())
	{
		order o=second_iter.top();
		a2=a1[k++];
		int _alloc;
		if((a2.p1!=-1) && !_out_alloc[a2.p1] )
		{
			_alloc=a2.p1;
			_out_alloc[a2.p1]=true;
		}
		else if((a2.p2!=-1) && (!_out_alloc[a2.p2]) && !_out_pri[a2.p2])
		{	
			_alloc=a2.p2;
			_out_alloc[a2.p2]=true;
		}
		else if((a2.p3!=-1) && (!_out_alloc[a2.p3]) && !_out_pri[a2.p3] )
		{
			_alloc=a2.p3;
			_out_alloc[a2.p3]=true;
		}
		else if(!_out_alloc[a2.p4] && !_out_pri[a2.p4])
		{
			_alloc=a2.p4;
			_out_alloc[a2.p4]=true;
		}
		else
		{
			cout<<"\n Error no port available\n";
			exit(0);
		}
		o.vc->_route_set->Clear();
		if (o.f->type == Flit::READ_REQUEST)
			o.vc->_route_set->AddRange( _alloc, gReadReqBeginVC, gReadReqEndVC );
		else if (o.f->type == Flit::WRITE_REQUEST)
			o.vc->_route_set->AddRange( _alloc, gWriteReqBeginVC, gWriteReqEndVC );
		else if (o.f->type ==  Flit::READ_REPLY)
			o.vc->_route_set->AddRange( _alloc, gReadReplyBeginVC, gReadReplyEndVC );
		else if (o.f->type ==  Flit::WRITE_REPLY)
			o.vc->_route_set->AddRange( _alloc, gWriteReplyBeginVC, gWriteReplyEndVC );
		else if (o.f->type ==  Flit::ANY_TYPE)
			o.f->cur_port=_alloc;
		switch(_alloc)
		{
			case 0: r->channel_use.push_back(r->GetID()+1); r->channel_use.push_back(0);break;//cout<<r->GetID()+1<<"    "<<r->GetID()<<"\n"; break;
			case 1: r->channel_use.push_back(r->GetID()-1);r->channel_use.push_back(1); break;//cout<<r->GetID()-1<<"    "<<r->GetID()<<"\n"; break;	
			case 2: r->channel_use.push_back(r->GetID()+gK); r->channel_use.push_back(2);break;// cout<<r->GetID()+gK<<"    "<<r->GetID()<<"\n"; break;
			case 3: r->channel_use.push_back(r->GetID()-gK);r->channel_use.push_back(3);break;// cout<<r->GetID()-gK<<"    "<<r->GetID()<<"\n";  break;
		}
		second_iter.pop();
	}

}
//--end of flit bless
*/

struct assign
{
	int p1;
	int p2;
	int p3;
	int p4;
};
typedef struct assign assign;
assign a2[4];
int alpha;


int bless_next_mesh(int cur,int dest, int from_router, Router* r, Flit *f)
{
	int c=cur,d=dest;
	int out_port;
	int dim_left;

	for (dim_left = 0; dim_left < gN; ++dim_left )
	{

		if ( ( cur % gK ) != ( dest % gK ) )  //current and destination columns are not matching
		{
			break;
		}
		cur /= gK; dest /= gK;
	}

	if ( dim_left < gN )
	{
		cur %= gK; dest %= gK;
		if ( cur < dest )
			out_port = 2*dim_left;     // Right  //East:0, West:1, South:2, North:3, Eject:4

		else
			out_port = 2*dim_left + 1; // Left

	}

	else
	{
		out_port = 2*gN;  // Eject
	}

//cout<<endl<<out_port<<"tried for"<<f->id;
		
	

//	if(!_out_alloc[out_port])
//	{
//		_out_alloc[out_port]=true;
		return out_port;
//	}
/*
	else //if contention assign a port randomly
	{
//		deflections++;
		while(1)
		{
			out_port=RandomInt(3);
	//		if(f->id==62)
{	//		cout<<r->GetID()<<"|"<<f->id<<"|"<<f->src<<"|"<<f->dest<<"|"<<out_port<<endl;
//getchar();		
}			int k=pow(2,out_port);
			int z=r->use_const;
			z = z&15;

			if(k==(k&z))
			{
				if(!_out_alloc[out_port] && out_port!=2*gN)
				{
					_out_alloc[out_port] = true;
				
//cout<<sb_size;		getchar();			
deflections++;
					f->defl = true;
		
					return out_port;
				}
			}
		
	
		}
	}
*/
}



void dor_bless_mesh( Router *r, priority_queue<order> * flit_order)	//Permutaion network logic
{

int _alloc, first=1;       //alloc is to contain the desired outport of the flit calculated from bless_mesh_next()


list<Flit *>NE;
list<Flit *>SW;
list<Flit *>NS;
list<Flit *>EW;
list<Flit *>Defl;
list<Flit *>non_defl;

//cout<<"\nRouter"<<r->GetID()<<endl;
r->flitbuff2.clear();

int avail_ports[4]={1,1,1,1};  //E,W,N,S

//*INFD
int s = (gK-1);

//For Edges and corners, what are the available ports


if(r->GetID()/gK == 0)
avail_ports[3]=0;
else if(r->GetID()/gK == s)
avail_ports[2]=0;
if(r->GetID()%gK == 0)
avail_ports[1]=0;
else if(r->GetID()%gK == s)
avail_ports[0]=0;
//INFD*/

//####INFD CODE
int faultmap = GetRouterFault(r->GetID());
for(int ix = 0; ix < 4 ; ix++){
	avail_ports[ix] = faultmap%2;
	faultmap/=2;
}
//####INFD ENDS


	for(int i=0;i<4;i++)
	{
		_out_alloc[i]=false;
	}

	while(!flit_order->empty())	//Getting required port
	{
		order o=flit_order->top();
		_alloc = bless_next_mesh(r->GetID(),o.f->dest,o.f->from_router,r,o.f);
		if(first==1 && _Mod_MinBD == "true" )	//Select first flit as silver flit
		{
			first++;
			o.f->silver = true;
		}
		o.f->get_port=_alloc;

		//North=3, East=0, West =1, South=2

		if(o.f->port == 3 || o.f->port == 0)
		  {NE.push_back(o.f);}
		else if(o.f->port == 2 || o.f->port == 1)
		  SW.push_back(o.f);

		flit_order->pop();
	} //end of while

//To keep silver flit in front of the list

		if( NE.size()>0 )
		if( NE.front()->silver == false && NE.size()==2 )
		{
		  NE.push_back(NE.front());
    		  NE.pop_front();
		}
		if( SW.size()>0 )
		if(SW.front()->silver == false && SW.size()==2)
		{
		  SW.push_back(SW.front());
    		  SW.pop_front();
		}

//Arbiter1
		if( NE.size()>0 )
		{
		if( NE.front()->get_port == 3 || NE.front()->get_port == 2) 
		{
		  NS.push_back(NE.front()); 
		  if(NE.size()==2)
		  EW.push_back(NE.back());
		}
		else if( NE.front()->get_port == 0 || NE.front()->get_port == 1 )
		{
		  EW.push_back(NE.front());
		  if(NE.size()==2)
		  NS.push_back(NE.back());
		}
		}

//Arbiter2
		if( SW.size()>0 )
		{
		if( SW.front()->get_port == 3 || SW.front()->get_port == 2) 
		{
		  NS.push_back(SW.front()); 
		  if(SW.size()==2)
		  EW.push_back(SW.back());
		}
		else if( SW.front()->get_port == 0 || SW.front()->get_port == 1)
		{
		  EW.push_back(SW.front());
		  if(SW.size()==2)
		  NS.push_back(SW.back());
		}
		}
NE.clear();
SW.clear();
//To keep silver flit in front of the list
		if( NS.size()>0 )
		if( NS.front()->silver == false && NS.size()==2 )
		{
		  NS.push_back(NS.front());
    		  NS.pop_front();
		}
		if( EW.size()>0 )
		if(EW.front()->silver == false && EW.size()==2)
		{
		  EW.push_back(EW.front());
    		  EW.pop_front();
		}
//Arbiter3		
		if(NS.size()>0)
		{
		if(NS.front()->get_port == 3 )
		{
		  {NS.front()->cur_port =  3;_out_alloc[3]=true;}
		  if(NS.size()==2)
		  {NS.back()->cur_port = 2;_out_alloc[2]=true;}
		}
		else
		{
		  {NS.front()->cur_port =  2;_out_alloc[2]=true;}
		  if(NS.size()==2)
		  {NS.back()->cur_port = 3;_out_alloc[3]=true;}
		}
		}
//Arbiter4
		if( EW.size()>0)
		{
		if(EW.front()->get_port == 0)
		{
		  {EW.front()->cur_port =  0;_out_alloc[0]=true;}
		  if(EW.size()==2)
		  {EW.back()->cur_port = 1;_out_alloc[1]=true;}
		}
		else
		{
		  {EW.front()->cur_port =  1;_out_alloc[1]=true;}
		  if(EW.size()==2)
		  {EW.back()->cur_port = 0;_out_alloc[0]=true;}	
		}
		}

//Arbiter stages completed

list<Flit *>::iterator it;
list<Flit *>::iterator it1;

//Deflected, non_deflected flits & Side buffer

  Flit *f;

  for ( it=NS.begin() ; it != NS.end(); it++ )
  {
    f = *it;
	int i = -1;
	if( avail_ports[f->cur_port] == 0 )
	{
		while( avail_ports[f->cur_port]!= 1 || _out_alloc[f->cur_port] == true )
		f->cur_port = ++i;
 
	}

	if( i<4 )
	_out_alloc[i] = true;

	
    if( f->get_port != f->cur_port )
    {	
	deflections++;
	f->ndefl++;
	Defl.push_back(f);
    }
    else
	non_defl.push_back(f);
  }

  for ( it1=EW.begin() ; it1 != EW.end(); it1++ )
  {
    f = *it1;

	int i = -1;
	if( avail_ports[f->cur_port] == 0 )
	{
		while( avail_ports[f->cur_port] != 1 || _out_alloc[f->cur_port]==true )
		f->cur_port = ++i; 
	}
	if( i<4 )
	_out_alloc[i] = true;

    if( f->get_port != f->cur_port )
    {	
	deflections++;
	f->ndefl++;
	Defl.push_back(f);
    }
    else
	{non_defl.push_back(f);}
   }
   NS.clear();
   EW.clear();

   int i=0;
   int a=RandomInt(Defl.size());
   list<Flit *>::iterator it2;

if(_Mod_MinBD == "true")	//Selecting deflected flit for side buffer
{
int temp = 0;
 for ( it2=Defl.begin() ; it2 != Defl.end(); it2++ )
  {
	f = *it2;
	if( f->hop_dist < temp )
	{
		a = i;
		temp = f->hop_dist;
	}
	i++;
  }
}

i = 0;
//Moving random flit to side_buffer
  for ( it2=Defl.begin() ; it2 != Defl.end(); it2++ )
  {
	f = *it2;
	if( a == i && r->sidebuff.size()<sb_size )
	{
	r->sidebuff.push(f);
//	if(  r->sidebuff.size() >= 3 )
//	{cout<<r->GetID()<<" size: "<<r->sidebuff.size()<<endl;getchar();}
	}
	else
	non_defl.push_back(f);	
	i++;
  }

//Other flits to channels
  list<Flit *>::iterator it3;
  for ( it3=non_defl.begin() ; it3 != non_defl.end(); it3++ )
  {
	f = *it3;
	if( f->silver == true )
	f->silver = false;
//cout<<"\nFlit: "<<f->id<<" Port"<<f->cur_port<<" Dest"<<f->dest<<" dfadf"<<f->get_port<<endl;
	switch(f->cur_port)
	{
	case 0: r->channel_use.push_back(r->GetID()+1); r->channel_use.push_back(0);break;//cout<<r->GetID()+1<<"    "<<r->GetID()<<"\n"; break;
	case 1: r->channel_use.push_back(r->GetID()-1);r->channel_use.push_back(1); break;//cout<<r->GetID()-1<<"    "<<r->GetID()<<"\n"; break;
	case 2: r->channel_use.push_back(r->GetID()+gK); r->channel_use.push_back(2);break;// cout<<r->GetID()+gK<<"    "<<r->GetID()<<"\n"; break;
	case 3: r->channel_use.push_back(r->GetID()-gK);r->channel_use.push_back(3);break;// cout<<r->GetID()-gK<<"    "<<r->GetID()<<"\n";  break;
	}
	r->flitbuff2.push_back(f);
  }

}

/*
//flit bless added shankar
void dor_bless_mesh( Router *r, priority_queue<order> * flit_order)	//BLESS sequntial port allocator
{

	//r->channel_use=new vector<int>;
	int _alloc, temp=sb_size, del;  
	bool state=false;
  Flit *rem_flit = new Flit();
  VC *from_vc;


//if( r->GetID()/gK == 0 || r->GetID()/gK == (gK-1) || r->GetID()%gK == 0 || r->GetID()%gK == (gK-1))
//temp = sb_size-2;

//cout<<" \n Router:"<<r->GetID()<<endl;

//if( r->GetID()/gK == 0)
//{
//if(r->GetID()%gK == 0 || r->GetID()%gK==(gK-1))
//temp = 0;
//}
//else if(r->GetID()/gK == gK-1 )
//{
//if( r->GetID()%gK == 0 || r->GetID()%gK == (gK-1))
//temp = 0;
//}

//if(  r->GetID()/gK != 0 && r->GetID()/gK != (gK-1) && r->GetID()%gK != 0 && r->GetID()%gK != (gK-1))
//temp = sb_size+2;

//cout<<" \nsize:"<<temp;getchar();

	for(int i=0;i<5;i++)
	{
		_out_alloc[i]=false;
	}
	_out_alloc[4]=true;


list<Flit *>::iterator it;
//it = r->flitbuff2.begin();
r->flitbuff2.clear();

	while(!flit_order->empty())
	{
		order o=flit_order->top();

//cout<<"\nRouting: "<<o.f->id<<endl;

		_alloc = bless_next_mesh(r->GetID(),o.f->dest,o.f->from_router,r,o.f);

//if(o.f->id == 1000)
//{cout<<GetSimTime()<<" Router"<<r->GetID()<<" ::Routing flit "<<o.f->id<<" to "<<_alloc<<" src:"<<o.f->src<<" dest:"<<o.f->dest<<endl;}//getchar();}


	  if( state==false && o.f->defl==true && r->sidebuff.size()<temp )
	  {  
		//r->flitbuff2.push_back(o.f);

		deflections--;
		rem_flit = o.f;
		state = true;

//		cout<<rem_flit->id<<" removed";

		r->sidebuff.push(o.f);		//Sending into side buffer
		o.f->defl = false;
  	  }

	  else	//Side buffer is full
	  {
		r->flitbuff2.push_back(o.f);
//	  	cout<<"\nSide buffer is full:"<<r->sidebuff.size();
		o.f->defl = false;
//cout<<r->GetID()<<"|"<<o.f->id<<"|"<<o.f->src<<"|"<<o.f->dest<<"|"<<_alloc<<endl;

		o.f->cur_port=_alloc;
	
		switch(_alloc)
		{
		case 0: r->channel_use.push_back(r->GetID()+1); r->channel_use.push_back(0);break;//cout<<r->GetID()+1<<"    "<<r->GetID()<<"\n"; break;
		case 1: r->channel_use.push_back(r->GetID()-1);r->channel_use.push_back(1); break;//cout<<r->GetID()-1<<"    "<<r->GetID()<<"\n"; break;
		case 2: r->channel_use.push_back(r->GetID()+gK); r->channel_use.push_back(2);break;// cout<<r->GetID()+gK<<"    "<<r->GetID()<<"\n"; break;
		case 3: r->channel_use.push_back(r->GetID()-gK);r->channel_use.push_back(3);break;// cout<<r->GetID()-gK<<"    "<<r->GetID()<<"\n";  break;
		}
	  }
//		if(it!=r->flitbuff2.end())
//		++it;
	flit_order->pop();
	}
//Flit *f;
//for ( it=r->flitbuff2.begin() ; it != r->flitbuff2.end(); it++ )
//{
//if( (*it)->id == rem_flit->id )
//r->flitbuff2.erase(it);
//}

}
*/

/*
void dor_bless_mesh( Router *r, priority_queue<order> * flit_order)
{

  int dual = 0, Eje_band = 1;
  bool sbuff = false;
  bool sone = false;	// Only one flit will be sent to side buff
  int _alloc;
  
  queue<Flit *> Flit_queue;
  queue<VC *> Vc_queue;

  queue<Flit *> Flit_temp;
  queue<VC *> Vc_temp;

  Flit *rem_flit = new Flit();
  VC *from_vc;

  int x1,y1,x2,y2;
  x1 = r->GetID()/gK;
  y1 = r->GetID()%gK;

//cout<<"\nRouter:"<<r->GetID()<<endl;

	for(int i=0;i<4;i++)
	{
		_out_alloc[i]=false;
	}
_out_alloc[4]=true;
//cout<<"\nTotal size:"<<flit_order->size();

//Allocating the available ports for the ports for the flits

	while(!flit_order->empty())
	{
	  order o = flit_order->top();
	  o.vc->_route_set->Clear();
	
//	cout<<o.f->ndefl<<endl;

	  if( o.f->dest == r->GetID() && dual < Eje_band ) 		//Dual Ejection
	  {
		_alloc = 4;
//	cout<<o.f->id<<" || "<<o.f->dest<<" ejecting"<<endl;
		o.f->get_port = _alloc;
		Flit_queue.push(o.f);
		Vc_queue.push(o.vc);

		if( dual == Eje_band-1 )
		_out_alloc[4] = true;	
		dual++;				
	  }

	  else 
	  {
		_alloc = bless_next_mesh(r->GetID(),o.f->dest,o.f->from_router,r,o.f); //find the port
		
		if( o.f->defl == true )	//For defl flit do not allocate port now
		{
		  o.f->ndefl = o.f->ndefl+1;

		  _out_alloc[_alloc] = false;  //Do not allocate this port now

		  Flit_temp.push( o.f );	//Find the port later
		  Vc_temp.push( o.vc );
		}

		else		//Required port is allocated
		{
		  o.f->get_port = _alloc;  

		  Flit_queue.push(o.f);
		  Vc_queue.push(o.vc);
		}

	  }

	flit_order->pop();
	}
//cout<<"\nTotal:"<<deflections;
//cout<<"\nDefl size:"<<Flit_temp.size();
/************************************* Real MinBD *********************************************

 queue<Flit *> Flits;
 queue<VC *> Vcs;

	while( !Flit_temp.empty())	//For all Deflected flits
	{
	   Flit *f = Flit_temp.front();Flit_temp.pop();
	   VC *vc = Vc_temp.front();Vc_temp.pop();

		deflections++;	//Total no of deflections
		f->defl = false;

		if( f->dest!=r->GetID() )
		{
		  Flits.push(f);	
		  Vcs.push(vc);
		}
		else
		{

x2=f->dest/gK;
y2=f->dest%gK;
if( (abs(x2-x1)+abs(y2-y1)) <= 2 )
two_dist++; 

		  _alloc = bless_next_mesh(r->GetID(),f->dest,f->from_router,r,f); //find the port
		  f->get_port = _alloc;
		  f->defl = false;

		  Flit_queue.push(f);	
		  Vc_queue.push(vc);
		}
		
	}	

//Now all deflected flits are in Flits queue

	int randf;

	srand(time(NULL));  // Initialize random number generator.
	if(!Flits.empty())
	randf = (rand() % Flits.size()) + 1;
//	cout<<randf<<endl;

	int n = 1;

	while( !Flits.empty())	//For all Deflected flits
	{
	   Flit *f = Flits.front();Flits.pop();
	   VC *vc = Vcs.front();Vcs.pop();

		_alloc = bless_next_mesh(r->GetID(),f->dest,f->from_router,r,f); //find the port
		f->get_port = _alloc;
		f->defl = false;

		if( (n==randf) )	//Qualified for side buffer
		{
		   sone = true;

		   rem_flit = f;
		   from_vc = vc;
		}
		else
		{

x2=f->dest/gK;
y2=f->dest%gK;
if( (abs(x2-x1)+abs(y2-y1)) <= 1 )
two_dist++; 
		   Flit_queue.push(f);
	  	   Vc_queue.push(vc);
		}
		n++;
	}

/************************************** Real MinBD *********************************************/
/************************************ Priority MinBD *******************************************/
//The flits which doesn't get the required ports
/*
	while( !Flit_temp.empty())	//For all Deflected flits
	{
	Flit *f = Flit_temp.front();Flit_temp.pop();
	VC *vc = Vc_temp.front();Vc_temp.pop();

	deflections++;	//Total no of deflections

		_alloc = bless_next_mesh(r->GetID(),f->dest,f->from_router,r,f); //find the port
		
		f->get_port = _alloc;
		f->defl = false;
		if( f->dest!=r->GetID() ) // For each defl flit, find which fliit may goes to side buffer 
		{
			if( sone == false )
			{
				sone = true;
//cout<<"\nPrev:"<<rem_flit->id<<" Pri:"<<rem_flit->pri<<" New:"<<f->id<<" Pri:"<<f->pri<<endl;//getchar();

				rem_flit = f;
				from_vc = vc;
			}
			else if( sone==true && rem_flit->ndefl < f->ndefl )
			{

//cout<<"\nPrev:"<<rem_flit->id<<" Pri:"<<rem_flit->pri<<" New:"<<f->id<<" Pri:"<<f->pri<<endl;getchar();

				Flit_queue.push(rem_flit);	//Pushing the old flit with low priority
				Vc_queue.push(from_vc);

				rem_flit = f;			//Remove flit is new flit
				from_vc = vc;
			}
			else
			{
//			cout<<"\nPrev:"<<rem_flit->id<<" Pri:"<<rem_flit->pri<<" New:"<<f->id<<" Pri:"<<f->pri<<endl;getchar();

			   Flit_queue.push(f);
		  	   Vc_queue.push(vc);
			}
		}
		else
		{
		  Flit_queue.push(f);
		  Vc_queue.push(vc);
		}
		
	}
/************************************ Priority MinBD *******************************************
// Moving to Side buffer
	if( sone == true )
	{ 
	  if(r->sidebuff.size()<sb_size )
	  {  
//cout<<"\nR:"<<r->GetID()<<" |F:"<<rem_flit->id<<" |S:"<<rem_flit->src<<" |D:"<<rem_flit->dest<<" |P:"<<rem_flit->get_port;//getchar();
//cout<<"\n1 side";if( GetSimTime()==100 )getchar();

//		cout<<"sbsize"<<r->sidebuff.size()<<" || "<<sb_size;//getchar();  

	  	  rem_flit->get_port = -1;
		  deflections--;//Since it is not deflecting

		  rem_flit->ndefl = rem_flit->ndefl-1;

		  from_vc->RemoveFlit();		//Removing from VC
		  r->sidebuff.push(rem_flit);		//Sending into side buffer
  	  }
	  else	//Side buffer is full
	  {
//	  	cout<<"\nSide buffer is full:"<<r->sidebuff.size();
x2=rem_flit->dest/gK;
y2=rem_flit->dest%gK;
if( (abs(x2-x1)+abs(y2-y1)) <= 1 )
two_dist++; 
		Flit_queue.push(rem_flit);
	  	Vc_queue.push(from_vc);
	  }
	}

	Flit *f;
	VC *vc;

//if(Flit_queue.size()>=1)
//{cout<<"\nRemaining:"<<Flit_queue.size();}//getchar();}

//Completing the allocation
	while (!Flit_queue.empty())
  	{

  	  f = Flit_queue.front();  Flit_queue.pop();
	  vc = Vc_queue.front();   Vc_queue.pop();	

cout<<r->GetID()<<"|Flit:"<<f->id<<"|src:"<<f->src<<"|dest:"<<f->dest<<"|port:"<<f->get_port<<endl;

		if (f->type == Flit::READ_REQUEST)
			vc->_route_set->AddRange( f->get_port, gReadReqBeginVC, gReadReqEndVC );
		else if (f->type == Flit::WRITE_REQUEST)
			vc->_route_set->AddRange( f->get_port, gWriteReqBeginVC, gWriteReqEndVC );
		else if (f->type ==  Flit::READ_REPLY)
			vc->_route_set->AddRange( f->get_port, gReadReplyBeginVC, gReadReplyEndVC );
		else if (f->type ==  Flit::WRITE_REPLY)
			vc->_route_set->AddRange( f->get_port, gWriteReplyBeginVC, gWriteReplyEndVC );
		else if (f->type ==  Flit::ANY_TYPE)
			vc->_route_set->AddRange( f->get_port, 0, gNumVCS-1 );
		
		f->cur_port = f->get_port;

		switch( f->cur_port )
		{
		case 0: r->channel_use.push_back(r->GetID()+1); r->channel_use.push_back(0);break;//cout<<r->GetID()+1<<"    "<<r->GetID()<<"\n"; break;
		case 1: r->channel_use.push_back(r->GetID()-1);r->channel_use.push_back(1); break;//cout<<r->GetID()-1<<"    "<<r->GetID()<<"\n"; break;
		case 2: r->channel_use.push_back(r->GetID()+gK); r->channel_use.push_back(2);break;// cout<<r->GetID()+gK<<"    "<<r->GetID()<<"\n"; break;
		case 3: r->channel_use.push_back(r->GetID()-gK);r->channel_use.push_back(3);break;// cout<<r->GetID()-gK<<"    "<<r->GetID()<<"\n";  break;
		}
	  f->get_port = -1;  
	}

}


*/
//=============================================================

void InitializeRoutingMap( )
{
	/* Register routing functions here */   //read comment given in the top of this file to know format and how to register.

	// ===================================================
	// Balfour-Schultz
	gRoutingFunctionMap["nca_fattree"]         = &fattree_nca;
	gRoutingFunctionMap["anca_fattree"]        = &fattree_anca;
	gRoutingFunctionMap["nca_qtree"]           = &qtree_nca;
	gRoutingFunctionMap["nca_tree4"]           = &tree4_nca;
	gRoutingFunctionMap["anca_tree4"]          = &tree4_anca;
	gRoutingFunctionMap["dor_mesh"]            = &dor_mesh;
	gRoutingFunctionMap["xy_yx_mesh"]          = &xy_yx_mesh;
	gRoutingFunctionMap["dor_isolated_mesh"]   = &dor_mesh;
	gRoutingFunctionMap["xy_yx_isolated_mesh"] = &xy_yx_mesh;
	gRoutingFunctionMap["dest_tag_crossbar"]   = &dest_tag_crossbar ;
	// End Balfour-Schultz
	// ===================================================

	gRoutingFunctionMap["single_single"]   = &singlerf;

	gRoutingFunctionMap["dim_order_mesh"]  = &dim_order_mesh;
	gRoutingFunctionMap["dim_order_ni_mesh"]  = &dim_order_ni_mesh;
	gRoutingFunctionMap["dim_order_torus"] = &dim_order_torus;
	gRoutingFunctionMap["dim_order_ni_torus"] = &dim_order_ni_torus;
	gRoutingFunctionMap["dim_order_bal_torus"] = &dim_order_bal_torus;

	gRoutingFunctionMap["romm_mesh"]       = &romm_mesh; 
	gRoutingFunctionMap["romm_ni_mesh"]    = &romm_ni_mesh;

	gRoutingFunctionMap["min_adapt_mesh"]   = &min_adapt_mesh;
	gRoutingFunctionMap["min_adapt_torus"]  = &min_adapt_torus;

	gRoutingFunctionMap["planar_adapt_mesh"] = &planar_adapt_mesh;

	gRoutingFunctionMap["limited_adapt_mesh"] = &limited_adapt_mesh;

	gRoutingFunctionMap["valiant_mesh"]  = &valiant_mesh;
	gRoutingFunctionMap["valiant_torus"] = &valiant_torus;
	gRoutingFunctionMap["valiant_ni_torus"] = &valiant_ni_torus;

	gRoutingFunctionMap["dest_tag_fly"] = &dest_tag;

	gRoutingFunctionMap["chaos_mesh"]  = &chaos_mesh;
	gRoutingFunctionMap["chaos_torus"] = &chaos_torus;
	bRoutingFunctionMap["dor_bless_mesh"]=  &dor_bless_mesh;
	cout<<"routemap initialized...."<<endl; // added John
}

void * GetRoutingFunction( const Configuration& config )
{
	string b;
	void *rf;

	config.GetStr("bless",b);

	sb_size = config.GetInt("side_buffersize");

	config.GetStr("Mod_MinBD",_Mod_MinBD);
	if(_Mod_MinBD == "true")
	sb_size = sb_size-1; 	
	

	string fn, topo, fn_topo;
	config.GetStr( "routing_function", fn, "none" );
	config.GetStr( "topology", topo );
	fn_topo = fn + "_" + topo;
	if(b=="false")
	{
		map<string, tRoutingFunction>::const_iterator match;

		match = gRoutingFunctionMap.find( fn_topo );

		if ( match != gRoutingFunctionMap.end( ) )
		{
			rf =(void *) match->second;
		}
		else
		{
			if ( fn == "none" )
			{
				cout << "Error: No routing function specified in configuration." << endl;
			}
			else
			{
				cout << "Error: Undefined routing function '" << fn << "' for the topology '"
					<< topo << "'." << endl;
			}
			exit(-1);
		}

	}
	else
	{
		map<string, bRoutingFunction>::const_iterator match;

		match = bRoutingFunctionMap.find( fn_topo );

		if ( match != bRoutingFunctionMap.end( ) )
		{
			rf = (void *)match->second;
		}
		else
		{
			if ( fn == "none" )
			{
				cout << "Error: No routing function specified in configuration." << endl;
			}
			else
			{
				cout << "Error: Undefined routing function '" << fn << "' for the topology '"
					<< topo << "'." << endl;
			}
			exit(-1);
		}


	}



	gNumVCS = config.GetInt( "num_vcs" );

	// memoize 
	memo_log2gC = log_two( gC ) ;

	//
	// traffic class partitions
	//
	gReadReqBeginVC    = config.GetInt("read_request_begin_vc");
	gReadReqEndVC      = config.GetInt("read_request_end_vc");

	gWriteReqBeginVC   = config.GetInt("write_request_begin_vc");
	gWriteReqEndVC     = config.GetInt("write_request_end_vc");

	gReadReplyBeginVC  = config.GetInt("read_reply_begin_vc");
	gReadReplyEndVC    = config.GetInt("read_reply_end_vc");

	gWriteReplyBeginVC = config.GetInt("write_reply_begin_vc");
	gWriteReplyEndVC   = config.GetInt("write_reply_end_vc");





	return rf;
}


