#ifndef _FORMAL_BINARY_PRODUCT
#define _FORMAL_BINARY_PRODUCT

//#include "Operator.hpp"
//#include "utils.hpp"
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <stddef.h>
#include <gmpxx.h>

#if defined (__GNUC__) && (__GNUC_MAJOR__ > 3 || (__GNUC_MAJOR__ == 3 && __GNUC_MINOR__ >= 1))
#define DEPRECATED __attribute__((deprecated))
#else
#define DEPRECATED
#endif

namespace flopoco {

/* some bit of context here:
 * there's b, a polynomial which is written b_1*1b-1 + b_2*1b-2 + ... +
 * b_n * 1b-n
 * and we try to calculate a power p of b, by evaluating the full
 * formula the simplifying it (see Jérémie Detrey's thesis pp. 40-41)
 */

/* actually MonomialOfBits will be used with a unique size, and using
 * different sizes together won't work
 * if we knew this size at compile time it would be better as a template
 * argument */
// a MonomialOfBits is a product of b_i
class MonomialOfBits {
	public:
		MonomialOfBits (size_t n): data(std::vector<bool>(n)), n(n) {
		}
		MonomialOfBits operator* (const MonomialOfBits& rhs) const;
		bool operator== (const MonomialOfBits& rhs) const {
			return (this->n == rhs.n) && (this->data == rhs.data);
		}
		bool operator< (const MonomialOfBits& rhs) const;

		std::vector<bool> data;
		size_t n;
};

class ProductBit;
class ProductBitIR;
class Product;
class ProductIR;
struct ProductIRQuoRem;
// a ProductBit is for a sum_i (product_j b_(j_i)) where a mapping i->j_i
// appears at most one time in the sum [not enforced in representation]
class ProductBit {
	public:
		ProductBit (size_t mon_size)
			:data (std::list<MonomialOfBits>())
			,mon_size (mon_size) {
		}
		ProductBit (const ProductBitIR& rhs);

		std::list<MonomialOfBits> data;
		size_t mon_size;
};

// and a ProductBitIR is a sum_i (alpha_i (product_j b_(j_i)))
// note the alpha_i, which can be eliminated by ProductIR::simplify()
class ProductBitIR {
	public:
		ProductBitIR (const ProductBit& rhs);
		ProductBitIR (size_t mon_size)
			:data (std::map<MonomialOfBits,int>())
			,mon_size (mon_size) {
		}
		int getTimes (const MonomialOfBits& e) const;
		void addToCoeff (const MonomialOfBits& e, int coeff);
		ProductBitIR& operator+= (const ProductBitIR& rhs);
		ProductBitIR operator* (const ProductBitIR& rhs) const;

		std::map<MonomialOfBits, int> data;
		size_t mon_size;
};

// a ProductIR is a sum of (sum_i alpha_i (product b_(j_i))) 1b-k
// note the supplemental alpha_i's compared to Product,
// all of those which can be eliminated by simplify()
/* data is represented by a *little-endian* vector in the 2 following classes.
   why little ? because within simplify if we have a carry
   on the msb the vector has to grow on the msb side, which
   consequently has to be on the right as std::vectors only
   grow to the right (std::vector<T>::push_back but not push_front) */
class ProductIR {
	public:
		ProductIR (size_t w, int msb, size_t mon_size) 
			:data (std::vector<ProductBitIR>
					(w, ProductBitIR(mon_size))
			      )
			,msb (msb),mon_size (mon_size) {
		}
		ProductIR (std::vector<ProductBitIR> data,
				int msb, size_t mon_size)
			:data (data), msb (msb), mon_size (mon_size) {
		}
		ProductIR (const Product& rhs);
		// returns the polynomial x_0*1b-1 + x_1*1b-2 + ...
		static ProductIR identity (size_t mon_size);
		ProductIR& operator+= (const ProductIR& rhs) DEPRECATED;
		ProductIR& operator>>= (int n) DEPRECATED;
		ProductIR& operator*= (const ProductBitIR& rhs) DEPRECATED;
		ProductIR operator* (const ProductIR& rhs) const;
		ProductIR toPow (size_t n) const;
		ProductIR& simplifyInPlace (void); // returns *this
		ProductIR& setMSB (int newMSB); // returns *this
		ProductIRQuoRem div (int divisor);
		ProductIR truncate (mpz_class ulps) const;

		std::vector<ProductBitIR> data;
		int msb;
		size_t mon_size;
};

// to return the result of euclidean division
struct ProductIRQuoRem {
	ProductIR quo;
	ProductIR rem;
};

// a Product is a sum of (sum_i (product_j b_(j_i))) 1b-k
class Product {
	public:
		Product (const ProductIR& rhs);

		std::vector<ProductBit> data;
		int msb;
		size_t mon_size;
};

}

std::ostream& operator<< (std::ostream& o, const flopoco::MonomialOfBits& m);
std::ostream& operator<< (std::ostream& o, const flopoco::ProductBitIR& pbi);
std::ostream& operator<< (std::ostream& o, const flopoco::ProductIR& pi);

#endif //include guard

