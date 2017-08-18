#include <iostream>
#include <math.h>
#include <cstdlib>
#include "../utils.hpp"
#include "PolynomialTable.hpp"
using namespace std;

namespace flopoco{


	PolynomialTable::PolynomialTable(Target* target, int wIn, int wOut) : 
		Table(target, wIn, wOut)  
	{
		ostringstream name; 
		name <<"InvTable_0_"<<wIn<<"_"<<wOut;
		setName(name.str());

	}

	PolynomialTable::~PolynomialTable() {}
  

	int    PolynomialTable::double2input(double x){
		int result;
		cerr << "??? PolynomialTable::double2input not yet implemented ";
		exit(1);
		return result;
	}


	double PolynomialTable::input2double(int x) {
		double y;
		cerr << "??? PolynomialTable::double2input not yet implemented ";
		exit(1);
		return(y);
	}

	mpz_class PolynomialTable::double2output(double x){
		cerr << "??? PolynomialTable::double2input not yet implemented ";
		exit(1);
		return 0;
	}

	double PolynomialTable::output2double(mpz_class x) {
		double y;
		cerr << "??? PolynomialTable::double2input not yet implemented ";
		exit(1);
  
		return(y);
	}


	mpz_class PolynomialTable::function(int x)
	{
		mpz_class r=0;

		switch(x) {
 

		case 0 : r= 
				mpz_class( 67108868 )+(mpz_class( 131071 )<< 27 )+(mpz_class( 508 )<< 44 );
			break;
		case 1 : r= 
				mpz_class( 67370502 )+(mpz_class( 130562 )<< 27 )+(mpz_class( 502 )<< 44 );
			break;
		case 2 : r= 
				mpz_class( 67631124 )+(mpz_class( 130059 )<< 27 )+(mpz_class( 497 )<< 44 );
			break;
		case 3 : r= 
				mpz_class( 67890745 )+(mpz_class( 129563 )<< 27 )+(mpz_class( 494 )<< 44 );
			break;
		case 4 : r= 
				mpz_class( 68149378 )+(mpz_class( 129070 )<< 27 )+(mpz_class( 486 )<< 44 );
			break;
		case 5 : r= 
				mpz_class( 68407032 )+(mpz_class( 128584 )<< 27 )+(mpz_class( 480 )<< 44 );
			break;
		case 6 : r= 
				mpz_class( 68663720 )+(mpz_class( 128103 )<< 27 )+(mpz_class( 475 )<< 44 );
			break;
		case 7 : r= 
				mpz_class( 68919451 )+(mpz_class( 127629 )<< 27 )+(mpz_class( 471 )<< 44 );
			break;
		case 8 : r= 
				mpz_class( 69174238 )+(mpz_class( 127157 )<< 27 )+(mpz_class( 463 )<< 44 );
			break;
		case 9 : r= 
				mpz_class( 69428089 )+(mpz_class( 126693 )<< 27 )+(mpz_class( 460 )<< 44 );
			break;
		case 10 : r= 
				mpz_class( 69681015 )+(mpz_class( 126234 )<< 27 )+(mpz_class( 456 )<< 44 );
			break;
		case 11 : r= 
				mpz_class( 69933027 )+(mpz_class( 125779 )<< 27 )+(mpz_class( 451 )<< 44 );
			break;
		case 12 : r= 
				mpz_class( 70184134 )+(mpz_class( 125329 )<< 27 )+(mpz_class( 447 )<< 44 );
			break;
		case 13 : r= 
				mpz_class( 70434346 )+(mpz_class( 124883 )<< 27 )+(mpz_class( 441 )<< 44 );
			break;
		case 14 : r= 
				mpz_class( 70683671 )+(mpz_class( 124443 )<< 27 )+(mpz_class( 436 )<< 44 );
			break;
		case 15 : r= 
				mpz_class( 70932121 )+(mpz_class( 124007 )<< 27 )+(mpz_class( 432 )<< 44 );
			break;
		case 16 : r= 
				mpz_class( 71179703 )+(mpz_class( 123576 )<< 27 )+(mpz_class( 428 )<< 44 );
			break;
		case 17 : r= 
				mpz_class( 71426427 )+(mpz_class( 123149 )<< 27 )+(mpz_class( 423 )<< 44 );
			break;
		case 18 : r= 
				mpz_class( 71672302 )+(mpz_class( 122727 )<< 27 )+(mpz_class( 420 )<< 44 );
			break;
		case 19 : r= 
				mpz_class( 71917336 )+(mpz_class( 122309 )<< 27 )+(mpz_class( 416 )<< 44 );
			break;
		case 20 : r= 
				mpz_class( 72161539 )+(mpz_class( 121893 )<< 27 )+(mpz_class( 408 )<< 44 );
			break;
		case 21 : r= 
				mpz_class( 72404917 )+(mpz_class( 121485 )<< 27 )+(mpz_class( 407 )<< 44 );
			break;
		case 22 : r= 
				mpz_class( 72647480 )+(mpz_class( 121080 )<< 27 )+(mpz_class( 404 )<< 44 );
			break;
		case 23 : r= 
				mpz_class( 72889236 )+(mpz_class( 120678 )<< 27 )+(mpz_class( 399 )<< 44 );
			break;
		case 24 : r= 
				mpz_class( 73130193 )+(mpz_class( 120279 )<< 27 )+(mpz_class( 392 )<< 44 );
			break;
		case 25 : r= 
				mpz_class( 73370359 )+(mpz_class( 119885 )<< 27 )+(mpz_class( 389 )<< 44 );
			break;
		case 26 : r= 
				mpz_class( 73609740 )+(mpz_class( 119497 )<< 27 )+(mpz_class( 388 )<< 44 );
			break;
		case 27 : r= 
				mpz_class( 73848346 )+(mpz_class( 119111 )<< 27 )+(mpz_class( 384 )<< 44 );
			break;
		case 28 : r= 
				mpz_class( 74086184 )+(mpz_class( 118727 )<< 27 )+(mpz_class( 378 )<< 44 );
			break;
		case 29 : r= 
				mpz_class( 74323260 )+(mpz_class( 118349 )<< 27 )+(mpz_class( 375 )<< 44 );
			break;
		case 30 : r= 
				mpz_class( 74559583 )+(mpz_class( 117973 )<< 27 )+(mpz_class( 370 )<< 44 );
			break;
		case 31 : r= 
				mpz_class( 74795159 )+(mpz_class( 117601 )<< 27 )+(mpz_class( 366 )<< 44 );
			break;
		case 32 : r= 
				mpz_class( 75029995 )+(mpz_class( 117234 )<< 27 )+(mpz_class( 365 )<< 44 );
			break;
		case 33 : r= 
				mpz_class( 75264098 )+(mpz_class( 116870 )<< 27 )+(mpz_class( 362 )<< 44 );
			break;
		case 34 : r= 
				mpz_class( 75497476 )+(mpz_class( 116509 )<< 27 )+(mpz_class( 360 )<< 44 );
			break;
		case 35 : r= 
				mpz_class( 75730134 )+(mpz_class( 116151 )<< 27 )+(mpz_class( 356 )<< 44 );
			break;
		case 36 : r= 
				mpz_class( 75962080 )+(mpz_class( 115796 )<< 27 )+(mpz_class( 352 )<< 44 );
			break;
		case 37 : r= 
				mpz_class( 76193320 )+(mpz_class( 115444 )<< 27 )+(mpz_class( 348 )<< 44 );
			break;
		case 38 : r= 
				mpz_class( 76423860 )+(mpz_class( 115096 )<< 27 )+(mpz_class( 345 )<< 44 );
			break;
		case 39 : r= 
				mpz_class( 76653707 )+(mpz_class( 114750 )<< 27 )+(mpz_class( 341 )<< 44 );
			break;
		case 40 : r= 
				mpz_class( 76882866 )+(mpz_class( 114409 )<< 27 )+(mpz_class( 339 )<< 44 );
			break;
		case 41 : r= 
				mpz_class( 77111345 )+(mpz_class( 114070 )<< 27 )+(mpz_class( 337 )<< 44 );
			break;
		case 42 : r= 
				mpz_class( 77339148 )+(mpz_class( 113734 )<< 27 )+(mpz_class( 333 )<< 44 );
			break;
		case 43 : r= 
				mpz_class( 77566283 )+(mpz_class( 113401 )<< 27 )+(mpz_class( 331 )<< 44 );
			break;
		case 44 : r= 
				mpz_class( 77792754 )+(mpz_class( 113071 )<< 27 )+(mpz_class( 328 )<< 44 );
			break;
		case 45 : r= 
				mpz_class( 78018568 )+(mpz_class( 112744 )<< 27 )+(mpz_class( 326 )<< 44 );
			break;
		case 46 : r= 
				mpz_class( 78243731 )+(mpz_class( 112418 )<< 27 )+(mpz_class( 320 )<< 44 );
			break;
		case 47 : r= 
				mpz_class( 78468247 )+(mpz_class( 112097 )<< 27 )+(mpz_class( 319 )<< 44 );
			break;
		case 48 : r= 
				mpz_class( 78692122 )+(mpz_class( 111779 )<< 27 )+(mpz_class( 317 )<< 44 );
			break;
		case 49 : r= 
				mpz_class( 78915363 )+(mpz_class( 111462 )<< 27 )+(mpz_class( 313 )<< 44 );
			break;
		case 50 : r= 
				mpz_class( 79137974 )+(mpz_class( 111148 )<< 27 )+(mpz_class( 310 )<< 44 );
			break;
		case 51 : r= 
				mpz_class( 79359960 )+(mpz_class( 110838 )<< 27 )+(mpz_class( 309 )<< 44 );
			break;
		case 52 : r= 
				mpz_class( 79581327 )+(mpz_class( 110530 )<< 27 )+(mpz_class( 307 )<< 44 );
			break;
		case 53 : r= 
				mpz_class( 79802080 )+(mpz_class( 110224 )<< 27 )+(mpz_class( 304 )<< 44 );
			break;
		case 54 : r= 
				mpz_class( 80022224 )+(mpz_class( 109921 )<< 27 )+(mpz_class( 302 )<< 44 );
			break;
		case 55 : r= 
				mpz_class( 80241764 )+(mpz_class( 109620 )<< 27 )+(mpz_class( 299 )<< 44 );
			break;
		case 56 : r= 
				mpz_class( 80460705 )+(mpz_class( 109322 )<< 27 )+(mpz_class( 297 )<< 44 );
			break;
		case 57 : r= 
				mpz_class( 80679052 )+(mpz_class( 109026 )<< 27 )+(mpz_class( 294 )<< 44 );
			break;
		case 58 : r= 
				mpz_class( 80896810 )+(mpz_class( 108732 )<< 27 )+(mpz_class( 291 )<< 44 );
			break;
		case 59 : r= 
				mpz_class( 81113983 )+(mpz_class( 108441 )<< 27 )+(mpz_class( 289 )<< 44 );
			break;
		case 60 : r= 
				mpz_class( 81330576 )+(mpz_class( 108152 )<< 27 )+(mpz_class( 286 )<< 44 );
			break;
		case 61 : r= 
				mpz_class( 81546594 )+(mpz_class( 107866 )<< 27 )+(mpz_class( 285 )<< 44 );
			break;
		case 62 : r= 
				mpz_class( 81762041 )+(mpz_class( 107581 )<< 27 )+(mpz_class( 281 )<< 44 );
			break;
		case 63 : r= 
				mpz_class( 81976922 )+(mpz_class( 107299 )<< 27 )+(mpz_class( 279 )<< 44 );
			break;
		case 64 : r= 
				mpz_class( 82191241 )+(mpz_class( 107019 )<< 27 )+(mpz_class( 276 )<< 44 );
			break;
		case 65 : r= 
				mpz_class( 82405003 )+(mpz_class( 106742 )<< 27 )+(mpz_class( 276 )<< 44 );
			break;
		case 66 : r= 
				mpz_class( 82618211 )+(mpz_class( 106467 )<< 27 )+(mpz_class( 274 )<< 44 );
			break;
		case 67 : r= 
				mpz_class( 82830871 )+(mpz_class( 106194 )<< 27 )+(mpz_class( 273 )<< 44 );
			break;
		case 68 : r= 
				mpz_class( 83042986 )+(mpz_class( 105923 )<< 27 )+(mpz_class( 271 )<< 44 );
			break;
		case 69 : r= 
				mpz_class( 83254561 )+(mpz_class( 105653 )<< 27 )+(mpz_class( 267 )<< 44 );
			break;
		case 70 : r= 
				mpz_class( 83465600 )+(mpz_class( 105385 )<< 27 )+(mpz_class( 264 )<< 44 );
			break;
		case 71 : r= 
				mpz_class( 83676106 )+(mpz_class( 105121 )<< 27 )+(mpz_class( 264 )<< 44 );
			break;
		case 72 : r= 
				mpz_class( 83886084 )+(mpz_class( 104857 )<< 27 )+(mpz_class( 260 )<< 44 );
			break;
		case 73 : r= 
				mpz_class( 84095538 )+(mpz_class( 104596 )<< 27 )+(mpz_class( 259 )<< 44 );
			break;
		case 74 : r= 
				mpz_class( 84304471 )+(mpz_class( 104337 )<< 27 )+(mpz_class( 257 )<< 44 );
			break;
		case 75 : r= 
				mpz_class( 84512888 )+(mpz_class( 104079 )<< 27 )+(mpz_class( 254 )<< 44 );
			break;
		case 76 : r= 
				mpz_class( 84720792 )+(mpz_class( 103824 )<< 27 )+(mpz_class( 253 )<< 44 );
			break;
		case 77 : r= 
				mpz_class( 84928187 )+(mpz_class( 103571 )<< 27 )+(mpz_class( 252 )<< 44 );
			break;
		case 78 : r= 
				mpz_class( 85135077 )+(mpz_class( 103319 )<< 27 )+(mpz_class( 250 )<< 44 );
			break;
		case 79 : r= 
				mpz_class( 85341465 )+(mpz_class( 103070 )<< 27 )+(mpz_class( 249 )<< 44 );
			break;
		case 80 : r= 
				mpz_class( 85547356 )+(mpz_class( 102821 )<< 27 )+(mpz_class( 246 )<< 44 );
			break;
		case 81 : r= 
				mpz_class( 85752752 )+(mpz_class( 102575 )<< 27 )+(mpz_class( 245 )<< 44 );
			break;
		case 82 : r= 
				mpz_class( 85957657 )+(mpz_class( 102331 )<< 27 )+(mpz_class( 244 )<< 44 );
			break;
		case 83 : r= 
				mpz_class( 86162075 )+(mpz_class( 102088 )<< 27 )+(mpz_class( 242 )<< 44 );
			break;
		case 84 : r= 
				mpz_class( 86366009 )+(mpz_class( 101847 )<< 27 )+(mpz_class( 240 )<< 44 );
			break;
		case 85 : r= 
				mpz_class( 86569463 )+(mpz_class( 101607 )<< 27 )+(mpz_class( 237 )<< 44 );
			break;
		case 86 : r= 
				mpz_class( 86772440 )+(mpz_class( 101369 )<< 27 )+(mpz_class( 235 )<< 44 );
			break;
		case 87 : r= 
				mpz_class( 86974943 )+(mpz_class( 101133 )<< 27 )+(mpz_class( 233 )<< 44 );
			break;
		case 88 : r= 
				mpz_class( 87176976 )+(mpz_class( 100898 )<< 27 )+(mpz_class( 231 )<< 44 );
			break;
		case 89 : r= 
				mpz_class( 87378541 )+(mpz_class( 100666 )<< 27 )+(mpz_class( 230 )<< 44 );
			break;
		case 90 : r= 
				mpz_class( 87579643 )+(mpz_class( 100434 )<< 27 )+(mpz_class( 227 )<< 44 );
			break;
		case 91 : r= 
				mpz_class( 87780284 )+(mpz_class( 100205 )<< 27 )+(mpz_class( 227 )<< 44 );
			break;
		case 92 : r= 
				mpz_class( 87980467 )+(mpz_class( 99977 )<< 27 )+(mpz_class( 225 )<< 44 );
			break;
		case 93 : r= 
				mpz_class( 88180196 )+(mpz_class( 99751 )<< 27 )+(mpz_class( 225 )<< 44 );
			break;
		case 94 : r= 
				mpz_class( 88379473 )+(mpz_class( 99527 )<< 27 )+(mpz_class( 225 )<< 44 );
			break;
		case 95 : r= 
				mpz_class( 88578303 )+(mpz_class( 99302 )<< 27 )+(mpz_class( 220 )<< 44 );
			break;
		case 96 : r= 
				mpz_class( 88776686 )+(mpz_class( 99082 )<< 27 )+(mpz_class( 222 )<< 44 );
			break;
		case 97 : r= 
				mpz_class( 88974628 )+(mpz_class( 98861 )<< 27 )+(mpz_class( 220 )<< 44 );
			break;
		case 98 : r= 
				mpz_class( 89172130 )+(mpz_class( 98642 )<< 27 )+(mpz_class( 218 )<< 44 );
			break;
		case 99 : r= 
				mpz_class( 89369196 )+(mpz_class( 98424 )<< 27 )+(mpz_class( 216 )<< 44 );
			break;
		case 100 : r= 
				mpz_class( 89565828 )+(mpz_class( 98208 )<< 27 )+(mpz_class( 215 )<< 44 );
			break;
		case 101 : r= 
				mpz_class( 89762029 )+(mpz_class( 97994 )<< 27 )+(mpz_class( 214 )<< 44 );
			break;
		case 102 : r= 
				mpz_class( 89957803 )+(mpz_class( 97780 )<< 27 )+(mpz_class( 212 )<< 44 );
			break;
		case 103 : r= 
				mpz_class( 90153151 )+(mpz_class( 97569 )<< 27 )+(mpz_class( 212 )<< 44 );
			break;
		case 104 : r= 
				mpz_class( 90348077 )+(mpz_class( 97358 )<< 27 )+(mpz_class( 210 )<< 44 );
			break;
		case 105 : r= 
				mpz_class( 90542583 )+(mpz_class( 97149 )<< 27 )+(mpz_class( 208 )<< 44 );
			break;
		case 106 : r= 
				mpz_class( 90736673 )+(mpz_class( 96940 )<< 27 )+(mpz_class( 205 )<< 44 );
			break;
		case 107 : r= 
				mpz_class( 90930348 )+(mpz_class( 96734 )<< 27 )+(mpz_class( 205 )<< 44 );
			break;
		case 108 : r= 
				mpz_class( 91123611 )+(mpz_class( 96530 )<< 27 )+(mpz_class( 205 )<< 44 );
			break;
		case 109 : r= 
				mpz_class( 91316466 )+(mpz_class( 96325 )<< 27 )+(mpz_class( 202 )<< 44 );
			break;
		case 110 : r= 
				mpz_class( 91508914 )+(mpz_class( 96122 )<< 27 )+(mpz_class( 200 )<< 44 );
			break;
		case 111 : r= 
				mpz_class( 91700958 )+(mpz_class( 95921 )<< 27 )+(mpz_class( 199 )<< 44 );
			break;
		case 112 : r= 
				mpz_class( 91892601 )+(mpz_class( 95720 )<< 27 )+(mpz_class( 196 )<< 44 );
			break;
		case 113 : r= 
				mpz_class( 92083844 )+(mpz_class( 95523 )<< 27 )+(mpz_class( 198 )<< 44 );
			break;
		case 114 : r= 
				mpz_class( 92274692 )+(mpz_class( 95325 )<< 27 )+(mpz_class( 196 )<< 44 );
			break;
		case 115 : r= 
				mpz_class( 92465146 )+(mpz_class( 95128 )<< 27 )+(mpz_class( 194 )<< 44 );
			break;
		case 116 : r= 
				mpz_class( 92655208 )+(mpz_class( 94933 )<< 27 )+(mpz_class( 193 )<< 44 );
			break;
		case 117 : r= 
				mpz_class( 92844881 )+(mpz_class( 94740 )<< 27 )+(mpz_class( 194 )<< 44 );
			break;
		case 118 : r= 
				mpz_class( 93034167 )+(mpz_class( 94548 )<< 27 )+(mpz_class( 194 )<< 44 );
			break;
		case 119 : r= 
				mpz_class( 93223069 )+(mpz_class( 94356 )<< 27 )+(mpz_class( 192 )<< 44 );
			break;
		case 120 : r= 
				mpz_class( 93411589 )+(mpz_class( 94165 )<< 27 )+(mpz_class( 189 )<< 44 );
			break;
		case 121 : r= 
				mpz_class( 93599730 )+(mpz_class( 93975 )<< 27 )+(mpz_class( 187 )<< 44 );
			break;
		case 122 : r= 
				mpz_class( 93787493 )+(mpz_class( 93787 )<< 27 )+(mpz_class( 187 )<< 44 );
			break;
		case 123 : r= 
				mpz_class( 93974880 )+(mpz_class( 93601 )<< 27 )+(mpz_class( 187 )<< 44 );
			break;
		case 124 : r= 
				mpz_class( 94161895 )+(mpz_class( 93415 )<< 27 )+(mpz_class( 186 )<< 44 );
			break;
		case 125 : r= 
				mpz_class( 94348539 )+(mpz_class( 93231 )<< 27 )+(mpz_class( 186 )<< 44 );
			break;
		case 126 : r= 
				mpz_class( 94534815 )+(mpz_class( 93047 )<< 27 )+(mpz_class( 185 )<< 44 );
			break;
		case 127 : r= 
				mpz_class( 94720724 )+(mpz_class( 92864 )<< 27 )+(mpz_class( 182 )<< 44 );
			break;
		case 128 : r= 
				mpz_class( 94906269 )+(mpz_class( 92682 )<< 27 )+(mpz_class( 180 )<< 44 );
			break;
		case 129 : r= 
				mpz_class( 95276275 )+(mpz_class( 92322 )<< 27 )+(mpz_class( 178 )<< 44 );
			break;
		case 130 : r= 
				mpz_class( 95644851 )+(mpz_class( 91966 )<< 27 )+(mpz_class( 176 )<< 44 );
			break;
		case 131 : r= 
				mpz_class( 96012012 )+(mpz_class( 91613 )<< 27 )+(mpz_class( 173 )<< 44 );
			break;
		case 132 : r= 
				mpz_class( 96377772 )+(mpz_class( 91266 )<< 27 )+(mpz_class( 171 )<< 44 );
			break;
		case 133 : r= 
				mpz_class( 96742150 )+(mpz_class( 90924 )<< 27 )+(mpz_class( 171 )<< 44 );
			break;
		case 134 : r= 
				mpz_class( 97105162 )+(mpz_class( 90583 )<< 27 )+(mpz_class( 168 )<< 44 );
			break;
		case 135 : r= 
				mpz_class( 97466820 )+(mpz_class( 90248 )<< 27 )+(mpz_class( 167 )<< 44 );
			break;
		case 136 : r= 
				mpz_class( 97827143 )+(mpz_class( 89915 )<< 27 )+(mpz_class( 165 )<< 44 );
			break;
		case 137 : r= 
				mpz_class( 98186143 )+(mpz_class( 89586 )<< 27 )+(mpz_class( 163 )<< 44 );
			break;
		case 138 : r= 
				mpz_class( 98543835 )+(mpz_class( 89260 )<< 27 )+(mpz_class( 160 )<< 44 );
			break;
		case 139 : r= 
				mpz_class( 98900233 )+(mpz_class( 88939 )<< 27 )+(mpz_class( 159 )<< 44 );
			break;
		case 140 : r= 
				mpz_class( 99255353 )+(mpz_class( 88620 )<< 27 )+(mpz_class( 157 )<< 44 );
			break;
		case 141 : r= 
				mpz_class( 99609205 )+(mpz_class( 88306 )<< 27 )+(mpz_class( 156 )<< 44 );
			break;
		case 142 : r= 
				mpz_class( 99961805 )+(mpz_class( 87995 )<< 27 )+(mpz_class( 155 )<< 44 );
			break;
		case 143 : r= 
				mpz_class( 100313165 )+(mpz_class( 87686 )<< 27 )+(mpz_class( 152 )<< 44 );
			break;
		case 144 : r= 
				mpz_class( 100663300 )+(mpz_class( 87381 )<< 27 )+(mpz_class( 151 )<< 44 );
			break;
		case 145 : r= 
				mpz_class( 101012220 )+(mpz_class( 87080 )<< 27 )+(mpz_class( 150 )<< 44 );
			break;
		case 146 : r= 
				mpz_class( 101359939 )+(mpz_class( 86781 )<< 27 )+(mpz_class( 148 )<< 44 );
			break;
		case 147 : r= 
				mpz_class( 101706472 )+(mpz_class( 86484 )<< 27 )+(mpz_class( 146 )<< 44 );
			break;
		case 148 : r= 
				mpz_class( 102051825 )+(mpz_class( 86192 )<< 27 )+(mpz_class( 145 )<< 44 );
			break;
		case 149 : r= 
				mpz_class( 102396014 )+(mpz_class( 85903 )<< 27 )+(mpz_class( 144 )<< 44 );
			break;
		case 150 : r= 
				mpz_class( 102739050 )+(mpz_class( 85615 )<< 27 )+(mpz_class( 141 )<< 44 );
			break;
		case 151 : r= 
				mpz_class( 103080945 )+(mpz_class( 85332 )<< 27 )+(mpz_class( 141 )<< 44 );
			break;
		case 152 : r= 
				mpz_class( 103421710 )+(mpz_class( 85050 )<< 27 )+(mpz_class( 139 )<< 44 );
			break;
		case 153 : r= 
				mpz_class( 103761355 )+(mpz_class( 84772 )<< 27 )+(mpz_class( 138 )<< 44 );
			break;
		case 154 : r= 
				mpz_class( 104099891 )+(mpz_class( 84497 )<< 27 )+(mpz_class( 137 )<< 44 );
			break;
		case 155 : r= 
				mpz_class( 104437332 )+(mpz_class( 84223 )<< 27 )+(mpz_class( 135 )<< 44 );
			break;
		case 156 : r= 
				mpz_class( 104773685 )+(mpz_class( 83952 )<< 27 )+(mpz_class( 133 )<< 44 );
			break;
		case 157 : r= 
				mpz_class( 105108960 )+(mpz_class( 83686 )<< 27 )+(mpz_class( 133 )<< 44 );
			break;
		case 158 : r= 
				mpz_class( 105443171 )+(mpz_class( 83420 )<< 27 )+(mpz_class( 131 )<< 44 );
			break;
		case 159 : r= 
				mpz_class( 105776326 )+(mpz_class( 83158 )<< 27 )+(mpz_class( 131 )<< 44 );
			break;
		case 160 : r= 
				mpz_class( 106108435 )+(mpz_class( 82897 )<< 27 )+(mpz_class( 129 )<< 44 );
			break;
		case 161 : r= 
				mpz_class( 106439508 )+(mpz_class( 82639 )<< 27 )+(mpz_class( 128 )<< 44 );
			break;
		case 162 : r= 
				mpz_class( 106769553 )+(mpz_class( 82384 )<< 27 )+(mpz_class( 127 )<< 44 );
			break;
		case 163 : r= 
				mpz_class( 107098582 )+(mpz_class( 82130 )<< 27 )+(mpz_class( 125 )<< 44 );
			break;
		case 164 : r= 
				mpz_class( 107426602 )+(mpz_class( 81880 )<< 27 )+(mpz_class( 124 )<< 44 );
			break;
		case 165 : r= 
				mpz_class( 107753625 )+(mpz_class( 81631 )<< 27 )+(mpz_class( 123 )<< 44 );
			break;
		case 166 : r= 
				mpz_class( 108079658 )+(mpz_class( 81385 )<< 27 )+(mpz_class( 122 )<< 44 );
			break;
		case 167 : r= 
				mpz_class( 108404710 )+(mpz_class( 81141 )<< 27 )+(mpz_class( 121 )<< 44 );
			break;
		case 168 : r= 
				mpz_class( 108728791 )+(mpz_class( 80899 )<< 27 )+(mpz_class( 120 )<< 44 );
			break;
		case 169 : r= 
				mpz_class( 109051907 )+(mpz_class( 80660 )<< 27 )+(mpz_class( 119 )<< 44 );
			break;
		case 170 : r= 
				mpz_class( 109374071 )+(mpz_class( 80422 )<< 27 )+(mpz_class( 118 )<< 44 );
			break;
		case 171 : r= 
				mpz_class( 109695287 )+(mpz_class( 80187 )<< 27 )+(mpz_class( 117 )<< 44 );
			break;
		case 172 : r= 
				mpz_class( 110015567 )+(mpz_class( 79953 )<< 27 )+(mpz_class( 116 )<< 44 );
			break;
		case 173 : r= 
				mpz_class( 110334915 )+(mpz_class( 79722 )<< 27 )+(mpz_class( 115 )<< 44 );
			break;
		case 174 : r= 
				mpz_class( 110653344 )+(mpz_class( 79492 )<< 27 )+(mpz_class( 114 )<< 44 );
			break;
		case 175 : r= 
				mpz_class( 110970857 )+(mpz_class( 79265 )<< 27 )+(mpz_class( 113 )<< 44 );
			break;
		case 176 : r= 
				mpz_class( 111287466 )+(mpz_class( 79039 )<< 27 )+(mpz_class( 112 )<< 44 );
			break;
		case 177 : r= 
				mpz_class( 111603174 )+(mpz_class( 78816 )<< 27 )+(mpz_class( 111 )<< 44 );
			break;
		case 178 : r= 
				mpz_class( 111917994 )+(mpz_class( 78594 )<< 27 )+(mpz_class( 110 )<< 44 );
			break;
		case 179 : r= 
				mpz_class( 112231930 )+(mpz_class( 78374 )<< 27 )+(mpz_class( 109 )<< 44 );
			break;
		case 180 : r= 
				mpz_class( 112544990 )+(mpz_class( 78157 )<< 27 )+(mpz_class( 109 )<< 44 );
			break;
		case 181 : r= 
				mpz_class( 112857182 )+(mpz_class( 77940 )<< 27 )+(mpz_class( 107 )<< 44 );
			break;
		case 182 : r= 
				mpz_class( 113168513 )+(mpz_class( 77726 )<< 27 )+(mpz_class( 107 )<< 44 );
			break;
		case 183 : r= 
				mpz_class( 113478990 )+(mpz_class( 77513 )<< 27 )+(mpz_class( 106 )<< 44 );
			break;
		case 184 : r= 
				mpz_class( 113788620 )+(mpz_class( 77301 )<< 27 )+(mpz_class( 104 )<< 44 );
			break;
		case 185 : r= 
				mpz_class( 114097408 )+(mpz_class( 77093 )<< 27 )+(mpz_class( 104 )<< 44 );
			break;
		case 186 : r= 
				mpz_class( 114405365 )+(mpz_class( 76884 )<< 27 )+(mpz_class( 102 )<< 44 );
			break;
		case 187 : r= 
				mpz_class( 114712494 )+(mpz_class( 76679 )<< 27 )+(mpz_class( 102 )<< 44 );
			break;
		case 188 : r= 
				mpz_class( 115018802 )+(mpz_class( 76475 )<< 27 )+(mpz_class( 101 )<< 44 );
			break;
		case 189 : r= 
				mpz_class( 115324298 )+(mpz_class( 76272 )<< 27 )+(mpz_class( 100 )<< 44 );
			break;
		case 190 : r= 
				mpz_class( 115628985 )+(mpz_class( 76072 )<< 27 )+(mpz_class( 100 )<< 44 );
			break;
		case 191 : r= 
				mpz_class( 115932874 )+(mpz_class( 75871 )<< 27 )+(mpz_class( 98 )<< 44 );
			break;
		case 192 : r= 
				mpz_class( 116235966 )+(mpz_class( 75675 )<< 27 )+(mpz_class( 99 )<< 44 );
			break;
		case 193 : r= 
				mpz_class( 116538270 )+(mpz_class( 75478 )<< 27 )+(mpz_class( 97 )<< 44 );
			break;
		case 194 : r= 
				mpz_class( 116839793 )+(mpz_class( 75283 )<< 27 )+(mpz_class( 96 )<< 44 );
			break;
		case 195 : r= 
				mpz_class( 117140540 )+(mpz_class( 75090 )<< 27 )+(mpz_class( 96 )<< 44 );
			break;
		case 196 : r= 
				mpz_class( 117440516 )+(mpz_class( 74898 )<< 27 )+(mpz_class( 95 )<< 44 );
			break;
		case 197 : r= 
				mpz_class( 117739729 )+(mpz_class( 74707 )<< 27 )+(mpz_class( 94 )<< 44 );
			break;
		case 198 : r= 
				mpz_class( 118038182 )+(mpz_class( 74519 )<< 27 )+(mpz_class( 94 )<< 44 );
			break;
		case 199 : r= 
				mpz_class( 118335883 )+(mpz_class( 74331 )<< 27 )+(mpz_class( 93 )<< 44 );
			break;
		case 200 : r= 
				mpz_class( 118632836 )+(mpz_class( 74146 )<< 27 )+(mpz_class( 93 )<< 44 );
			break;
		case 201 : r= 
				mpz_class( 118929048 )+(mpz_class( 73961 )<< 27 )+(mpz_class( 92 )<< 44 );
			break;
		case 202 : r= 
				mpz_class( 119224526 )+(mpz_class( 73776 )<< 27 )+(mpz_class( 90 )<< 44 );
			break;
		case 203 : r= 
				mpz_class( 119519271 )+(mpz_class( 73595 )<< 27 )+(mpz_class( 90 )<< 44 );
			break;
		case 204 : r= 
				mpz_class( 119813292 )+(mpz_class( 73414 )<< 27 )+(mpz_class( 89 )<< 44 );
			break;
		case 205 : r= 
				mpz_class( 120106592 )+(mpz_class( 73235 )<< 27 )+(mpz_class( 88 )<< 44 );
			break;
		case 206 : r= 
				mpz_class( 120399179 )+(mpz_class( 73058 )<< 27 )+(mpz_class( 89 )<< 44 );
			break;
		case 207 : r= 
				mpz_class( 120691057 )+(mpz_class( 72880 )<< 27 )+(mpz_class( 87 )<< 44 );
			break;
		case 208 : r= 
				mpz_class( 120982230 )+(mpz_class( 72705 )<< 27 )+(mpz_class( 87 )<< 44 );
			break;
		case 209 : r= 
				mpz_class( 121272703 )+(mpz_class( 72531 )<< 27 )+(mpz_class( 86 )<< 44 );
			break;
		case 210 : r= 
				mpz_class( 121562482 )+(mpz_class( 72359 )<< 27 )+(mpz_class( 86 )<< 44 );
			break;
		case 211 : r= 
				mpz_class( 121851573 )+(mpz_class( 72187 )<< 27 )+(mpz_class( 85 )<< 44 );
			break;
		case 212 : r= 
				mpz_class( 122139980 )+(mpz_class( 72016 )<< 27 )+(mpz_class( 84 )<< 44 );
			break;
		case 213 : r= 
				mpz_class( 122427707 )+(mpz_class( 71848 )<< 27 )+(mpz_class( 85 )<< 44 );
			break;
		case 214 : r= 
				mpz_class( 122714759 )+(mpz_class( 71679 )<< 27 )+(mpz_class( 83 )<< 44 );
			break;
		case 215 : r= 
				mpz_class( 123001143 )+(mpz_class( 71512 )<< 27 )+(mpz_class( 83 )<< 44 );
			break;
		case 216 : r= 
				mpz_class( 123286860 )+(mpz_class( 71346 )<< 27 )+(mpz_class( 82 )<< 44 );
			break;
		case 217 : r= 
				mpz_class( 123571917 )+(mpz_class( 71181 )<< 27 )+(mpz_class( 81 )<< 44 );
			break;
		case 218 : r= 
				mpz_class( 123856317 )+(mpz_class( 71019 )<< 27 )+(mpz_class( 82 )<< 44 );
			break;
		case 219 : r= 
				mpz_class( 124140067 )+(mpz_class( 70856 )<< 27 )+(mpz_class( 81 )<< 44 );
			break;
		case 220 : r= 
				mpz_class( 124423168 )+(mpz_class( 70694 )<< 27 )+(mpz_class( 79 )<< 44 );
			break;
		case 221 : r= 
				mpz_class( 124705628 )+(mpz_class( 70534 )<< 27 )+(mpz_class( 79 )<< 44 );
			break;
		case 222 : r= 
				mpz_class( 124987448 )+(mpz_class( 70376 )<< 27 )+(mpz_class( 79 )<< 44 );
			break;
		case 223 : r= 
				mpz_class( 125268636 )+(mpz_class( 70217 )<< 27 )+(mpz_class( 78 )<< 44 );
			break;
		case 224 : r= 
				mpz_class( 125549192 )+(mpz_class( 70061 )<< 27 )+(mpz_class( 78 )<< 44 );
			break;
		case 225 : r= 
				mpz_class( 125829123 )+(mpz_class( 69906 )<< 27 )+(mpz_class( 78 )<< 44 );
			break;
		case 226 : r= 
				mpz_class( 126108435 )+(mpz_class( 69750 )<< 27 )+(mpz_class( 77 )<< 44 );
			break;
		case 227 : r= 
				mpz_class( 126387127 )+(mpz_class( 69597 )<< 27 )+(mpz_class( 77 )<< 44 );
			break;
		case 228 : r= 
				mpz_class( 126665206 )+(mpz_class( 69444 )<< 27 )+(mpz_class( 76 )<< 44 );
			break;
		case 229 : r= 
				mpz_class( 126942678 )+(mpz_class( 69292 )<< 27 )+(mpz_class( 76 )<< 44 );
			break;
		case 230 : r= 
				mpz_class( 127219543 )+(mpz_class( 69141 )<< 27 )+(mpz_class( 75 )<< 44 );
			break;
		case 231 : r= 
				mpz_class( 127495807 )+(mpz_class( 68991 )<< 27 )+(mpz_class( 74 )<< 44 );
			break;
		case 232 : r= 
				mpz_class( 127771475 )+(mpz_class( 68842 )<< 27 )+(mpz_class( 74 )<< 44 );
			break;
		case 233 : r= 
				mpz_class( 128046548 )+(mpz_class( 68694 )<< 27 )+(mpz_class( 73 )<< 44 );
			break;
		case 234 : r= 
				mpz_class( 128321033 )+(mpz_class( 68546 )<< 27 )+(mpz_class( 72 )<< 44 );
			break;
		case 235 : r= 
				mpz_class( 128594929 )+(mpz_class( 68402 )<< 27 )+(mpz_class( 73 )<< 44 );
			break;
		case 236 : r= 
				mpz_class( 128868246 )+(mpz_class( 68256 )<< 27 )+(mpz_class( 72 )<< 44 );
			break;
		case 237 : r= 
				mpz_class( 129140982 )+(mpz_class( 68113 )<< 27 )+(mpz_class( 72 )<< 44 );
			break;
		case 238 : r= 
				mpz_class( 129413145 )+(mpz_class( 67969 )<< 27 )+(mpz_class( 71 )<< 44 );
			break;
		case 239 : r= 
				mpz_class( 129684736 )+(mpz_class( 67827 )<< 27 )+(mpz_class( 71 )<< 44 );
			break;
		case 240 : r= 
				mpz_class( 129955760 )+(mpz_class( 67686 )<< 27 )+(mpz_class( 71 )<< 44 );
			break;
		case 241 : r= 
				mpz_class( 130226220 )+(mpz_class( 67544 )<< 27 )+(mpz_class( 69 )<< 44 );
			break;
		case 242 : r= 
				mpz_class( 130496118 )+(mpz_class( 67406 )<< 27 )+(mpz_class( 70 )<< 44 );
			break;
		case 243 : r= 
				mpz_class( 130765462 )+(mpz_class( 67265 )<< 27 )+(mpz_class( 68 )<< 44 );
			break;
		case 244 : r= 
				mpz_class( 131034249 )+(mpz_class( 67129 )<< 27 )+(mpz_class( 69 )<< 44 );
			break;
		case 245 : r= 
				mpz_class( 131302488 )+(mpz_class( 66991 )<< 27 )+(mpz_class( 68 )<< 44 );
			break;
		case 246 : r= 
				mpz_class( 131570179 )+(mpz_class( 66855 )<< 27 )+(mpz_class( 68 )<< 44 );
			break;
		case 247 : r= 
				mpz_class( 131837327 )+(mpz_class( 66719 )<< 27 )+(mpz_class( 67 )<< 44 );
			break;
		case 248 : r= 
				mpz_class( 132103934 )+(mpz_class( 66585 )<< 27 )+(mpz_class( 67 )<< 44 );
			break;
		case 249 : r= 
				mpz_class( 132370006 )+(mpz_class( 66450 )<< 27 )+(mpz_class( 66 )<< 44 );
			break;
		case 250 : r= 
				mpz_class( 132635542 )+(mpz_class( 66318 )<< 27 )+(mpz_class( 66 )<< 44 );
			break;
		case 251 : r= 
				mpz_class( 132900548 )+(mpz_class( 66186 )<< 27 )+(mpz_class( 66 )<< 44 );
			break;
		case 252 : r= 
				mpz_class( 133165027 )+(mpz_class( 66054 )<< 27 )+(mpz_class( 65 )<< 44 );
			break;
		case 253 : r= 
				mpz_class( 133428983 )+(mpz_class( 65923 )<< 27 )+(mpz_class( 65 )<< 44 );
			break;
		case 254 : r= 
				mpz_class( 133692416 )+(mpz_class( 65793 )<< 27 )+(mpz_class( 64 )<< 44 );
			break;
		case 255 : r= 
				mpz_class( 133955332 )+(mpz_class( 65664 )<< 27 )+(mpz_class( 64 )<< 44 );
			break; 
 
 
 
		}
		return r;
	}


}
