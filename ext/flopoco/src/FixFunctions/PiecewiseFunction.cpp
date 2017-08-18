/*
  Piecewise function object for FloPoCo

  Author: Mioara Joldes

  (developed for the polynomial-based square root)

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  

  All rights reserved.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SOLLYA

#include "PiecewiseFunction.hpp"
#include <sstream>

namespace flopoco{

	PiecewiseFunction::PiecewiseFunction(string name_)
	{
	
	  /*Get the functions from the string*/
	  string::size_type  foundf, startIndex,i1,i2,i3;
	   Function *f;
	  
    startIndex=0;
	  while( startIndex != string::npos ) {
	    foundf  = name_.find_first_of(",",startIndex);
	    string functionName=name_.substr(startIndex, foundf-startIndex);
	    
	    i1=name_.find_first_of(",",foundf+1);
	    string xmins=name_.substr(foundf+1, i1 -(foundf+1));
	    double xmin = atof(xmins.c_str());
	    
	    i2=name_.find_first_of(",",i1+1);
	    string xmaxs=name_.substr(i1+1, i2-(i1+1));
	    double xmax = atof(xmaxs.c_str());
	    
	    i3=name_.find_first_of(",",i2+1);
	    string scales=name_.substr(i2+1, i3-(i2+1));
	    double scale = atof(scales.c_str());
	    
	    f= new Function( functionName.c_str(), xmin, xmax,scale); 
	    fArray.push_back(f);
      //cout<<"\n-------------"<<f->getName()<<endl;  
      startIndex = name_.find_first_of(";",startIndex+1);
      if (startIndex != string::npos )  startIndex=startIndex+1;
  }
  
  

	
		// Name HAS to be unique!
		// will cause weird bugs otherwise
		ostringstream complete_name;
		complete_name << name_ ; 
		name = complete_name.str();

		}

	PiecewiseFunction::~PiecewiseFunction()
	{
		//free_memory(node);
	}

	string PiecewiseFunction::getName() const
	{
		return name;
	}

	vector<Function*> PiecewiseFunction::getPiecewiseFunctionArray() const
  
	{
	return fArray;
	}

	 Function* PiecewiseFunction:: getPiecewiseFunctionArray(int i) const
	{
	  if (((unsigned)i)<fArray.size())
		return fArray[i];
    else return NULL;
	}
}

#endif //HAVE_SOLLYA
