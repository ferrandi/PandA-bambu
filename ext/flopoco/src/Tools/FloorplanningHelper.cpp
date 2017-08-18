/*
Tools for performing floorplaning

Author : Matei Istoan, Florent de Dinechin

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2012.
  All rights reserved.

*/


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"
#include "utils.hpp"
#include "FloorplanningHelper.hpp"


namespace flopoco{
	

	FloorplanningHelper::FloorplanningHelper(Target* target_, Operator* op_){
		
		target = target_;
		parentOp = op_;
	}
	
	void FloorplanningHelper::addToFlpComponentList(std::string name){
		
		flComponentList.push_back(name);
	}
	
	void FloorplanningHelper::addToInstanceNames(std::string componentName, std::string instanceName){
		
		flInstanceNames[componentName] = instanceName;
	}
	
	void FloorplanningHelper::initFloorplanning(double ratio){
		
		floorplanningRatio = ratio;
		
		virtualModuleId = 0;
		
		prevEstimatedCountFF 		 = 0;
		prevEstimatedCountLUT 		 = 0;
		prevEstimatedCountMemory 	 = 0;
		prevEstimatedCountMultiplier = 0;
	}
	
	std::string FloorplanningHelper::manageFloorplan(){
		//create a new virtual module for the resources that are between
		//	two modules, and add it to the corresponding lists
		std::ostringstream result;
		Operator* virtualComponent = new Operator(target); 
		std::string moduleName = join("virtual_module_", virtualModuleId);
		
		result << "";
		
		virtualModuleId++;
		
		virtualComponent->setName(moduleName);
		virtualComponent->reHelper->estimatedCountLUT 			= parentOp->reHelper->estimatedCountLUT 	   - prevEstimatedCountLUT;
		virtualComponent->reHelper->estimatedCountFF  			= parentOp->reHelper->estimatedCountFF 		   - prevEstimatedCountFF;
		virtualComponent->reHelper->estimatedCountMultiplier 	= parentOp->reHelper->estimatedCountMultiplier - prevEstimatedCountMultiplier;
		virtualComponent->reHelper->estimatedCountMemory 		= parentOp->reHelper->estimatedCountMemory 	   - prevEstimatedCountMemory;
		
		if((virtualComponent->reHelper->estimatedCountLUT!=0) || (virtualComponent->reHelper->estimatedCountFF!=0) 
				|| (virtualComponent->reHelper->estimatedCountMultiplier!=0) || (virtualComponent->reHelper->estimatedCountMemory!=0)){
			flComponentList.push_back(moduleName);
			flVirtualComponentList[moduleName] = virtualComponent;
			
			result << "Created virtual module - " << moduleName << endl;
			result << tab << "Added " << parentOp->reHelper->estimatedCountLUT		  - prevEstimatedCountLUT << " function generators" << endl;
			result << tab << "Added " << parentOp->reHelper->estimatedCountFF		  - prevEstimatedCountFF << " registers" << endl;
			result << tab << "Added " << parentOp->reHelper->estimatedCountMultiplier - prevEstimatedCountMultiplier << " multipliers" << endl;
			result << tab << "Added " << parentOp->reHelper->estimatedCountMemory	  - prevEstimatedCountMemory << " memories" << endl;
			
			prevEstimatedCountLUT 			= parentOp->reHelper->estimatedCountLUT;
			prevEstimatedCountFF 			= parentOp->reHelper->estimatedCountFF;
			prevEstimatedCountMultiplier 	= parentOp->reHelper->estimatedCountMultiplier;
			prevEstimatedCountMemory 		= parentOp->reHelper->estimatedCountMemory;
		}
		
		return result.str();
	}
	
	std::string FloorplanningHelper::addPlacementConstraint(std::string source, std::string sink, int type){
		std::ostringstream result;
		constraintType newConstraint;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		//check to see if the type of the constraint is valid
		if(!((type==TO_LEFT_OF) || (type==TO_RIGHT_OF) || (type==ABOVE) || (type==UNDER) || 
				(type==TO_LEFT_OF_WITH_EXTRA) || (type==TO_RIGHT_OF_WITH_EXTRA) || (type==ABOVE_WITH_EXTRA) || (type==UNDER_WITH_EXTRA))){
			cerr << "Error: Trying to add a placement constraint of undefined type." << endl;
			exit(1);
		}
		
		//check if the source is a valid sub-component name
		if(subComponents.find(source)==subComponents.end()){
			cerr << "Error: source sub-component " << source << " was not found" << endl;
			exit(1);
		}
		
		//check if the sink is a valid sub-component name
		if(subComponents.find(sink)==subComponents.end()){
			cerr << "Error: sink sub-component " << sink << " was not found" << endl;
			exit(1);
		}
		
		newConstraint.type 		= PLACEMENT;
		newConstraint.source 	= source;
		newConstraint.sink 		= sink;
		newConstraint.value 	= type;
		
		flPlacementConstraintList.push_back(newConstraint);
		
		result << "Created new placement constraint" << endl;
		result << tab << "Sink module " << sink
				<< " is " << ((type==TO_LEFT_OF) ? "to the left of" 							: 
							(type==TO_RIGHT_OF) ? "to the right of" 							: 
							(type==ABOVE) ? "above" 											: 
							(type==UNDER) ? "under" 											: 
							(type==TO_LEFT_OF_WITH_EXTRA) ? "to the left of (with glue logic)"  : 
							(type==TO_RIGHT_OF_WITH_EXTRA) ? "to the right of (with glue logic)": 
							(type==ABOVE_WITH_EXTRA) ? "above (with glue logic)" 				: "under (with glue logic)")
				<< " source module " << source << endl;
		
		return result.str();
	}
	
	std::string FloorplanningHelper::addConnectivityConstraint(std::string source, std::string sink, int nrWires){
		std::ostringstream result;
		constraintType newConstraint;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		//no non-positive values allowed for the number of wires
		if(nrWires<1){
			cerr << "Error: trying to add an invalid number of wires:" << nrWires << endl;
			exit(1);
		}
		
		//check if the source is a valid sub-component name
		if(subComponents.find(source)==subComponents.end()){
			cerr << "Error: source sub-component " << source << " was not found" << endl;
			exit(1);
		}
		
		//check if the sink is a valid sub-component name
		if(subComponents.find(sink)==subComponents.end()){
			cerr << "Error: sink sub-component " << sink << " was not found" << endl;
			exit(1);
		}
		
		newConstraint.type 		= CONNECTIVITY;
		newConstraint.source 	= source;
		newConstraint.sink 		= sink;
		newConstraint.value 	= nrWires;
		
		flConnectivityConstraintList.push_back(newConstraint);
		
		result << "Created new connectivity constraint" << endl;
		result << tab << "Source module " << source 
				<< "is connected to sink module " << sink << " by " << nrWires << " wires" << endl;
		
		return result.str();
	}
	
	std::string FloorplanningHelper::addAspectConstraint(std::string source, double ratio){
		std::ostringstream result;
		constraintType newConstraint;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		//no non-positive values allowed for the aspect ratio
		if(ratio<=0){
			cerr << "Error: invalid aspect ratio:" << ratio << endl;
			exit(1);
		}
		
		//check if the source is a valid sub-component name
		if(subComponents.find(source)==subComponents.end()){
			cerr << "Error: source sub-component " << source << " was not found" << endl;
			exit(1);
		}
		
		newConstraint.type 		= ASPECT;
		newConstraint.source 	= source;
		newConstraint.ratio 	= ratio;
		
		flAspectConstraintList.push_back(newConstraint);
		
		result << "Created new aspect constraint" << endl;
		result << tab << "Source module " << source << " width to height ratio: " << ratio << endl;
		
		return result.str();
	}
	
	std::string FloorplanningHelper::addContentConstraint(std::string source, int value, int length){
		std::ostringstream result;
		constraintType newConstraint;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		//no non-positive values allowed for the length
		if(length<=0){
			cerr << "Error: invalid aspect ratio:" << length << endl;
			exit(1);
		}
		
		//check if the source is a valid sub-component name
		if(subComponents.find(source)==subComponents.end()){
			cerr << "Error: source sub-component " << source << " was not found" << endl;
			exit(1);
		}
		
		newConstraint.type 			= CONTENT;
		newConstraint.source 		= source;
		newConstraint.value 		= value;
		newConstraint.specialValue 	= length;
		
		flContentConstraintList.push_back(newConstraint);
		
		result << "Created new content constraint" << endl;
		result << tab << "Source module " << source 
											<< " contains " << (value==ADDER ? "an adder" : value==MULTIPLIER ? "a multiplier" : "") 
											<< " of length " << length << endl;
		
		return result.str();
	}
	
	std::string FloorplanningHelper::processConstraints(){
		std::ostringstream result;
		map<string, bool> processedModules;
		
		result << "Processing placement and connectivity constraints" << endl;
		
		//initialize the list of processed modules to false for all
		if(flPlacementConstraintList.size()>0){
			for(unsigned int i=0; i<flComponentList.size(); i++){
				processedModules[flComponentList[i]] = false;
			}
		}
		
		result << tab << "Initialized list of processed modules" << endl;
		
		//process each constraint, one by one, and update the virtual grid
		for(unsigned int i=0; i<flPlacementConstraintList.size(); i++){
			constraintType newConstraint = flPlacementConstraintList[i];
			constraintType sinkConnectivityConstraint;
			coordinateType sourceCoordinate, sinkCoordinate, newCoordinate;
			bool constrainedModule = false, moduleShiftNecessary = true;
			int incrementX, incrementY;
			int shiftX, shiftY;
			vector<std::string> connectedConstrainedModules;
			vector<constraintType> connectedModulesConstraints;
			
			result << tab << "Processing constraint" << endl;
			
			//only interested in placement constraints, for the main loop
			if(newConstraint.type != PLACEMENT)
				continue;
			
			result << tab << tab << "Found placement constraint between " << newConstraint.source << " and " << newConstraint.sink << endl;
			
			processedModules[newConstraint.source] 	= true;
			processedModules[newConstraint.sink] 	= true;
			
			//get coordinates of source and sink modules
			sourceCoordinate 	= flComponentCordVirtual[newConstraint.source];
			sinkCoordinate 		= flComponentCordVirtual[newConstraint.sink];
			
			//if the modules are already obeying the constraints, skip further processing
			if(newConstraint.value==TO_LEFT_OF || newConstraint.value==TO_LEFT_OF_WITH_EXTRA){ //to the left
				incrementX = -1;
				incrementY = 0;
				
				result << tab << tab << " Modules already obeying constraint - nothing left to do" << endl; 
			
				if(sinkCoordinate.x<sourceCoordinate.x && sinkCoordinate.y==sourceCoordinate.y)
					continue;
			}else if(newConstraint.value==TO_RIGHT_OF || newConstraint.value==TO_RIGHT_OF_WITH_EXTRA){ // to the right
				incrementX = 1;
				incrementY = 0;
				
				result << tab << tab << " Modules already obeying constraint - nothing left to do" << endl;
				
				if(sinkCoordinate.x>sourceCoordinate.x && sinkCoordinate.y==sourceCoordinate.y)
					continue;
			}else if(newConstraint.value==ABOVE || newConstraint.value==ABOVE_WITH_EXTRA){ // above
				incrementX = 0;
				incrementY = -1;
				
				result << tab << tab << " Modules already obeying constraint - nothing left to do" << endl;
				
				if(sinkCoordinate.x==sourceCoordinate.x && sinkCoordinate.y<sourceCoordinate.y)
					continue;
			}else{ // under
				incrementX = 0;
				incrementY = 1;
				
				result << tab << tab << " Modules already obeying constraint - nothing left to do" << endl;
				
				if(sinkCoordinate.x==sourceCoordinate.x && sinkCoordinate.y<sourceCoordinate.y)
					continue;
			}
			
			//determine the initial position, from the existing connectivity constraints
			//	initial position depends on whether other modules must occupy the same position - 
			//	their order is decided based how strong their connections
			//	with the source module are
			
			//find the other modules that have the same relative placement and
			// find their connectivity constraints
			
			result << tab << tab << "Searching for other modules connected to the source module " 
				<< newConstraint.source << " and that have connectivity constraints" << endl;
			
			connectedConstrainedModules.push_back(newConstraint.source);
			for(unsigned int j=0; j<flConnectivityConstraintList.size(); j++){
				constraintType newConnectivityConstraint = flConnectivityConstraintList[j];
				
				if((newConnectivityConstraint.source == newConstraint.source) && (newConnectivityConstraint.sink == newConstraint.sink)){
					constrainedModule = true;
					sinkConnectivityConstraint = newConnectivityConstraint;
				}
				
				//if there is another module that is connected to the source 
				//	module and that has the same relative placement, add 
				//	it to the list of modules so that they can be rearranged
				if((newConnectivityConstraint.source == newConstraint.source) && (newConnectivityConstraint.sink != newConstraint.sink)){
					for(unsigned int k=0; k<flPlacementConstraintList.size(); k++){
						constraintType tempPlacementConstraint = flPlacementConstraintList[k];
						
						if((tempPlacementConstraint.source == newConnectivityConstraint.source) 
								&& (tempPlacementConstraint.sink == newConnectivityConstraint.sink) 
								&& (tempPlacementConstraint.type == newConstraint.type)
								&& (processedModules[tempPlacementConstraint.sink] == true)){
							connectedConstrainedModules.push_back(newConnectivityConstraint.sink);
							connectedModulesConstraints.push_back(tempPlacementConstraint);
						}
					}
				}
			}
			//find the position for the newly placed module, given the 
			//	list of the other modules depending on the source module
			
			//add to the list of connected modules those that don't have 
			//	connectivity constraints
			
			result << tab << tab << "Completing the list of connected modules with the modules that do not have connectivity constraints" << endl;
			
			for(unsigned int j=0; j<flPlacementConstraintList.size(); j++){
				constraintType tempPlacementConstraint = flPlacementConstraintList[j];
				bool elementPresent = false;
				
				if((tempPlacementConstraint.source == newConstraint.source) 
						&& (tempPlacementConstraint.type == newConstraint.type)
						&& (processedModules[tempPlacementConstraint.sink] == true)){
							
					for(unsigned int k=0; k<connectedConstrainedModules.size(); k++)
						if(connectedConstrainedModules[k] == tempPlacementConstraint.sink){
							elementPresent = true;
							break;
						}
					if(!elementPresent){
						connectedConstrainedModules.push_back(tempPlacementConstraint.sink);
					}
				}
			}
			
			//initialize the new coordinates
			
			result << tab << tab << "Initializing the new coordinates of the sink module " << newConstraint.sink << endl;
			
			//check if the sink module has connectivity constraints
			if(constrainedModule){// there is a connectivity constraint on the sink module
				if(connectedModulesConstraints.size() == 0){  // no connectivity constraints on the other modules
					//place the new module first after the source
					newCoordinate.x = sourceCoordinate.x + incrementX;
					newCoordinate.y = sourceCoordinate.y + incrementY;
					
					shiftX = incrementX;
					shiftY = incrementY;
					
					result << tab << tab << tab << "No other module has connectivity constraints" 
						<< " - sink module placed immediately after source" << endl;
				}else{ // there are connectivity constraints
					//place the module based on the order of the constraints
					constraintType tempConstraint;
					
					//search for the first module that has lower connectivity constraints than the sink module
					tempConstraint.sink = "none";
					tempConstraint.value = -1;
					for(unsigned int j=0; j<connectedModulesConstraints.size(); j++){
						if(connectedModulesConstraints[j].value > tempConstraint.value 
								&& connectedModulesConstraints[j].value < sinkConnectivityConstraint.value){
							tempConstraint = connectedModulesConstraints[j];
						}
					}
					
					if(tempConstraint.sink == "none"){// this is the lowest priority module
						// look for the lowest connectivity constraint and add the new module after it
						tempConstraint = connectedModulesConstraints[0];
						
						for(unsigned int j=1; j<connectedModulesConstraints.size(); j++){
							if(connectedModulesConstraints[j].value > tempConstraint.value){
								tempConstraint = connectedModulesConstraints[j];
							}
						}
						
						moduleShiftNecessary = false;
					}
					
					newCoordinate.x = flComponentCordVirtual[tempConstraint.sink].x + incrementX;
					newCoordinate.y = flComponentCordVirtual[tempConstraint.sink].y + incrementY;
					
					shiftX = incrementX;
					shiftY = incrementY;
					
					result << tab << tab << tab << "There are other modules that have connectivity constraints"
						<< " - sink module placed according to connectivities" << endl;
				}
			}else{//no connectivity constraints on the sink module
				//get the last element added and insert this new element after it
				coordinateType tempCoordinate = flComponentCordVirtual[connectedConstrainedModules.back()];
				
				newCoordinate.x = tempCoordinate.x + incrementX;
				newCoordinate.y = tempCoordinate.y + incrementY;
				
				moduleShiftNecessary = false;
				
				result << tab << tab << tab << "There are no connectivity constraints" 
					<< " - sink module placed as the currently placed module" << endl;
			}
			
			//if the glue logic must also be moved
			if(newConstraint.value==TO_LEFT_OF_WITH_EXTRA || newConstraint.value==TO_RIGHT_OF_WITH_EXTRA 
				|| newConstraint.value==ABOVE_WITH_EXTRA || newConstraint.value==UNDER_WITH_EXTRA){
				//look for a virtual module just before the sink module, if it exists
				// if it does, then place it between the source and the sink
				std::string prevModuleName = "";
				
				result << tab << tab << "Also moving the glue logic with the sink module " << newConstraint.sink << endl;
				
				for(unsigned int j=0; j<flComponentList.size(); j++){
					if(newConstraint.sink == flComponentList[j])
						break;
					else
						prevModuleName = flComponentList[j];
				}
				if(prevModuleName.find("virtual_module_") != string::npos){
					flComponentCordVirtual[prevModuleName] = newCoordinate;
					newCoordinate.x += incrementX;
					newCoordinate.y += incrementY;
					
					shiftX += incrementX;
					shiftY += incrementY;
					
					result << tab << tab << tab << "Found virtual module before sink - moving it with the sink module" << endl;
				}
			}
						
			//change the coordinates of the sink module
			flComponentCordVirtual[newConstraint.sink] = newCoordinate;
			
			result << tab << tab << "Placed new module at the desired location" << endl;
			
			//if needed, shift the already placed modules
			if(moduleShiftNecessary){
				
				result << tab << tab << tab << "Shift of the other modules connected to source module " 
					<< newConstraint.source << " necessary and being performed" << endl;
				
				for(unsigned int j=0; j<flComponentList.size(); j++){
					coordinateType tempCoordinate;
					
					if(processedModules[flComponentList[i]] && flComponentList[j]!=newConstraint.sink){
						tempCoordinate = flComponentCordVirtual[flComponentList[j]];
						
						if(newConstraint.value==TO_LEFT_OF || newConstraint.value==TO_LEFT_OF_WITH_EXTRA){ //to the left
							if(tempCoordinate.x>=newCoordinate.x && tempCoordinate.y==newCoordinate.y)
								tempCoordinate.x += shiftX;
						}else if(newConstraint.value==TO_RIGHT_OF || newConstraint.value==TO_RIGHT_OF_WITH_EXTRA){ // to the right
							if(tempCoordinate.x<=newCoordinate.x && tempCoordinate.y==newCoordinate.y)
								tempCoordinate.x += shiftX;
						}else if(newConstraint.value==ABOVE || newConstraint.value==ABOVE_WITH_EXTRA){ // above
							if(tempCoordinate.y<=newCoordinate.y && tempCoordinate.x==newCoordinate.x)
								tempCoordinate.y += shiftY;
						}else{ // under
							if(tempCoordinate.y>=newCoordinate.y && tempCoordinate.x==newCoordinate.x)
								tempCoordinate.y += shiftY;
						}
					}
				}
			}
			
			//report changes on the virtual grid
			result << tab << tab << "Updated coordinates of module " << newConstraint.sink << endl;
			result << tab << tab << tab << " from x=" << sinkCoordinate.x << " and y=" << sinkCoordinate.y << endl;
			result << tab << tab << tab
					<< ((newConstraint.value==TO_LEFT_OF  || newConstraint.value==TO_LEFT_OF_WITH_EXTRA) ? " moving to the left of module " :
					   (newConstraint.value==TO_RIGHT_OF || newConstraint.value==TO_RIGHT_OF_WITH_EXTRA) ? " moving to the right of module " : 
					   (newConstraint.value==ABOVE 		 || newConstraint.value==ABOVE_WITH_EXTRA) 		 ? " moving above module " 
																										 : " moving under module ")
					<< newConstraint.source << endl;
			result << tab << tab << tab << " to x=" << newCoordinate.x << " and y=" << newCoordinate.y << endl;
		}
		
		//re-normalize the virtual grid
		//the constraints might have created wholes in the grid, 
		//	leading to unused space, both on the horizontal and the vertical
		
		result << tab << "Renormalizing virtual grid to eliminate possible gaps" << endl;
		
		//scan the components' y coordinates; if there are negative 
		//	coordinates, shift all coordinates downwards, so as to
		//	have all coordinates non-negative
		bool hasEmptyLines = false;
		int minLine = 0, maxLine = 0;
		int minColumn = 0;
		
		//compute the maximum line - indicates the modules for which 
		//	not to search for gaps
		//compute the minimum line - shows how much the other lines 
		//	must be moved downwards
		for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
			coordinateType tempCoordinate = it->second;
			
			if(tempCoordinate.y < minLine)
				minLine = tempCoordinate.y;
			if(tempCoordinate.y>maxLine)
				maxLine = tempCoordinate.y;
				
			if(tempCoordinate.x < minColumn)
				minColumn = tempCoordinate.x;
		}
		
		//if there are negative lines, increase all the lines so that all 
		//	the lines are positive
		if(minLine < 0){
			for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
				(it->second).y += abs(minLine);
			}
			maxLine += abs(minLine);
			
			result << tab << tab << "Renormalized virtual grid lines" << endl;
		}
		
		//if there are negative columns, increase all the columns so that all 
		//	the columns are positive
		if(minColumn < 0){
			for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
				(it->second).x += abs(minColumn);
			}
			
			result << tab << tab << "Renormalized virtual grid columns" << endl;
		}
		
		//scan the components' y coordinates; if there are empty lines,
		//	move the rest of the lines up
		
		//as long as the closest line to the current is more than a line 
		//	away, move the line upwards so as to fill the gaps
		do{
			int minGapSize = flComponentCordVirtual.size(), minGapLine = 0;
			
			for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
				coordinateType tempCoordinate = it->second;
				coordinateType closestCoordinate = it->second;
				
				if(tempCoordinate.y == maxLine)
					continue;
				
				closestCoordinate.y += flComponentCordVirtual.size();
				
				for(map<string, coordinateType>::iterator it2 = flComponentCordVirtual.begin(); it2 != flComponentCordVirtual.end(); it2++){
					coordinateType tempCoordinate2 = it2->second;
					
					if((tempCoordinate2.y > tempCoordinate.y) && (tempCoordinate2.y < closestCoordinate.y) && (it->first != it2->first))
						closestCoordinate = tempCoordinate2;
				}
				
				if((closestCoordinate.y-tempCoordinate.y < minGapSize) && (closestCoordinate.y-tempCoordinate.y > 1)){
					minGapSize = closestCoordinate.y-tempCoordinate.y;
					minGapLine = tempCoordinate.y;
				}
			}
			
			if(minGapSize>1 && minGapSize!=flComponentCordVirtual.size()){
				for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
					if((it->second).y >= minGapLine)
						flComponentCordVirtual[(it->first)].y -= (minGapSize-1);
				}
				
				result << tab << tab << "Removed empty line - gap of " << minGapSize << " at line " << minGapLine << endl;
			}else{
				hasEmptyLines = false;
			}
		}while(hasEmptyLines == true);
		
		return result.str();
	}
	
	std::string FloorplanningHelper::createVirtualGrid(){
		std::ostringstream result;
		
		result << "Creating virtual grid of sub-components" << endl;
		
		//create the virtual component grid, and place the modules one
		//	under the other, in the order that they are instantiated
		//virtual modules, for the glue logic, are also considered as
		//	modules
		for(unsigned int i=0; i<flComponentList.size(); i++){
			std::string componentName = flComponentList[i];
			coordinateType newCoordinate;
			
			newCoordinate.x = 0;
			newCoordinate.y = i;
			
			flComponentCordVirtual[componentName] = newCoordinate;
			
			result << tab << "Sub-component " << componentName << " placed by default at x=0 and y=" << i << endl;
		}
		
		return result.str();
	}
	
	/*
	bool FloorplanningHelper::createHorrizontalPlacement(){
		
	}
	
	bool FloorplanningHelper::createVerticalPlacement(){
		
	}
	*/
	
	std::string FloorplanningHelper::createPlacementGrid(){
		ostringstream result;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		result << "Created real grid of components" << endl;
		
		//different processing techniques and effort for modules that
		//	contain and that don't contain DSP and RAM blocks
		if(parentOp->reHelper->estimatedCountDSP!=0 
				|| parentOp->reHelper->estimatedCountRAM!=0 || parentOp->reHelper->estimatedCountROM!=0){ // for modules with DSPs/RAMs
			int nrLines = 0, nrColumns = 0;
			int maxDSPperLine = 0, maxDSPperColumn = 0, maxRAMperLine = 0, maxRAMperColumn = 0;
			vector<int> nrDSPperLine, nrDSPperColumn, nrRAMperLine, nrRAMperColumn;
			vector< vector<string> > subcomponentMatrixLines, subcomponentMatrixColumns;
			int maxComponentHeight = 0;
			
			result << tab << "Creating the grid of real coordintes with DSP or/and RAM requirements" << endl;
			
			//determine the maximum number of DSPs in one line of the floorplan
			//	if the nb. is larger the number of lines in the chip, throw warning to the user,
			//	then check to see the maximum number of DSPs in one column
			//-> if viable, break the lines longer than the capacity and warn 
			//	user of the performed operations
			
			//construct (almost) 2D versions of the virtual grid, organized by lines and by columns
			
			//determine the number of lines - there are no gaps, so the lines will be numbered from 0 to nrLines
			for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
				coordinateType tempCoordinate = it->second;
				
				if(tempCoordinate.y > nrLines)
					nrLines = tempCoordinate.y;
				if(tempCoordinate.x > nrColumns)
					nrColumns = tempCoordinate.x;
			}
			
			//create the 2D virtual grid - by lines
			
			//initialize the two grids
			subcomponentMatrixLines.resize(nrLines);
			subcomponentMatrixColumns.resize(nrColumns);
			nrDSPperLine.resize(nrLines, 0);
			nrDSPperColumn.resize(nrLines, 0);
			nrRAMperLine.resize(nrLines, 0);
			nrRAMperColumn.resize(nrColumns, 0);
			
			//populate the lines and comlumns grid with the elements from the 
			// list of elements
			for(unsigned int i=0; i<flComponentList.size(); i++){
				Operator* tempOperator;
				coordinateType tempCoordinate = flComponentCordVirtual[flComponentList[i]];
				
				if(subComponents.find(flComponentList[i]) == subComponents.end())
					tempOperator = flVirtualComponentList[flComponentList[i]];
				else
					tempOperator = subComponents[flComponentList[i]];
				
				subcomponentMatrixLines[tempCoordinate.y].push_back(flComponentList[i]);
				subcomponentMatrixColumns[tempCoordinate.x].push_back(flComponentList[i]);
				
				nrDSPperLine[tempCoordinate.y] += (tempOperator->reHelper->estimatedCountDSP > 0) ? 1 : 0;
				nrRAMperLine[tempCoordinate.y] += (tempOperator->reHelper->estimatedCountRAM > 0) ? 1 : 0;
				nrRAMperLine[tempCoordinate.y] += (tempOperator->reHelper->estimatedCountROM > 0) ? 1 : 0;
				
				nrDSPperColumn[tempCoordinate.x] += tempOperator->reHelper->estimatedCountDSP;
				nrRAMperColumn[tempCoordinate.x] += tempOperator->reHelper->estimatedCountRAM;
				nrRAMperColumn[tempCoordinate.x] += tempOperator->reHelper->estimatedCountROM;
			}
			
			result << tab << "Created the (almost) 2D versions of the virtual component grid, organized by lines and by columns" << endl;
			
			//go through the lines and the columns and check if the required 
			//	number of DSPs/RAMs is larger than those available on-chip
			
			result << tab << "Testing if the DSP/RAM requirements can be satisfied by the current floorplan" << endl;
			
			for(int i=0; i<nrLines; i++){
				if((nrDSPperLine[i] > target->multiplierPosition.size()) || (nrRAMperLine[i] > target->memoryPosition.size())){
					result << "Error: the current floorplan will not fit on the design;" 
								<< " try changing the design orientation or having less elements in a line" << endl;
					result << "Floorplan creation abandoned" << endl;
					
					cerr << "Error: the current floorplan will not fit on the design; try changing the design orientation" << endl;
					cerr << tab << "Flushing the debug information to the debug file \'flopoco.floorplan.debug\' and exiting" << endl;
					//flush debugging messages to the debug file
					ofstream file;
					file.open("flopoco.floorplan.debug");
					file << result.str();
					file.close();
					exit(1);
				}
			}
			for(int i=0; i<nrColumns; i++){
				if((nrDSPperColumn[i] > target->dspPerColumn) || (nrRAMperLine[i] > target->ramPerColumn)){
					result << "Error: the current floorplan will not fit on the design;" 
								<< " try changing the design orientation or having less elements in a column" << endl;
					result << "Floorplan creation abandoned" << endl;
					
					cerr << "Error: the current floorplan will not fit on the design; try changing the design orientation" << endl;
					cerr << tab << "Flushing the debug information to the debug file \'flopoco.floorplan.debug\' and exiting" << endl;
					//flush debugging messages to the debug file
					ofstream file;
					file.open("flopoco.floorplan.debug");
					file << result.str();
					file.close();
					exit(1);
				}
			}
			
			//determine the maximum height of a bounding box for a 
			//	sub-component based on the largets chain of DSPs or RAMs 
			//	in any of the sub-components
			for(unsigned int i=0; i<flComponentList.size(); i++){
				Operator* tempOperator;
				std::string componentName = flComponentList[i];
				int multiplierHeightRatio, ramHeightRatio, lutHeightRatio, ffHeightRatio;
				double ratio = 1.0;
				
				if(subComponents.find(componentName) == subComponents.end())
					tempOperator = flVirtualComponentList[componentName];
				else
					tempOperator = subComponents[componentName];
					
				for(unsigned int j=0; j<flAspectConstraintList.size(); j++){
					constraintType tempConstraint = flAspectConstraintList[j];
					
					if(tempConstraint.source == flComponentList[i])
							ratio = tempConstraint.ratio;
				}
				
				for(unsigned int j=0; j<flContentConstraintList.size(); j++){
					constraintType tempConstraint = flContentConstraintList[j];
					
					if(tempConstraint.source == flComponentList[i])
							ratio = 1.0/tempConstraint.specialValue;
				}
				
				lutHeightRatio 			= ceil(sqrt(tempOperator->reHelper->estimatedCountLUT / ratio));
				ffHeightRatio			= ceil(sqrt(tempOperator->reHelper->estimatedCountFF / 2.0 / ratio));
				multiplierHeightRatio	= tempOperator->reHelper->estimatedCountMultiplier / target->dspHeightInLUT;
				ramHeightRatio 			= tempOperator->reHelper->estimatedCountMemory / target->ramHeightInLUT;
				if(lutHeightRatio > maxComponentHeight)
					maxComponentHeight = lutHeightRatio;
				if(ffHeightRatio > maxComponentHeight)
					maxComponentHeight = ffHeightRatio;
				if(multiplierHeightRatio > maxComponentHeight)
					maxComponentHeight = multiplierHeightRatio;
				if(ramHeightRatio > maxComponentHeight)
					maxComponentHeight = ramHeightRatio;
			}
			
			result << tab << "Computed the maximum height of the components" << endl;
			
			//order the lines of the 2D virtual placement matrix, in the
			//	increasing order of their x coordinates
			//sort using quicksort----------------------------------------------
			for(unsigned int i=0; i<subcomponentMatrixLines.size(); i++){
				vector< vector<string> > stack;
				vector<string> sortedLine;
				
				stack.push_back(subcomponentMatrixLines[i]);
				while(stack.size()>0){
					vector<string> tempArray;
					
					tempArray = stack.back();
					stack.pop_back();
					while(tempArray.size()==1){
						sortedLine.insert(sortedLine.begin(), tempArray[0]);
						
						if(stack.size()>0){
							tempArray = stack.back();
							stack.pop_back();
						}else
							break;
					}
					
					if(tempArray.size() == 1)
						continue;
					
					srand(time(NULL));
					int pivotIndex = rand() % tempArray.size();
					string pivot = tempArray[pivotIndex];
					vector<string> lesser, greater;
					
					for(unsigned int j=0; j<tempArray.size(); j++){
						if(j == pivotIndex)
							continue;
						if(flComponentCordVirtual[tempArray[j]].x < flComponentCordVirtual[pivot].x){
							lesser.insert(lesser.begin(), tempArray[j]);
						}
						else{
							greater.insert(greater.begin(), tempArray[j]);
						}
					}
					
					if(lesser.size() > 0)
						stack.push_back(lesser);
					
					stack.push_back(std::vector<string>(1, pivot));
					
					if(greater.size() > 0)
						stack.push_back(greater);
				}
				
				subcomponentMatrixLines[i] = sortedLine;
			}
			//sort using quicksort----------------------------------------------
			
			result << tab << "Ordered the elements of the lines in the order that they appear in the virtual grid" << endl;
			
			//go through the 2D virtual placement list and generate the
			//	real coordinate list
			int prevCoordX, prevCoordY;
			
			result << tab << "Creating the initial placement in the real coordinate grid" << endl;
			
			//create an initial placement, then move the components around
			//	so as to satisfy their resource requirements
			//take into account ASPECT and CONTENT constraints
			for(unsigned int i=0; i<subcomponentMatrixLines.size(); i++){
				vector<string> matrixLine = subcomponentMatrixLines[i];
				
				prevCoordX = 0;
				prevCoordY = i * maxComponentHeight * (1.0/floorplanningRatio);
				for(unsigned int j=0; j<matrixLine.size(); j++){
					int lutWidth, ffWidth, componentWidth;
					coordinateType componentLocation, componentDimension;
					Operator* currentComponent = subComponents[matrixLine[j]];
					
					lutWidth = (currentComponent->reHelper->estimatedCountLUT/target->lutPerSlice)/maxComponentHeight;		//width in slices
					ffWidth = (currentComponent->reHelper->estimatedCountFF/target->ffPerSlice)/maxComponentHeight;		//width in slices
					componentWidth = (lutWidth>ffWidth) ? lutWidth : ffWidth;
					
					componentLocation.x = prevCoordX + 1;
					componentLocation.y = prevCoordY;
					componentDimension.x = componentWidth * (1.0/floorplanningRatio);
					componentDimension.y = maxComponentHeight * (1.0/floorplanningRatio);
					
					flComponentCordReal[matrixLine[j]] = componentLocation;
					flComponentDimension[matrixLine[j]] = componentDimension;
					
					prevCoordX = prevCoordX + 1 + componentDimension.x;
				}
			}
			
			result << tab << "Adjusting the component grid so that components with RAM and DSP requirement are properly placed" << endl;
			
			//shift the components to the left so that they meet the
			//	DSP and RAM requirements
			for(unsigned int i=0; i<subcomponentMatrixLines.size(); i++){
				vector<string> matrixLine = subcomponentMatrixLines[i];
				
				for(unsigned int j=0; j<matrixLine.size(); j++){
					bool dspSatisfied = false, ramSatisfied = false;
						
					for(unsigned int k=0; k<target->multiplierPosition.size(); k++){
						if(target->multiplierPosition[k] == flComponentCordReal[matrixLine[j]].x+1){
							dspSatisfied = true;
							break;
						}
					}
					for(unsigned int k=0; k<target->memoryPosition.size(); k++){
						if(target->memoryPosition[k] == flComponentCordReal[matrixLine[j]].x+1){
							dspSatisfied = true;
							break;
						}
					}
					
					if(dspSatisfied && ramSatisfied)
						continue;
					else if(!dspSatisfied && !ramSatisfied){
						int closestDSPColumn = flComponentCordReal[matrixLine[j]].x, closestRAMColumn = flComponentCordReal[matrixLine[j]].x;
						int closestMove, dimensionIncrease;
						
						//decide which is closest, DSP or RAM, then shift to that column, 
						//	and then extend the width to accomodate the other resource
						for(unsigned int k=0; k<target->multiplierPosition.size(); k++){
							if(target->multiplierPosition[k] >= flComponentCordReal[matrixLine[j]].x){
								closestDSPColumn = target->multiplierPosition[k]+1;
								break;
							}
						}
						for(unsigned int k=0; k<target->memoryPosition.size(); k++){
							if(target->memoryPosition[k] >= flComponentCordReal[matrixLine[j]].x){
								closestRAMColumn = target->memoryPosition[k]+1;
								break;
							}
						}
						
						closestMove = (closestDSPColumn>closestRAMColumn) ? closestDSPColumn : closestRAMColumn;
						dimensionIncrease += abs(closestDSPColumn - closestRAMColumn);
						//if needed, shift all the components to the left
						if((closestMove - flComponentCordReal[matrixLine[j]].x) > 0){
							for(unsigned int k=j; k<matrixLine.size(); k++){
								flComponentCordReal[matrixLine[k]].x += dimensionIncrease + 
																			(closestMove - flComponentCordReal[matrixLine[j]].x);
							}
						}
					}else if(!dspSatisfied){
						int closestDSPColumn = flComponentCordReal[matrixLine[j]].x;
						
						for(unsigned int k=0; k<target->multiplierPosition.size(); k++){
							if(target->multiplierPosition[k] >= flComponentCordReal[matrixLine[j]].x){
								closestDSPColumn = target->multiplierPosition[k]+1;
								break;
							}
						}
						
						//if needed, shift all the components to the left
						int shiftSize = closestDSPColumn - flComponentCordReal[matrixLine[j]].x;
						if(shiftSize > 0){
							for(unsigned int k=j; k<matrixLine.size(); k++){
								flComponentCordReal[matrixLine[k]].x += shiftSize;
							}
						}
					}else if(!ramSatisfied){
						int closestRAMColumn = flComponentCordReal[matrixLine[j]].x;
						
						for(unsigned int k=0; k<target->memoryPosition.size(); k++){
							if(target->memoryPosition[k] >= flComponentCordReal[matrixLine[j]].x){
								closestRAMColumn = target->memoryPosition[k]+1;
								break;
							}
						}
						
						//if needed, shift all the components to the left
						int shiftSize = closestRAMColumn - flComponentCordReal[matrixLine[j]].x;
						if(shiftSize > 0){
							for(unsigned int k=j; k<matrixLine.size(); k++){
								flComponentCordReal[matrixLine[k]].x += shiftSize;
							}
						}
					}
				}
			}
			
		}else{
			//creating the placement for operators that don not have DSPs and RAMs
			result << tab << "Creating the grid of real coordintes" << endl;
			
			int nrLines = 0;
			vector< vector<string> > subcomponentMatrixLines;
			int maxComponentHeight = 0;
			
			result << tab << "Construct a (almost) 2D version of the virtual grid, organized by lines" << endl;
			
			//construct a (almost) 2D version of the virtual grid, organized by lines
			for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
				if((it->second).y > nrLines)
					nrLines = (it->second).y;
			}
			for(int i=0; i<=nrLines; i++){
				vector<string> tempLevelList;
				
				for(map<string, coordinateType>::iterator it = flComponentCordVirtual.begin(); it != flComponentCordVirtual.end(); it++){
					if((it->second).y == i){
						tempLevelList.push_back(it->first);
					}
				}
				subcomponentMatrixLines.push_back(tempLevelList);
			}
			
			//determine the maximum height of a bounding box of any of 
			//	the sub-components; the goal is to have balanced dimensions 
			//	for the boxes, so make the largest box as close to being 
			//	square -> hence the square root
			for(unsigned int i=0; i<flComponentList.size(); i++){
				Operator* tempOperator;
				int operatorWidth; 
				
				if(subComponents.find(flComponentList[i]) != subComponents.end()){
					tempOperator = subComponents[flComponentList[i]];
				}else{
					tempOperator = flVirtualComponentList[flComponentList[i]];
				}
				
				operatorWidth = ceil(sqrt((tempOperator->reHelper->estimatedCountLUT)/(double)target->lutPerSlice));
				if(operatorWidth>maxComponentHeight)
					maxComponentHeight = operatorWidth;
				operatorWidth = ceil(sqrt((tempOperator->reHelper->estimatedCountFF)/(double)target->ffPerSlice));
				if(operatorWidth>maxComponentHeight)
					maxComponentHeight = operatorWidth;
			}
			
			//if the modules chain vertically and they run out of space, expand design horizontally
			while(ceil(nrLines*maxComponentHeight*sqrt(1.0/floorplanningRatio)) > target->topSliceY){
				maxComponentHeight--;
				
				//design cannot be floorplanned with the current constraints
				if(maxComponentHeight == 0){
					cerr << "Error: the design cannot be floorplanned with the current constraints. Please reconsider and re-run." << endl;
					cerr << tab << "number of lines= " << nrLines << " maximum component height=" << maxComponentHeight << " ratio=" << 1.0/floorplanningRatio << endl;
					cerr << tab << "maximum allowed height=" << target->topSliceY << endl;
					exit(1);
				}
			}
			
			result << tab << "Order the lines of the 2D virtual placement matrix" << endl;
			
			//order the lines of the 2D virtual placement matrix, in the
			//	increasing order of their x coordinates
			for(unsigned int i=0; i<subcomponentMatrixLines.size(); i++){
				vector<string> tempList = subcomponentMatrixLines[i];
				
				for(unsigned int j=0; j<tempList.size()-1; j++)
					for(unsigned int k=j+1; k<tempList.size(); k++){
						if((flComponentCordVirtual[tempList[j]]).x > (flComponentCordVirtual[tempList[k]]).x){
							coordinateType tempCoord = flComponentCordVirtual[tempList[j]];
							flComponentCordVirtual[tempList[j]] = flComponentCordVirtual[tempList[k]];
							flComponentCordVirtual[tempList[k]] = tempCoord;
							
							string tempString = tempList[j];
							tempList[j] = tempList[k];
							tempList[k] = tempString;
						}
					}
					
				subcomponentMatrixLines[i] = tempList;
			}
			
			result << tab << "Generate the real coordinate grid" << endl;
			
			//go through the 2D virtual placement list and generate the
			//	real coordinate list
			int prevCoordX, prevCoordY;
			
			//place each line at a time; lines with lower y coordinates 
			//	are processed first, and inside the lines the elements 
			// are processed in the increasing order of their x coordinates
			for(unsigned int i=0; i<subcomponentMatrixLines.size(); i++){
				vector<string> matrixLine = subcomponentMatrixLines[i];
				
				prevCoordX = 0;
				prevCoordY = ceil(i * maxComponentHeight * sqrt(1.0/floorplanningRatio));
				for(unsigned int j=0; j<matrixLine.size(); j++){
					int lutWidth, ffWidth, componentWidth;
					coordinateType componentLocation, componentDimension;
					Operator* currentComponent;
					
					if(subComponents.find(matrixLine[j]) != subComponents.end()){
						currentComponent = subComponents[matrixLine[j]];
						
						lutWidth = ceil(((double)(currentComponent->reHelper->estimatedCountLUT)/(target->lutPerSlice))/maxComponentHeight);		//width in slices
						ffWidth = ceil(((double)(currentComponent->reHelper->estimatedCountFF)/(target->ffPerSlice))/maxComponentHeight);		//width in slices
						componentWidth = (lutWidth>ffWidth) ? lutWidth : ffWidth;
						
						componentLocation.x = prevCoordX + 1;
						componentLocation.y = prevCoordY;
						componentDimension.x = ceil(componentWidth * sqrt(1.0/(double)floorplanningRatio));
						componentDimension.y = ceil(maxComponentHeight * sqrt(1.0/(double)floorplanningRatio));
						
						flComponentCordReal[matrixLine[j]] = componentLocation;
						flComponentDimension[matrixLine[j]] = componentDimension;
						
						prevCoordX = prevCoordX + 1 + componentDimension.x;
					}
				}
			}
		}
		
		return result.str();
	}
	
	std::string FloorplanningHelper::createConstraintsFile(){
		ofstream file;
		ostringstream result;
		
		result << "Created output constraints file" << endl;
		
		//create the physical file
		if(target->getVendor() == "Xilinx")
			file.open("flopoco.ucf");
		else if(target->getVendor() == "Altera"){
		
		}
		
		result << tab << "Adding constraints" << endl;
		result << tab << "Added constraints to contain the entire operator" << endl;
		file << createPlacementForComponent("root");
		
		//create the placement constraints for each sub-component at a time
		//this time, the boxes for the glue logic aren't gien any bounds, 
		//	but they will have the empty space between the modules, which 
		//	should fit them
		for(unsigned int i=0; i<flComponentList.size(); i++){
			result << tab << "Added constraints for sub-component " << flComponentList[i] << endl;
			file << createPlacementForComponent(flComponentList[i]);
		}
		
		file.close();
		
		if(target->getVendor() == "Xilinx"){
			cerr << "***Floorplan written to \'flopoco.ucf\' constraints file" << endl;
		}else if(target->getVendor() == "Altera"){
			cerr << "ERROR: Floorplanning feature not yet implemented for Altera target" << endl;
		}else{
			cerr << "ERROR: Floorplanning not yet implemented for this target" << endl;
		}
		
		return result.str();
	}
	
	std::string FloorplanningHelper::createPlacementForComponent(std::string moduleName){
		ostringstream constraintString;
		map<string, Operator*> subComponents = parentOp->subComponents_;
		
		constraintString << "";
		
		//create the constraint for the whole operator
		if(moduleName == "root"){
			
			int maxX, maxY, minX, minY;
			map<string, coordinateType>::iterator it = flComponentCordReal.begin();
			
			minX = (it->second).x;
			minY = (it->second).y;
			
			maxX = minX + (flComponentDimension[it->first]).x;
			maxY = minY + (flComponentDimension[it->first]).y;
			
			it++;
			while(it != flComponentCordReal.end()){
				if((it->second).x < minX){
					minX = (it->second).x;
				}
				if((it->second).y < minY){
					minY = (it->second).y;
				}
				if((it->second).x+(flComponentDimension[it->first]).x > maxX){
					maxX = (it->second).x+(flComponentDimension[it->first]).x;
				}
				if((it->second).y+(flComponentDimension[it->first]).y > maxY){
					maxY = (it->second).y+(flComponentDimension[it->first]).y;
				}
				it++;
			}
			
			constraintString << "INST \"*\" AREA_GROUP=\"pblock_root\";" << endl;
			constraintString << "AREA_GROUP \"pblock_root\" RANGE=SLICE_X" 
				<< minX << "Y" << minY << ":SLICE_X" << maxX << "Y" << maxY << ";" << endl;
			constraintString << endl;
				
			return constraintString.str();
		}
		
		if(flVirtualComponentList.find(moduleName) != flVirtualComponentList.end())
			return constraintString.str();
		
		string instanceName = flInstanceNames[moduleName];
		
		if(target->getVendor() == "Xilinx"){
			//create the constraint
			constraintString << "INST \"" << instanceName << "\" AREA_GROUP=\"pblock_" << instanceName << "\";" << endl;
			//add constraints for function generators and registers
			constraintString << "AREA_GROUP \"pblock_" << instanceName 
				<< "\" RANGE=SLICE_X" << (flComponentCordReal[moduleName]).x << "Y" << (flComponentCordReal[moduleName]).y
				<< ":SLICE_X" << (flComponentCordReal[moduleName]).x + (flComponentDimension[moduleName]).x
				<< "Y" << (flComponentCordReal[moduleName]).y + (flComponentDimension[moduleName]).y << ";" << endl;
			constraintString << "AREA_GROUP \"pblock_" << instanceName << "\" GROUP=OPEN;" << endl;
			constraintString << "AREA_GROUP \"pblock_" << instanceName << "\" PLACE=OPEN;" << endl;
			//add constraints for DSPs
			if((subComponents[moduleName])->reHelper->estimatedCountMultiplier != 0){
				vector<int> dspPositions;
				int dspInColumn = (flComponentDimension[moduleName]).y/(target->dspHeightInLUT);
				
				for(unsigned int i=0; i<target->multiplierPosition.size(); i++){
					int currentDSPColumn = target->multiplierPosition[i];
					
					if(currentDSPColumn == (flComponentCordReal[moduleName]).x-1)
						dspPositions.push_back(currentDSPColumn);
					else if((currentDSPColumn>=(flComponentCordReal[moduleName]).x) && (currentDSPColumn<=(flComponentCordReal[moduleName]).x+(flComponentDimension[moduleName]).x))
						dspPositions.push_back(currentDSPColumn);
				}
				
				for(unsigned int i=0; i<dspPositions.size(); i++){
					int currentDSPColumn = dspPositions[i];
					
					constraintString << "AREA_GROUP \"pblock_" << flInstanceNames[moduleName] 
						<< "\" RANGE=DSP48_X" << currentDSPColumn << "Y" << (flComponentCordReal[moduleName]).y/target->dspHeightInLUT
						<< ":DSP48_X" << currentDSPColumn
						<< "Y" << (flComponentCordReal[moduleName]).y/target->dspHeightInLUT + dspInColumn << ";" << endl;
				}
			}
			//add constraints for RAMs
			if((subComponents[moduleName])->reHelper->estimatedCountMemory != 0){
				vector<int> ramPositions;
				int ramInColumn = (flComponentDimension[moduleName]).y/target->dspHeightInLUT;
				
				for(unsigned int i=0; i<target->multiplierPosition.size(); i++){
					int currentRAMColumn = target->multiplierPosition[i];
					
					if(currentRAMColumn == (flComponentCordReal[moduleName]).x-1)
						ramPositions.push_back(currentRAMColumn);
					else if((currentRAMColumn>=(flComponentCordReal[moduleName]).x) && (currentRAMColumn<=(flComponentCordReal[moduleName]).x+(flComponentDimension[moduleName]).x))
						ramPositions.push_back(currentRAMColumn);
				}
				
				for(unsigned int i=0; i<ramPositions.size(); i++){
					int currentRAMColumn = ramPositions[i];
					
					if(target->getID() == "Virtex4" || target->getID() == "Spartan3"){
					constraintString << "AREA_GROUP \"pblock_" << flInstanceNames[moduleName] 
						<< "\" RANGE=RAMB16_X" << currentRAMColumn << "Y" << (flComponentCordReal[moduleName]).y/target->ramHeightInLUT
						<< ":RAMB16_X" << currentRAMColumn
						<< "Y" << (flComponentCordReal[moduleName]).y/target->ramHeightInLUT + ramInColumn << ";" << endl;
					}else{
					constraintString << "AREA_GROUP \"pblock_" << flInstanceNames[moduleName] 
						<< "\" RANGE=RAMB36_X" << currentRAMColumn << "Y" << (flComponentCordReal[moduleName]).y/target->ramHeightInLUT
						<< ":RAMB36_X" << currentRAMColumn
						<< "Y" << (flComponentCordReal[moduleName]).y/target->ramHeightInLUT + ramInColumn << ";" << endl;	
					}
				}
			}
			//end of constraint
			constraintString << endl;
		}else if(target->getVendor() == "Altera"){
			//add constraints for function generators
			
			//add constraints for registers
			
			//add constraints for DSPs
			
			//add constraints for RAMs
			
		}
		
		return constraintString.str();
	}
	
	std::string FloorplanningHelper::createFloorplan(){
		ostringstream result;
		
		cerr << "=========================================================================" << endl;
		cerr << "*                          Floorplanning                                *" << endl;
		cerr << "=========================================================================" << endl;
		cerr << "Starting the creation of the floorplan for operator " << parentOp->getName() << endl;
		cerr << "***Triggered creation of the virtual arrangement of the sub-components" << endl;
		result << createVirtualGrid();
		cerr << "***Triggered processing of placement and connectivity constraints" << endl;
		result << processConstraints();
		cerr << "***Triggered creation of the actual arrangement of the sub-components" << endl;
		result << createPlacementGrid();
		cerr << "***Triggered creation of the constraints file" << endl;
		result << createConstraintsFile();
		cerr << "Finished creating the floorplan for operator " << parentOp->getName() << endl;
		cerr << "=========================================================================" << endl;
		
		return result.str();
	}
}
