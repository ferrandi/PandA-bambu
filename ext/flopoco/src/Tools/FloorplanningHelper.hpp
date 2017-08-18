/*
  Tools for performing floorplanning
 
  Authors:   Matei Istoan, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 
  2012 

  All Rights Reserved
*/


#ifndef FLOORPLANNINGHELPER_HPP
#define FLOORPLANNINGHELPER_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"
#include "utils.hpp"
#include "Tools/ResourceEstimationHelper.hpp"

using namespace std;



//Direction of placement constraints
#define ABOVE						0
#define UNDER						1
#define TO_LEFT_OF					2
#define TO_RIGHT_OF					3

#define ABOVE_WITH_EXTRA			4
#define UNDER_WITH_EXTRA			5
#define TO_LEFT_OF_WITH_EXTRA		6
#define TO_RIGHT_OF_WITH_EXTRA		7

//Type of module constraint
#define PLACEMENT 					0
#define CONNECTIVITY				1
#define ASPECT						2
#define CONTENT						3

//Type of module content constraint
#define ADDER	 					0
#define MULTIPLIER					1



namespace flopoco{


	class FloorplanningHelper
	{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Data types used for floorplanning
	struct coordinateType
	{
		int x;
		int y;
	};
	struct constraintType
	{
		int type;				/**< The constraint type: placement or connection */
		string source;			/**< The origin component */
		string sink;			/**< The destination module */
		int value;				/**< The value of the constraint, both for placement, connectivity and content; 
									 for connectivity constraints, it represents the number of wires, or 
									 the level (strong, weak, average etc.) 
									 for content, specifies the type of content
								*/
		int specialValue;		/**< The specific value of the constraint: for content, this is usually a bit-width */
		double ratio;			/**< The aspect ratio; it is computed as ratio = width/height */
	};
	/////////////////////////////////////////////////////////////////////////////////////////////////	
		
	public:
		FloorplanningHelper(Target* target_, Operator* op_);
	
		/** The destructor */
		~FloorplanningHelper();
		
		
		/**
		 * Initialize the resources used for the floorplanning process. This
		 * function should be called before the other functions used for 
		 * floorplanning are used.
		 * @param ratio the floorplanning ratio: to what degree should the 
		 * bounding boxes be filled (has a default value of 0.75)
		 */
		void initFloorplanning(double ratio = 0.75);
		
		/**
		 * Count the resources that have been added (as glue logic), since 
		 * the last module has been instantiated. It will create a virtual 
		 * module that is placed between the real modules, and that accounts 
		 * for the space needed for the glue logic.
		 * Possibly to be integrated in the instance() method, as the 
		 * process can be done without the intervention of the user.
		 * Uses and updates the pastEstimation... set of variables.
		 * @return the string summarizing the operation
		 */
		std::string manageFloorplan();
		
		/**
		 * Add a new placement constraint between the @source and @sink 
		 * modules. The constraint should be read as: "@sink is @type of @source".
		 * The type of the constraint should be one of the following 
		 * predefined constants: TO_LEFT_OF, TO_RIGHT_OF, ABOVE, UNDER.
		 * NOTE: @source and @sink are the operators' names, NOT 
		 * the instances' names
		 * @param source the source sub-component
		 * @param sink the sink sub-component
		 * @param type the constraint type (has as value predefined constant)
		 * @return the string summarizing the operation
		 */
		std::string addPlacementConstraint(std::string source, std::string sink, int type);
		
		/**
		 * Add a new connectivity constraint between the @source and @sink 
		 * modules. The constraint should be read as: "@sink is connected 
		 * to @source by @nrWires wires".
		 * NOTE: @source and @sink are the operators' names, NOT 
		 * the instances' names
		 * @param source the source sub-component
		 * @param sink the sink sub-component
		 * @param nrWires the number of wires that connect the two modules
		 * @return the string summarizing the operation
		 */
		std::string addConnectivityConstraint(std::string source, std::string sink, int nrWires);
		
		/**
		 * Add a new aspect constraint for @source module. The constraint 
		 * should be read as: "@source's width is @ratio times larger than 
		 * its width".
		 * @param source the source sub-component
		 * @param ratio the aspect ratio
		 * @return the string summarizing the operation
		 */
		std::string addAspectConstraint(std::string source, double ratio);
		
		/**
		 * Add a new constraint for @source module, regarding the contents 
		 * of the module. The constraint gives an indication on the possible 
		 * size/shape constraints, depending what the module contains.
		 * @param source the source sub-component
		 * @param value the type of content constraint
		 * @param length the length, if needed, of the component (for 
		 * example for adders or multipliers)
		 * @return the string summarizing the operation
		 */
		std::string addContentConstraint(std::string source, int value, int length);
		
		/**
		 * Process the placement and connectivity constraints that the 
		 * user has input using the corresponding functions.
		 * Start by processing the placement constraints and then, when 
		 * needed, process the connectivity constraints
		 * @return the string summarizing the operation
		 */
		std::string processConstraints();
		
		/**
		 * Create the virtual grid for the sub-components.
		 * @return the string summarizing the operation
		 */
		std::string createVirtualGrid();
		
		/**
		 * Transform the virtual placement grid into the actual placement on 
		 * the device, ready to generate the actual constraints file.
		 * Aspect and content constraints are also processed (when existent) in 
		 * order to have the right size/shape for the modules.
		 * @return the string summarizing the operation
		 */
		std::string createPlacementGrid();
		
		/**
		 * Create the file that will contain the floorplanning constraints.
		 * @return the string summarizing the operation
		 */
		std::string createConstraintsFile();
		
		/**
		 * Generate the placement for a given module.
		 * @param moduleName the name of the module
		 * @return the string summarizing the operation
		 */
		std::string createPlacementForComponent(std::string moduleName);
		
		/**
		 * Create the floorplan, according the flow described in each 
		 * function and according to the user placed constraints.
		 */
		std::string createFloorplan();
		
		/**
		 * Add a new component to the lost of components
		 * @param name the name of the sub-component
		 */
		void addToFlpComponentList(std::string name);
		
		/**
		 * Create the correspondance between a component and the instance 
		 * name that is assigned to it.
		 * @param componentName the name of the sub-component
		 * @param instanceName the name of the instance
		 */
		void addToInstanceNames(std::string componentName, std::string instanceName);
		
		
		//public class variables
		Operator* parentOp;
		Target* target;
		
		double 					floorplanningRatio;				/**< The level to which the areas in the floorplan are filled to */	
	
		vector<string> 				  flComponentList;				/**< The list of sub-components in the order in which they are instantiated */
		map<string, string>		  	  flInstanceNames;				/**< The list of eqivalences between instance names and components */
		map<string, Operator*>		  flVirtualComponentList;		/**< The list of virtual components, created for florplanning */
		map<string, coordinateType> flComponentCordVirtual;		/**< The coordinates of the sub-components in the virtual grid */
		map<string, coordinateType> flComponentCordReal;			/**< The coordinates of the sub-components in the real grid */
		map<string, coordinateType> flComponentDimension;			/**< The dimensions of the sub-components in the real grid */
		vector<constraintType> 	  flPlacementConstraintList;	/**< The list of placement constraints between components */
		vector<constraintType> 	  flConnectivityConstraintList;	/**< The list of placement constraints between components */
		vector<constraintType> 	  flAspectConstraintList;		/**< The list of placement constraints between components */
		vector<constraintType> 	  flContentConstraintList;		/**< The list of placement constraints between components */
		
		int 						  virtualModuleId;				/**< The unique identifier of each virtual module created, assigned in the order of their appearance */
		
		int 						  prevEstimatedCountFF;			/**< The previous count (at the last estimation) of flip-flops used in the design */
		int 						  prevEstimatedCountLUT;		/**< The previous count (at the last estimation) of function generators used in the design */
		int 						  prevEstimatedCountMultiplier;	/**< The previous count (at the last estimation) of dedicated multipliers used in the design */
		int 						  prevEstimatedCountMemory;		/**< The previous count (at the last estimation) of block memory elements used in the design */
	};
	
}


#endif
