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

/*config_utils.cpp
 *
 *The configuration object which contained the parsed data from the 
 *configuration file
 */

#include "booksim.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "config_utils.hpp"

Configuration *Configuration::theConfig = 0;
int zz=0; int matchsec=0; // added John

Configuration::Configuration( )
{
	  theConfig = this;
	  _config_file = 0;
}

void Configuration::AddStrField( const string &field, const string &value )
{
	
  	_str_map[field] = strdup( value.c_str( ) );
	 cout<<"param"<<zz++<<"   "<<field<<"=  "<<_str_map[field]<<endl;// added John
}


// Reading each field in config file, Based on type of field value the 3 following functions are accordingly called. 
void Configuration::Assign( const string &field, const string &value ) // for string values
{
	  map<string,char *>::const_iterator match;
	  
	  match = _str_map.find( field );		// find a match  in all fields..
	  if ( match != _str_map.end( ) ) 
	  {
		    free( _str_map[field] );
		    _str_map[field] = strdup( value.c_str( ) );
		   cout<<"param"<<zz++<<" S "<<field<<" = "<<_str_map[field]<<endl;// added John
	  } 
	  else
	 {
		    string errmsg = "Unknown field string ";
		    errmsg += field;

		    ParseError( errmsg, 0 );
	  }
	
}

void Configuration::Assign( const string &field, unsigned int value )  // for int
{
	  map<string,unsigned int>::const_iterator match;
	  
	  match = _int_map.find( field );
	  if ( match != _int_map.end( ) ) 
	 {
	   	 _int_map[field] = value;
			cout<<"param"<<zz++<<" I  "<<field<<" = "<<_int_map[field]<<endl;// added John
	 }
	 else
	 {
		    string errmsg = "Unknown field int ";
		    errmsg += field;

		    ParseError( errmsg, 0 );
	  }
}

void Configuration::Assign( const string &field, double value )  // for double
{
	  map<string,double>::const_iterator match;
	  
	  match = _float_map.find( field );
	  if ( match != _float_map.end( ) )
	  {
	   	 _float_map[field] = value;
		cout<<"param"<<zz++<<" F  "<<field<<" = "<<_float_map[field]<<endl;// added John
	  }
	 else
	 {
		    string errmsg = "Unknown field double ";
		    errmsg += field;

		    ParseError( errmsg, 0 );
	 }
}

void Configuration::GetStr( const string &field, string &value, const string &def ) const
{
	 map<string,char *>::const_iterator match;

	 match = _str_map.find( field );
	 if ( match != _str_map.end( ) )
	 {
	  	value = match->second;
		//matchsec++; // John
	 }
	 else
	 {
		value = def;
	 }
}

unsigned int Configuration::GetInt( const string &field, unsigned int def ) const
{
	  map<string,unsigned int>::const_iterator match;
	  unsigned int r = def;

	  match = _int_map.find( field );
	  if ( match != _int_map.end( ) )
	 {
	  	r = match->second;
		//matchsec++; // John
	  }

	  return r;
}

double Configuration::GetFloat( const string &field, double def ) const
{  
	  map<string,double>::const_iterator match;
	  double r = def;

	  match = _float_map.find( field );
	  if ( match != _float_map.end( ) )
	 {
	  	r = match->second;
		//matchsec++; // John
	  }

	  return r;
}

void Configuration::ParseFile( const string& filename )
{
	  if ( ( _config_file = fopen( filename.c_str( ), "r" ) ) == 0 )  // if config file couldnot be opened, show error
	 {
		cerr << "Could not open configuration file " << filename << endl;
		exit( -1 );
	  }

	  configparse( );

	  fclose( _config_file );
	  _config_file = 0;
}

void Configuration::ParseString( const string& str )
{
	  _config_string = str + ';';
	  configparse( );
	  _config_string = "";
}

int Configuration::Input( char *line, int max_size )
{
	  int length = 0;

	  if ( _config_file )
	  {
	  	   length = fread( line, 1, max_size, _config_file );
	  } 
	  else
	  {
		    length = _config_string.length();
		    _config_string.copy(line, max_size);
		    _config_string.clear();
	  }

	  return length;
}

void Configuration::ParseError( const string &msg, unsigned int lineno ) const
{
	 if ( lineno )
	 {
	   	 cerr << "Parse error on line " << lineno << " : " << msg << endl;  // invokes when any field is having unmatched field values.
	 } 
	 else
	 {
	  	cerr << "Parse error : " << msg << endl;
	 }
	 exit( -1 );
}

Configuration *Configuration::GetTheConfig( )
{
	return theConfig;
}

//============================================================

int config_input( char *line, int max_size )
{
	return Configuration::GetTheConfig( )->Input( line, max_size );
}

bool ParseArgs( Configuration *cf, int argc, char **argv )
{
	bool rc = false;
	//cout<<"file is "<<argc  <<"argv is "<<*argv <<endl;   //  added JOHN 
	
	//all dashed variables are ignored by the arg parser
	for(int i = 1; i < argc; ++i)
	{
		string arg(argv[i]);
		cout<< "Configuration file : "<<argv[i] <<endl;   //  added JOHN now arg contains the fullpath of config file
		size_t pos = arg.find('=');
	    	bool dash = (argv[i][0] =='-');
	    	if(pos == string::npos && !dash)
		{
	      		// parse config file
			cout<<"######..Parsing config file ....##########"<<endl;       //  added JOHN
	      		cf->ParseFile( argv[i] );                      // calling ParseFile Function with arguementa s config file.
	      		ifstream in(argv[i]);
	      		//cout << "BEGIN Configuration File: " << argv[i] << endl;
	      		while (!in.eof())
			{
				char c;
				in.get(c);
				// cout << c ;   //commented John .. display config file contents on screen (optional..)
	      		}
	      		cout << "END Configuration File: " << argv[i] << endl;
			//cout<<"matchsec="<<matchsec<<endl;
			rc = true;
	    	}
		else if(pos != string::npos)
		{
	 		// override individual parameter
			cout << "OVERRIDE Parameter: " << arg << endl;
			cf->ParseString(argv[i]);
	    	}
	  } // end for
	  return rc;
}  // end ParseArgs


//helpful for the GUI, write out nearly all variables contained in a config file.
//However, it can't and won't write out  empty strings since the booksim yacc
//parser won't be abled to parse bland strings
void Configuration::WriteFile( const string& filename){
  
  ostream *config_out= new ofstream(filename.c_str());
  
  
  for(map<string,char *>::const_iterator i = _str_map.begin(); 
      i!=_str_map.end();
      i++){
    //the parser won't read blanks lolz
    if(i->second[0]!='\0'){
      *config_out<<i->first<<" = "<<i->second<<";"<<endl;
    }
  }
  
  for(map<string, unsigned int>::const_iterator i = _int_map.begin(); 
      i!=_int_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }

  for(map<string, double>::const_iterator i = _float_map.begin(); 
      i!=_float_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }
  config_out->flush();
  delete config_out;
 
}


