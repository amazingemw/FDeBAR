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

#include "booksim.hpp"
#include "fault.hpp"
#include <map>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "random_utils.hpp"             //has Random functions
#include "misc_utils.hpp"               //has powi and log_two

map<int, int> RouterFaultMap;

void InitializeFaultMap(){		    	//GUIDE
	RouterFaultMap[0] = 21;    			//31=all available
	RouterFaultMap[1] = 23;    			//30=east not available
	RouterFaultMap[2] = 23;    			//29=west not available
	RouterFaultMap[3] = 23;    			//27=north not available
	RouterFaultMap[4] = 23;    			//23=south not available
	RouterFaultMap[5] = 23;    			//26=north and east not available    28=east and west not available             
	RouterFaultMap[6] = 23;    			//25=north and west not available    19=north and south not available
	RouterFaultMap[7] = 22;    			//22=south and east not available
							   			//21=south and west not available
	RouterFaultMap[8] = 29;       //  FAULT INDICATOR (change as required)
	RouterFaultMap[9] = 31;       //            N
	RouterFaultMap[10] = 31;      //  56_57_58_59_60_61_62_63
	RouterFaultMap[11] = 31;      //   |  |  |  |  |  |  |  |
	RouterFaultMap[12] = 31;      //  48_49_50_51_52_53_54_55 
	RouterFaultMap[13] = 31;      //   |  |  |  |  |  |  |  |
	RouterFaultMap[14] = 31;      //  40_41_42_43_44_45_46_47
	RouterFaultMap[15] = 30;      //   |  |  |  |  |  |  |  |
								  //  32_33_34_35_36_37_38_39
	RouterFaultMap[16] = 29;      // W |  |  |  |  |  |  |  | E
	RouterFaultMap[17] = 31;	  //  24_25_26_27+28_29_30_31
	RouterFaultMap[18] = 31;      //   |  |  |  |  |  |  |  |
	RouterFaultMap[19] = 31;      //  16_17_18_19_20_21_22_23
	RouterFaultMap[20] = 31;      //   |  |  |  |  |  |  |  |
	RouterFaultMap[21] = 31;	  //  08_09_10_11_12_13_14_15
	RouterFaultMap[22] = 31;      //   |  |  |  |  |  |  |  |
	RouterFaultMap[23] = 30;	  //  00_01_02_03_04_05_06_07
                                  //  			S
	RouterFaultMap[24] = 29;	  //             
	RouterFaultMap[25] = 31;      // replace with + to denote
	RouterFaultMap[26] = 31;      // faults in above map
	RouterFaultMap[27] = 31;      // Take care to add faults in both directions to avoid Segmentation fault
	RouterFaultMap[28] = 31;
	RouterFaultMap[29] = 31;
	RouterFaultMap[30] = 31;
	RouterFaultMap[31] = 30;
	
	RouterFaultMap[32] = 29;
	RouterFaultMap[33] = 31;
	RouterFaultMap[34] = 31;
	RouterFaultMap[35] = 31;
	RouterFaultMap[36] = 31;
	RouterFaultMap[37] = 31;
	RouterFaultMap[38] = 31;
	RouterFaultMap[39] = 30;
	
	RouterFaultMap[40] = 29;
	RouterFaultMap[41] = 31;
	RouterFaultMap[42] = 31;
	RouterFaultMap[43] = 31;
	RouterFaultMap[44] = 31;
	RouterFaultMap[45] = 31;
	RouterFaultMap[46] = 31;
	RouterFaultMap[47] = 30;
	
	RouterFaultMap[48] = 29;
	RouterFaultMap[49] = 31;
	RouterFaultMap[50] = 31;
	RouterFaultMap[51] = 31;
	RouterFaultMap[52] = 31;
	RouterFaultMap[53] = 31;
	RouterFaultMap[54] = 31;
	RouterFaultMap[55] = 30;
	
	RouterFaultMap[56] = 25;
	RouterFaultMap[57] = 27;
	RouterFaultMap[58] = 27;
	RouterFaultMap[59] = 27;
	RouterFaultMap[60] = 27;
	RouterFaultMap[61] = 27;
	RouterFaultMap[62] = 27;
	RouterFaultMap[63] = 26;
	cout<<"Fault Map Initialized...."<<endl;
}

int GetRouterFault( int _routerid ){
	map<int,int>::const_iterator match;
	int r;
	match = RouterFaultMap.find( _routerid );
	if ( match != RouterFaultMap.end( ) )
	 {
	  	r = match->second;
	 }
	 return r;
}



