/*
   A class used for plotting various drawings in SVG format

   This file is part of the FloPoCo project
   developed by the Arenaire team at Ecole Normale Superieure de Lyon

Author : Florent de Dinechin, Kinga Illyes, Bogdan Popa

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2012.
All rights reserved.

*/

#include "Plotter.hpp"


using namespace std;

namespace flopoco
{

	Plotter::Snapshot::Snapshot(vector<list<WeightedBit*> > bitheap, int minWeight_, 
			int maxWeight_, unsigned maxHeight_, bool didCompress_,  int cycle_, double cp_):
		maxWeight(maxWeight_), minWeight(minWeight_), maxHeight(maxHeight_), didCompress(didCompress_) , 
		cycle(cycle_), cp(cp_)
	{
		for(int w=minWeight; w<maxWeight_; w++)
		{
			list<WeightedBit*> t;

			if(bitheap[w].size()>0)	
			{
				for(list<WeightedBit*>::iterator it = bitheap[w].begin(); it!=bitheap[w].end(); ++it)	
				{
					WeightedBit* b = new WeightedBit(*it);
					t.push_back(b);
				}		

				bits.push_back(t);
			}
		}
	}

	
	bool operator< (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return ((b1.cycle<b2.cycle) || (b1.cycle==b2.cycle && b1.cp<b2.cp));
	}
	bool operator<= (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return ((b1.cycle<b2.cycle) || (b1.cycle==b2.cycle && b1.cp<=b2.cp));
	}
	bool operator== (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return (b1.cycle==b2.cycle && b1.cp==b2.cp);
	}
	bool operator!= (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return ((b1.cycle!=b2.cycle) || (b1.cycle==b2.cycle && b1.cp!=b2.cp));
	}
	bool operator> (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.cp>b2.cp));
	}
	bool operator>= (Plotter::Snapshot& b1, Plotter::Snapshot& b2){
		
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.cp>=b2.cp));
	}


	Plotter::Plotter(BitHeap* bh_):bh(bh_)
	{
		srcFileName = bh_->getOp()->getSrcFileName() + ":Plotter";
		smallMultIndex = 0;
	}



	Plotter::~Plotter()
	{

	}



	void Plotter::heapSnapshot(bool compress, int cycle, double cp)
	{
		if(!(cp==0.0 && cycle==0))
			if(compress)
			{
				unsigned size=snapshots.size();
				Snapshot* s = new Snapshot(bh->bits, bh->getMinWeight(), bh->getMaxWeight(), bh->getMaxHeight(), compress, cycle, cp);
				bool proceed=true;
				
				if (size==0)
				{
					snapshots.push_back(s);
				}
				else
				{
					vector<Snapshot*>::iterator it = snapshots.begin();

					it++;
					while(proceed) 
					{
						if (it==snapshots.end() || (*s < **it))
						{ // test in this order to avoid segfault!
							snapshots.insert(it, s);
							proceed=false;
						}
						else 
						{
							it++;
						}
					}
				}
			}
	}



	void Plotter::plotBitHeap()
	{
		drawInitialHeap();
		drawCompressionHeap();
	}



	void Plotter::setBitHeap(BitHeap* bh_)
	{
		bh = bh_;
	}



	void Plotter::plotMultiplierConfiguration(string name, vector<MultiplierBlock*> mulBlocks, 
			int wX, int wY, int wOut, int g)
	{
		drawAreaView(name, mulBlocks, wX, wY, wOut, g);
		drawLozengeView(name, mulBlocks, wX, wY, wOut, g);
	}



	void Plotter::addSmallMult(int topX_, int topY_, int dx_, int dy_)
	{
		topX[smallMultIndex]=topX_;
		topY[smallMultIndex]=topY_;
		dx=dx_;
		dy=dy_;
		smallMultIndex++;
	}



	void Plotter::drawInitialHeap()
	{
		initializeHeapPlotting(true);

		int offsetY = 0;
		int turnaroundX = snapshots[0]->bits.size() * 10 + 80;


		offsetY += 20 + snapshots[0]->maxHeight * 10;

		drawInitialConfiguration(snapshots[0]->bits, snapshots[0]->minWeight, offsetY, turnaroundX);

		fig << "<line x1=\"" << turnaroundX + 30 << "\" y1=\"" << 20 << "\" x2=\"" << turnaroundX + 30 
			<< "\" y2=\"" << offsetY +30 << "\" style=\"stroke:midnightblue;stroke-width:1\" />" << endl;

		fig << "<rect class=\"tooltip_bg\" id=\"tooltip_bg\" x=\"0\" y=\"0\" rx=\"4\" ry=\"4\" width=\"55\" height=\"17\" visibility=\"hidden\"/>" << endl;
		fig << "<text class=\"tooltip\" id=\"tooltip\" x=\"0\" y=\"0\" visibility=\"hidden\">Tooltip</text>" << endl;

		fig << "</svg>" << endl;

		fig.close();
	}



	void Plotter::drawCompressionHeap()
	{
		initializeHeapPlotting(false);

		int offsetY = 0;
		int turnaroundX = snapshots[snapshots.size()-1]->maxWeight * 10 + 100;

		//lastStage=snapshots[0]->stage;

		bool timeCondition;


		for(unsigned i=0; i< snapshots.size(); i++)
		{
			if(snapshots[i]->didCompress)
			{
				timeCondition = true;
				if (i > snapshots.size()-3)
					timeCondition = false;

				offsetY += 15 + snapshots[i]->maxHeight * 10;
				drawConfiguration(snapshots[i]->bits, i,snapshots[i]->cycle, snapshots[i]->cp, 
						snapshots[i]->minWeight, offsetY, turnaroundX, timeCondition);

				if (i!=snapshots.size()-1)
				{
					int j=i+1;

					while(snapshots[j]->didCompress==false)
						j++;

					int c = snapshots[j]->cycle - snapshots[i]->cycle;

					fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\"" 
						<< offsetY + 10 << "\" x2=\"" << 50
						<< "\" y2=\"" << offsetY + 10 << "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;

					while(c>0)
					{
						offsetY += 10;
						fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\"" 
							<< offsetY  << "\" x2=\"" << 50
							<< "\" y2=\"" << offsetY  << "\" style=\"stroke:midnightblue;stroke-width:2\" />" << endl;

						c--;
					}
				}
			}
		}

		fig << "<line x1=\"" << turnaroundX + 30 << "\" y1=\"" << 20 << "\" x2=\"" << turnaroundX + 30 
			<< "\" y2=\"" << offsetY +30 << "\" style=\"stroke:midnightblue;stroke-width:1\" />" << endl;

		fig << "<rect class=\"tooltip_bg\" id=\"tooltip_bg\" x=\"0\" y=\"0\" rx=\"4\" ry=\"4\" width=\"55\" height=\"17\" visibility=\"hidden\"/>" << endl;
		fig << "<text class=\"tooltip\" id=\"tooltip\" x=\"0\" y=\"0\" visibility=\"hidden\">Tooltip</text>" << endl;

		fig << "</svg>" << endl;

		fig.close();
	}



	void Plotter::drawAreaView(string name, vector<MultiplierBlock*> mulBlocks, int wX, int wY, int wOut, int g)
	{
		ostringstream figureFileName;
		figureFileName << "tiling_square_" << name << ".svg";

		FILE* pfile;
		pfile  = fopen(figureFileName.str().c_str(), "w");
		fclose(pfile);

		fig.open (figureFileName.str().c_str(), ios::trunc);


		//scaling factor for the whole drawing
		int scalingFactor = 5;

		//offsets for the X and Y axes
		int offsetX = 150;
		int offsetY = 180;

		//file header
		fig << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
			<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << endl
			<< "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl
			<< "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" onload=\"init(evt)\" >" << endl;

		addECMAFunction();
		
		//draw target rectangle
		drawTargetFigure(wX, wY, offsetX, offsetY, scalingFactor, true);
		
		//draw the small multipliers
		for(int i=0; i<smallMultIndex; i++)
		{
			drawSmallMult(wX, wY,  topX[i], topY[i], topX[i] + dx, topY[i] + dy, offsetX, offsetY, scalingFactor, true);
		}

		//draw DSPs
		int xT, yT, xB, yB;

		for(unsigned i=0; i<mulBlocks.size(); i++)
		{
			xT = mulBlocks[i]->gettopX();
			yT = mulBlocks[i]->gettopY();
			xB = mulBlocks[i]->getbotX();
			yB = mulBlocks[i]->getbotY();
			drawDSP(wX, wY, i, xT, yT, xB, yB, offsetX, offsetY, scalingFactor, true);
		}

		//draw truncation line
		if(wX+wY-wOut > 0)
		{
			drawLine(wX, wY, wOut, offsetX, offsetY, scalingFactor, true);    
		}

		//draw guard line
		if(g>0)
		{
			drawLine(wX, wY, wOut+g, offsetX, offsetY, scalingFactor, true);
		}

		fig << "<rect class=\"tooltip_bg\" id=\"tooltip_bg\" x=\"0\" y=\"0\" rx=\"4\" ry=\"4\" width=\"55\" height=\"17\" visibility=\"hidden\"/>" << endl;
		fig << "<text class=\"tooltip\" id=\"tooltip\" x=\"0\" y=\"0\" visibility=\"hidden\">Tooltip</text>" << endl;
		
		fig << "</svg>" << endl;

		fig.close();
	}



	void Plotter::drawLozengeView(string name, vector<MultiplierBlock*> mulBlocks, int wX, int wY, int wOut, int g)
	{
		ostringstream figureFileName;
		figureFileName << "tiling_sheared_" << name << ".svg";

		FILE* pfile;
		pfile  = fopen(figureFileName.str().c_str(), "w");
		fclose(pfile);

		fig.open (figureFileName.str().c_str(), ios::trunc);

		//scaling factor for the whole drawing
		int scalingFactor = 5;

		//offsets for the X and Y axes
		int offsetX = 180 + wY*scalingFactor;
		int offsetY = 180;

		//file header
		fig << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
			<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << endl
			<< "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl
			<< "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" onload=\"init(evt)\" >" << endl;

		addECMAFunction();

		//draw target lozenge
		drawTargetFigure(wX, wY, offsetX, offsetY, scalingFactor, false);
		
		//draw the small multipliers
		for(int i=0; i<smallMultIndex; i++)
		{
			drawSmallMult(wX, wY,  topX[i], topY[i], topX[i] + dx, topY[i] + dy, offsetX, offsetY, scalingFactor, false);
		}

		//draw DSPs
		int xT, yT, xB, yB;

		for(unsigned i=0; i<mulBlocks.size(); i++)
		{
			xT = mulBlocks[i]->gettopX();
			yT = mulBlocks[i]->gettopY();
			xB = mulBlocks[i]->getbotX();
			yB = mulBlocks[i]->getbotY();
			drawDSP(wX, wY, i, xT, yT, xB, yB, offsetX, offsetY, scalingFactor, false);
		}


		//draw truncation line
		if(wX+wY-wOut > 0)
		{
			drawLine(wX, wY, wOut, offsetX, offsetY, scalingFactor, false);    
		}

		//draw guard line
		if(g>0)
		{
			drawLine(wX, wY, wOut+g, offsetX, offsetY, scalingFactor, false);
		}

		fig << "<rect class=\"tooltip_bg\" id=\"tooltip_bg\" x=\"0\" y=\"0\" rx=\"4\" ry=\"4\" width=\"55\" height=\"17\" visibility=\"hidden\"/>" << endl;
		fig << "<text class=\"tooltip\" id=\"tooltip\" x=\"0\" y=\"0\" visibility=\"hidden\">Tooltip</text>" << endl;

		fig << "</svg>" << endl;

		fig.close();
	}



	void Plotter::drawLine(int wX, int wY, int wRez, int offsetX, int offsetY, int scalingFactor, bool isRectangle)
	{
		if(isRectangle)
			fig << "<line x1=\"" << offsetX + scalingFactor * (wRez - wY)
				<< "\" y1=\"" << offsetY << "\" x2=\"" << offsetX + scalingFactor*wRez
				<< "\" y2=\"" << offsetY + scalingFactor*wY 
				<< "\" style=\"stroke:rgb(255,0,0);stroke-width:2\"/>" 
				<< " onmousemove=\"ShowTooltip(evt, \'Truncation line\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
		else
			fig << "<line x1=\"" << offsetX + scalingFactor*(wRez - wY) 
				<< "\" y1=\"" << offsetY << "\" x2=\"" << offsetX + scalingFactor*(wRez - wY)
				<< "\" y2=\"" << offsetY + scalingFactor*wY 
				<< "\" style=\"stroke:rgb(255,0,0);stroke-width:2\"/>" 
				<< " onmousemove=\"ShowTooltip(evt, \'Truncation line with guard bits\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
	}



	void Plotter::drawDSP(int wX, int wY, int i, int xT, int yT, int xB, int yB, 
			int offsetX, int offsetY, int scalingFactor,  bool isRectangle)
	{
		//because the X axis is opposing, all X coordinates have to be turned around
		int turnaroundX;
		int textSize = (xB-xT)*0.85;

		int wxDSP = xB - xT;
		int wyDSP = yB - yT;

		int xTT=xT;

		if(xTT<0)
			xTT=-1;
		else
			xTT=xT/wxDSP;

		int yTT=yT;

		if(yTT<0)
			yTT=-1;
		else
			yTT=yT/wyDSP;	

		if(isRectangle)
		{
			turnaroundX = offsetX + wX * scalingFactor;
			fig << "<rect x=\"" << turnaroundX - xB*scalingFactor << "\" y=\"" << yT*scalingFactor + offsetY 
				<< "\" height=\"" << (yB-yT)*scalingFactor << "\" width=\"" << (xB-xT)*scalingFactor
				<< "\" style=\"fill:rgb(200, 200, 200);fill-opacity:0.7;stroke-width:1;stroke:rgb(0,0,0)\"" 
				<< " onmousemove=\"ShowTooltip(evt, \'X[" << xB-1 << ":" << xT << "] * Y[" << yB-1 << ":" << yT << "]\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;

			fig << "<text x=\"" << (2*turnaroundX - scalingFactor*(xT+xB))/2 -12 
				<< "\" y=\"" << ((yT+yB)*scalingFactor)/2 + offsetY + 7
				<< "\" font-size=\"" << textSize << "\" fill=\"blue\">D(" <<  xTT+1<<")("<<  yTT+1  << ")</text>" << endl;
		}
		else 
		{
			turnaroundX = wX * scalingFactor;
			fig << "<polygon points=\"" << turnaroundX - 5*xB + offsetX - 5*yT << "," << 5*yT + offsetY << " "
				<< turnaroundX - 5*xT + offsetX - 5*yT << "," << 5*yT + offsetY << " " 
				<< turnaroundX - 5*xT + offsetX - 5*yB << "," << 5*yB + offsetY << " "
				<< turnaroundX - 5*xB + offsetX - 5*yB << "," << 5*yB + offsetY
				<< "\" style=\"fill:rgb(200, 200, 200);stroke-width:1;fill-opacity:0.7;stroke:rgb(0,0,0)\" "
				<< " onmousemove=\"ShowTooltip(evt, \'X[" << xB-1 << ":" << xT << "] * Y[" << yB-1 << ":" << yT << "]\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;

			fig << "<text x=\"" << (2*turnaroundX - xB*5 - xT*5 + 2*offsetX)/2 - 14 - (yT*5 + yB*5)/2 
				<< "\" y=\"" << ( yT*5 + offsetY + yB*5 + offsetY )/2 + 7 
				<< "\" font-size=\"" << textSize << "\" fill=\"blue\">D(" <<  xTT+1<<")("<<  yTT+1  << ")</text>" << endl;
		}	
	}




	void Plotter::drawTargetFigure(int wX, int wY, int offsetX, int offsetY, int scalingFactor, bool isRectangle)
	{
		if(isRectangle)
			fig << "<rect x=\"" << offsetX << "\" y=\"" << offsetY 
				<< "\" height=\"" << wY * scalingFactor << "\" width=\"" << wX * scalingFactor 
				<<"\" style=\"fill:rgb(255, 0, 0);stroke-width:1;fill-opacity:0.1;stroke:rgb(0,0,0)\"/>" << endl;
		else
			fig << "<polygon points=\"" << offsetX << "," << offsetY << " " 
				<< wX*scalingFactor + offsetX << "," << offsetY << " " 
				<< wX*scalingFactor + offsetX - scalingFactor*wY << "," << wY*scalingFactor + offsetY << " "
				<< offsetX - scalingFactor*wY << "," << wY*scalingFactor + offsetY 	
				<< "\" style=\"fill:rgb(255, 0, 0);stroke-width:1;fill-opacity:0.1;stroke:rgb(0,0,0)\"/>" << endl;
	}



	void Plotter::drawSmallMult(int wX, int wY, int xT, int yT, int xB, 
			int yB, int offsetX, int offsetY, int scalingFactor,  bool isRectangle)
	{
		int turnaroundX;

		if(isRectangle)
		{
			turnaroundX = offsetX + wX * scalingFactor;
			fig << "<rect x=\"" << turnaroundX - xB*scalingFactor << "\" y=\"" << yT*scalingFactor + offsetY 
				<< "\" height=\"" << (yB-yT)*scalingFactor << "\" width=\"" << (xB-xT)*scalingFactor << "\""
				<< " style=\"fill:rgb(255, 228, 196);fill-opacity:0.7;stroke-width:1;stroke:rgb(0,0,0)\""
				<< " onmousemove=\"ShowTooltip(evt, \'X[" << xB-1 << ":" << xT << "] * Y[" << yB-1 << ":" << yT << "]\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
		}   
		else 
		{
			turnaroundX = wX * scalingFactor;
			fig << "<polygon points=\"" << turnaroundX - 5*xB + offsetX - 5*yT << "," << 5*yT + offsetY << " "
				<< turnaroundX - 5*xT + offsetX - 5*yT << "," << 5*yT + offsetY << " " 
				<< turnaroundX - 5*xT + offsetX - 5*yB << "," << 5*yB + offsetY << " "
				<< turnaroundX - 5*xB + offsetX - 5*yB << "," << 5*yB + offsetY
				<< "\" style=\"fill:rgb(255, 228, 196);fill-opacity:0.7;stroke-width:1;stroke:rgb(0,0,0)\""
				<< " onmousemove=\"ShowTooltip(evt, \'X[" << xB-1 << ":" << xT << "] * Y[" << yB-1 << ":" << yT << "]\')\"" 
				<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
		}
	}
	

	void Plotter::drawBit(int cnt, int w, int turnaroundX, int offsetY, int color, int cycle, int cp, string name)
	{
		const std::string colors[] = { "#97bf04","#0f1af2", 
			"orange", "#f5515c",  "lightgreen", "fuchsia", "indianred"};
		int index = color % 7;
		int ci,c1,c2,c3;	//print cp as a number, as a rational number, in nanoseconds

		c3 = cp % 10;
		cp = cp / 10;
		c2 = cp % 10;	
		cp = cp / 10;
		c1 = cp % 10;
		cp = cp / 10;
		ci = cp % 10;

		fig << "<circle cx=\"" << turnaroundX - w*10 - 5 << "\"" 
			<< " cy=\"" << offsetY - cnt*10 - 5 << "\"" 
			<< " r=\"3\"" 
			<< " fill=\"" << colors[index] << "\" stroke=\"black\" stroke-width=\"0.5\"" 
			<< " onmousemove=\"ShowTooltip(evt, \'" << name << ", " << cycle << " : " << ci << "." << c1 << c2 << c3 << " ns\')\""
			<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
	}



	void Plotter::addECMAFunction()
	{
		fig << endl;
		fig << "<style>" << endl <<
				tab << ".caption{" << endl <<
				tab << tab << "font-size: 15px;" << endl <<
				tab << tab << "font-family: Georgia, serif;" << endl <<
				tab << "}" << endl <<
				tab << ".tooltip{" << endl <<
				tab << tab << "font-size: 12px;" << endl <<
				tab << "}" << endl <<
				tab << ".tooltip_bg{" << endl <<
				tab << tab << "fill: white;" << endl <<
				tab << tab << "stroke: black;" << endl <<
				tab << tab << "stroke-width: 1;" << endl <<
				tab << tab << "opacity: 0.85;" << endl <<
				tab << "}" << endl <<
				"</style>" << endl;
		fig << endl;
		fig << "<script type=\"text/ecmascript\"> <![CDATA[  " << endl <<
				tab << "function init(evt) {" << endl <<
				tab << tab << "if ( window.svgDocument == null ) {" << endl <<
				tab << tab << tab << "svgDocument = evt.target.ownerDocument;" << endl <<
				tab << tab << "}" << endl <<
				tab << tab << "tooltip = svgDocument.getElementById('tooltip');" << endl << 
				tab << tab << "tooltip_bg = svgDocument.getElementById('tooltip_bg');" << endl << 
				tab << "}" << endl <<
				tab << "function ShowTooltip(evt, mouseovertext) {" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"x\",evt.clientX+10);" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"y\",evt.clientY+30);" << endl <<
				tab << tab << "tooltip.firstChild.data = mouseovertext;" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"visibility\",\"visible\");" << endl <<
				endl <<
				tab << tab << "length = tooltip.getComputedTextLength();" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"width\",length+8);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"x\",evt.clientX+7);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"y\",evt.clientY+18);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"visibility\",\"visible\");" << endl <<
				tab << "}" << endl <<
				tab << "function HideTooltip(evt) {" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"visibility\",\"hidden\");" << endl << 
				tab << tab << "tooltip_bg.setAttributeNS(null,\"visibility\",\"hidden\");" << endl << 
				tab << "}]]>" << endl << 
				"</script>" << endl;
		fig << endl;
	}



	void Plotter::initializeHeapPlotting(bool isInitial)
	{
		ostringstream figureFileName;
		
		if(isInitial)
			figureFileName << "BitHeap_initial_" << bh->getName()  << ".svg";
		else 
			figureFileName << "BitHeap_compression_" << bh->getName()  << ".svg";

		FILE* pfile;
		pfile  = fopen(figureFileName.str().c_str(), "w");
		fclose(pfile);

		fig.open (figureFileName.str().c_str(), ios::trunc);

		fig << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
			<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << endl
			<< "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl
			<< "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" onload=\"init(evt)\" >" << endl;

		addECMAFunction();
	}



	void Plotter::drawInitialConfiguration(vector<list<WeightedBit*> > bits, int minWeight, int offsetY, int turnaroundX)
	{
		int color = 0;
		int cnt = 0;
		vector<WeightedBit*> orderedBits;

		fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\"" 
			<< offsetY +10 << "\" x2=\"" << turnaroundX - bits.size()*10 - 50
			<< "\" y2=\"" << offsetY +10 << "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;

		for(unsigned i=minWeight; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				for(list<WeightedBit*>::iterator bit = bits[i].begin(); bit!=bits[i].end(); ++bit)
				{
					if(orderedBits.size()==0)
					{
						orderedBits.push_back((*bit));
					}
					bool proceed=true;
					vector<WeightedBit*>::iterator iterBit = orderedBits.begin();

					while(proceed)
					{
						if (iterBit==orderedBits.end())
						{
							orderedBits.push_back((*bit));
							proceed=false;
						}
						else
						{
							if( (**bit) == (**iterBit))
							{
								proceed=false;
							}
							else
							{
								if( (**bit) < (**iterBit))
								{
									orderedBits.insert(iterBit, *bit);
									proceed=false;
								}
								else
								{
									iterBit++;
								}
							}

						}
					}
				}
			}
		}

		for(unsigned i=0; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				cnt = 0;
				for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); ++it)
				{
					color=0;

					for (unsigned j=0; j<orderedBits.size(); j++)
					{
						if ( (**it) == (*orderedBits[j]) )
							color=j;
					}
					
					int cy = (*it)->getCycle();
					double cp = (*it)->getCriticalPath(cy)*100000000000;
					
					drawBit(cnt, i, turnaroundX, offsetY, color, cy, cp, (*it)->getName());
					cnt++;
				}
			}
		}
	}



	void Plotter::drawConfiguration(vector<list<WeightedBit*> > bits,unsigned nr, int cycle, double criticalPath,
			int minWeight, int offsetY, int turnaroundX, bool timeCondition)
	{
		int cnt = 0;
		int ci,c1,c2,c3;							//print cp as a number as a rational number, in nanoseconds
		int cpint = criticalPath * 1000000000000;

		c3 = cpint % 10;
		cpint = cpint / 10;
		c2 = cpint % 10;	
		cpint = cpint / 10;
		c1 = cpint % 10;
		cpint = cpint / 10;
		ci = cpint % 10;

		if(nr == 0)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
				<< "\" fill=\"midnightblue\">" << "before first compression" << "</text>" << endl;
		}else if(nr == snapshots.size()-1)
		{			
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
				<< "\" fill=\"midnightblue\">" << "before final addition" << "</text>" << endl;
		}else if(nr == snapshots.size()-2)
		{			
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
				<< "\" fill=\"midnightblue\">" << "before 3-bit height additions" << "</text>" << endl;
		}else
		{			
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
				<< "\" fill=\"midnightblue\">" << cycle << "</text>" << endl
				<< "<text x=\"" << turnaroundX + 80 << "\" y=\"" << offsetY + 3
				<< "\" fill=\"midnightblue\">" << ci << "." << c1 << c2 << c3 << " ns"  << "</text>" << endl;
		}
		
		turnaroundX -= minWeight*10;

		for(unsigned i=0; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				cnt = 0;
				for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); ++it)
				{
					int cy = (*it)->getCycle();
					double cp = (*it)->getCriticalPath(cy);
					
					if (timeCondition)
					{
						if ((cy<cycle) || ((cy==cycle) && (cp<=criticalPath)))
						{
							cp = cp * 1000000000000;  //picoseconds
							drawBit(cnt, i, turnaroundX, offsetY, (*it)->getType(), cy, cp, (*it)->getName());
							cnt++;
						}
					}
					else
					{
						drawBit(cnt, i, turnaroundX, offsetY, (*it)->getType(), cy, cp, (*it)->getName());
						cnt++;
					}
				}
			}
		}
	}



}

