Parameterized Automatic Cache Generator for Stratix FPGAs

Created by:
	Peter Yiannacouras under the supervision of professor 
	Jonathan Rose for partial fulfillment of BASc requirements
	of the Engineering Science program at the University of
	Toronto.  April 2003.
	Send emails to yiannac@ecf.utoronto.ca


Installation:

	type 'make' 


Use:

	run the 'cachegen' executable with no arguments to see Usage info.
	Note that data and address widths are given in bits and depth in words.
	Running 'make sample' will produce a sample cache in a directory 
	called 'sample'.  
	
	OR

	use the genall.sh script to generate all cache variants for a given
	dimension.   For example 'sh genall.sh 32 16 24 test' creates a 32 
	word deep cach with address width of 16 bits and data width of 24 
	bits.  Type 'make samplegenall' for an example



