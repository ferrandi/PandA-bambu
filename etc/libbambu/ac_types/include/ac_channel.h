/**************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 3.7                                                 *
 *                                                                        *
 *  Release Date    : Tue May 30 14:25:58 PDT 2017                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 3.7.2                                               *
 *                                                                        *
 *  Copyright 2004-2014, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *  
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  This file was modified by PandA team from Politecnico di Milano to    *
 *  generate efficient hardware for the PandA/bambu HLS tool.             *
 *  The API remains the same as defined by Mentor Graphics.               *
 *                                                                        *
 *************************************************************************/

/*
//  Source:          ac_channel.h
//  Description:     templatized channel communication class
//  Original Author: Andres Takach, Ph.D.
//  Modified by:     Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*/

#ifndef __AC_CHANNEL_H
#define __AC_CHANNEL_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#ifndef __BAMBU__
#include <deque>
#include <string>
#include <algorithm>
#endif

// Macro Definitions (obsolete - provided here for backward compatibility)
#define AC_CHAN_CTOR(varname) varname
#define AC_CHAN_CTOR_INIT(varname,init) varname(init)
#define AC_CHAN_CTOR_VAL(varname,init,val) varname(init,val)

///////////////////////////////////////////
// Class: ac_channel
//////////////////////////////////////////

template <class T>
class  ac_channel
{
public:

	T read();
	void read(T& t) ;
	void write(const T& t);

	// constructors
	ac_channel();  
	ac_channel(int init);
	ac_channel(int init, T val);

	bool available(unsigned int k);  // Return true if channel has at least k entries
	unsigned int  size();
	bool empty() {return size() == 0;}
 
	unsigned int debug_size() { 
#ifndef __BAMBU__
      return chan.size(); 
#endif
   }

	T    operator[](unsigned int pos) const {
#ifndef __BAMBU__
      return chan[pos];
#endif
   }

#ifndef __BAMBU__
	// These functions are useful for writing the testbench and the 
	// scverify flow but are not supported for synthesis.  
	bool operator==(const ac_channel &rhs) const { return this->chan == rhs.chan; }
	bool operator!=(const ac_channel &rhs) const { return !operator==(rhs); }
	void  reset() {chan.clear(); for (int i=0; i<(int)rSz; i++) write(rVal);}
 
	bool nb_read(T& t)   {
		if (size() != 0) {
			  read(t);
			  return true;
		} else return false;
	}
  
	int get_size_call_count() { int tmp=size_call_count; size_call_count=0; return tmp; }

    // data - The verification flow needs it to be public
	std::deque<T> chan;
#else
	// for synthesis
	bool nb_read(T& t);

private:
	// Prevent the compiler from autogenerating these.  (This enforces that channels are always passed by
	// reference.)  
	ac_channel(const ac_channel< T >&); 
	ac_channel& operator=(const ac_channel< T >&);
#endif

private:
 // data
#ifndef __BAMBU__
	unsigned int rSz;    // reset size
	T            rVal;   // resetValue
	int          size_call_count;
#else 
   T chan;  
#endif
};

// constructors with no name ID

template <class T>
ac_channel<T>::ac_channel()
#ifndef __BAMBU__
	: rSz(0)
	, size_call_count(0)
#endif
{}

// Default for types we don't know about
template <class T>
ac_channel<T>::ac_channel(int init) 
#ifndef __BAMBU__
	: rSz(init)
	, size_call_count(0)
#endif
{
	for (int i=init; i> 0; i--) {
		T dc; 
		write(dc);
	}
}

template <class T>
ac_channel<T>::ac_channel(int init, T val) 
#ifndef __BAMBU__
	: rSz(init)
	, rVal(val)
	, size_call_count(0)
#endif
{
	for (int i=init; i> 0; i--)
		write(val);
}

//The actual hardware looks very much like the SYNTHESIS model.  
// The 2 and 3 argument CTORs store the size arg
// in an integer variable similar to rSz.  The allows us to use a FIFO length which is 
// independent of the startup time of the design.

template <class T>
T ac_channel<T>::read()
{
#ifndef __BAMBU__
	if (chan.empty()) {
          std::cout << "Read from empty channel" << std::endl;  
          abort();
        }
	T t= chan.front(); 
	chan.pop_front(); 
	return t;
#else
	// this is non-sense (doesn't get used, right?)
  T dc;
  unsigned int chan_rSz = 0;
  if (chan_rSz == 0) {
    return chan;
  } else {
    chan_rSz--;
    return dc;
  }
#endif
}

template <class T>
void ac_channel<T>::read(T& t) 
{
	t = read();
}

template <class T>
void ac_channel<T>::write(const T& t) 
{
#ifndef __BAMBU__
	chan.push_back(t);
#endif
}

template <class T>
unsigned int  ac_channel<T>::size()
{
#ifndef __BAMBU__
	size_call_count++;
	return (int)chan.size();
#endif
}

template <class T>
bool ac_channel<T>::available(unsigned int k)
{ 
#ifndef __BAMBU__
	return chan.size() >= k;
#endif
}

template<class T>
inline std::ostream& operator<< (std::ostream& os, ac_channel<T> &a)
{
#ifndef __BAMBU__
  for (unsigned int i=0; i<a.size(); i++) {
    if (i > 0) os << " ";
    os << a[i];
  }
#endif
  return os;
}

#endif

