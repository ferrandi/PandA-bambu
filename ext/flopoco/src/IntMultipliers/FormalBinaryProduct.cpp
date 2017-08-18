#include "FormalBinaryProduct.hpp"
#include <iostream>

using namespace flopoco;
using std::cout;
using std::endl;

MonomialOfBits MonomialOfBits::operator* (const MonomialOfBits& rhs) const
{
	if (this->n != rhs.n)
		throw "sizes don't match (MonomialOfBits::operator+)";
	MonomialOfBits res (*this);
	std::vector<bool>::iterator i = res.data.begin();
	std::vector<bool>::const_iterator j = rhs.data.begin();
	for (;j != rhs.data.end (); i++,j++) {
		*i = *i || *j;
	}
	return res;
}
bool MonomialOfBits::operator< (const MonomialOfBits& rhs) const
{
	if (this->n != rhs.n)
		throw "sizes don't match (MonomialOfBits::operator<)";
	std::vector<bool>::const_iterator
		i = this->data.begin(), j = rhs.data.begin();
	for (; i != this->data.end(); i++, j++) {
		if (*i == *j) continue;
		return (*i < *j);
	}
	// these are equal
	return false;
}
std::ostream& operator<< (std::ostream& o, const MonomialOfBits& m)
{
	bool cont = false;
	size_t i;
	for (i = 0; i < m.data.size(); i++) {
		if (m.data[i]) {
			if (cont)
				o << '*';
			o << "x_" << i;
			cont = true;
		}
	}
	// and if there is no x_i at all...
	if (!cont) {
		o << '1';
	}
	return o;
}

// the ProductBitIR must have a 0 or 1 coeff (see also the throw)
ProductBit::ProductBit (const ProductBitIR& rhs)
	:data (std::list<MonomialOfBits>())
	,mon_size (rhs.mon_size)
{
	std::map<MonomialOfBits, int>::const_iterator it = rhs.data.begin();
	for (; it != rhs.data.end(); it++) {
		switch (it->second) {
			case 0:
				break;
			case 1:
				data.push_back (it->first);
				break;
			default:
				throw "The data must be from a ProductIR"
				      "on which simplify() has been called\n";
		}
	}
}

ProductBitIR::ProductBitIR (const ProductBit& rhs)
	:data (std::map<MonomialOfBits,int>())
	,mon_size (rhs.mon_size)
{
	std::list<MonomialOfBits>::const_iterator it
		= rhs.data.begin();
	for (; it != rhs.data.end(); it++) {
		addToCoeff (*it, 1);
	}
}
int ProductBitIR::getTimes (const MonomialOfBits& e) const
{
	if (e.n != mon_size)
		throw "Wrong monomial size (ProductBitIR::addToCoeff)\n";
	std::map<MonomialOfBits, int>::const_iterator it
		= data.find (e);
	if (it == data.end()) {
		return 0;
	}
	return it->second;
}
void ProductBitIR::addToCoeff (const MonomialOfBits& e, int coeff)
{
	if (e.n != mon_size)
		throw "Wrong monomial size (ProductBitIR::addToCoeff)\n";
	std::map<MonomialOfBits, int>::iterator it = data.find (e);
	if (it == data.end()) {
		data[e] = coeff;
	} else {
		it->second += coeff;
	}
}
ProductBitIR& ProductBitIR::operator+= (const ProductBitIR& rhs)
{
	std::map<MonomialOfBits, int>::const_iterator it
		= rhs.data.begin();
	for (; it != rhs.data.end(); it++) {
		this->addToCoeff (it->first, it->second);
	}
	return *this;
}
ProductBitIR ProductBitIR::operator* (const ProductBitIR& rhs) const
{
	if (this->mon_size != rhs.mon_size)
		throw "operands mon_size don't match (ProductBitIR::operator*)\n";
	ProductBitIR res (this->mon_size);
	std::map<MonomialOfBits, int>::const_iterator i
		= this->data.begin();
	std::map<MonomialOfBits, int>::const_iterator j;
	for (; i != this->data.end(); i++) {
		j = rhs.data.begin();
		for (; j != rhs.data.end(); j++) {
			MonomialOfBits prod = i->first * j->first;
			res.addToCoeff (prod, i->second * j->second);
		}
	}
	return res;
}
std::ostream& operator<< (std::ostream& o, const ProductBitIR& pbi)
{
	bool cont = false;
	std::map<MonomialOfBits, int>::const_iterator it =
		pbi.data.begin();
	for (; it != pbi.data.end(); it++) {
		if (it->second) {
			if (cont)
				o << " + ";
			// can print "+ -" in some cases
			if (it->second != 1) {
				o << it->second;
				o << ' ';
			}
			o << it->first;
			cont = true;
		}
	}
	// and if there's no term...
	if (!cont)
		o << '0';
	return o;
}
ProductIR::ProductIR (const Product& rhs)
	:data(std::vector<ProductBitIR>
			(rhs.data.size(),ProductBitIR(rhs.mon_size))
	     )
	,msb(rhs.msb)
	,mon_size (rhs.mon_size)
{
	std::vector<ProductBit>::const_iterator i = rhs.data.begin();
	std::vector<ProductBitIR>::iterator j = data.begin();
	for (; i != rhs.data.end(); i++, j++) {
		*j = ProductBitIR (*i);
	}
}
//src range must include dst range
ProductIR& ProductIR::operator+= (const ProductIR& rhs)
{
	if (this->msb < rhs.msb)
		throw "msb mismatch (ProductIR::operator+=)";
	if (this->msb - this->data.size() > rhs.msb - rhs.data.size())
		throw "lsb mismatch (ProductIR::operator+=)";
	if (this->mon_size != rhs.mon_size)
		throw "mon_size mismatch (ProductIR::operator+=)";
	// we use reverse iterators because we know MSBs more readily
	// than LSBs
	std::vector<ProductBitIR>::const_reverse_iterator j
		= rhs.data.rbegin();
	std::vector<ProductBitIR>::reverse_iterator i = this->data.rbegin();
	//align the 2 iterators
	i += (this->msb - rhs.msb);
	for (; j != rhs.data.rend(); i++,j++) {
		*i += *j;
	}
	return *this;
}
ProductIR& ProductIR::operator>>= (int n)
{
	if (n < 0)
		throw "invalid argument (ProductIR::operator>>=)";
	if (n == 0)
		return *this;
	// arithmetically it is a >>=, so since we use an el vector
	// we have to do a <<= in memory
	std::vector<ProductBitIR>::iterator i = this->data.begin();
	std::vector<ProductBitIR>::iterator j = i + n;
	for (; j != this->data.end(); i++,j++) {
		*i = *j;
	}
	for (; i != this->data.end(); i++) {
		*i = ProductBitIR(this->mon_size);
	}
	return *this;
}
ProductIR& ProductIR::operator*= (const ProductBitIR& rhs)
{
	if (this->mon_size != rhs.mon_size)
		throw "mon_size mismatch (ProductIR::operator*=)";
	std::vector<ProductBitIR>::iterator i = this->data.begin();
	for (; i != this->data.end(); i++) {
		//no ProductBitIR::operator*=
		*i = *i * rhs;
	}
	return *this;
}
std::ostream& operator<< (std::ostream& o, const ProductIR& pi)
{
	int i = pi.msb;
	// we begin by msb so we have to use a reverse iterator
	std::vector<ProductBitIR>::const_reverse_iterator it
		= pi.data.rbegin();
	bool cont = false;
	for (; it != pi.data.rend(); it++, i--) {
		if (cont)
			o << " + ";
		o << '(' << (*it) << ") * 1b" << i;
		cont = true;
	}
	return o;
}
ProductIR ProductIR::operator* (const ProductIR& rhs) const
{
	if (this->mon_size != rhs.mon_size)
		throw "mon_size mismatch (ProductIR::operator*)";
	size_t n = this->data.size(), m = rhs.data.size();
	ProductIR res (n+m-1, this->msb + rhs.msb, this->mon_size);
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < m; j++) {
			// since product has exactly the right size
			// it doesn't matter if we make the product
			// in counter-endian mode
			res.data[i+j] += (this->data[i] * rhs.data[j]);
		}
	}
	return res;
}
ProductIR ProductIR::toPow (size_t n) const
{
	if (n <= 0)
		throw "wrong argument (ProductIR::toPow)";
	ProductIR res = *this;;
	for (unsigned i = 0; i < n-1; i++) {
		res = res * *this;
	}
	return res;
}


ProductIR& ProductIR::simplifyInPlace (void)
{
	// we begin by lsb: non-rev iterator
	std::vector<ProductBitIR>::iterator i = data.begin(), i_tmp;
	int coeff; // to avoid messing beetween goto and declarations
	for (; i != data.end(); i++) {
		std::map<MonomialOfBits, int>::iterator j = i->data.begin();
		for (; j != i->data.end(); j++) {
			loop_begin:
			coeff = j->second;
			if (coeff < 0) {
				throw "negative coefficient, shouldn't"
				      "happen (ProductIR::simplify)";
			}
			if (coeff > 1) {
				i_tmp = i+1;
				if (i_tmp == data.end()) {
					msb++;
					data.push_back (ProductBitIR(mon_size));
					// now i and i_tmp are both invalid
					// so regenerate them
					i_tmp = data.end() - 1;
					i = i_tmp - 1;
					// apparently j is also destroyed
					j = i->data.begin();
					// not sure continue; does what I want
					if (j == i->data.end())
						break;
					else
						goto loop_begin;
				}
				i_tmp->addToCoeff (j->first, coeff >> 1);
				j->second = coeff & 0x1;
			}
		}
	}
	return *this;
}


ProductIR& ProductIR::setMSB (int newMSB)
{
	if (newMSB <= msb) {
		for (size_t i = 0; i < (msb - newMSB); i++) {
			data.pop_back ();
		}
		return *this;
	}
	for (size_t i = 0; i < (newMSB - msb); i++) {
		data.push_back (ProductBitIR (mon_size));
	}
	msb = newMSB;
	return *this;
}
// won't work well if simplify() isn't called before
// quo and/or rem can be NULL
ProductIRQuoRem ProductIR::div (int divisor)
{
	// ensure right dims for quo/rem
	ProductIRQuoRem res = {ProductIR (data.size(), msb, mon_size),
	                       ProductIR (data.size(), msb, mon_size)};
	// first gather the actually used coeffs to avoid
	// exponential theta(2^Monomial.size) complexity
	ProductBitIR used_coeffs (mon_size);
	std::vector<ProductBitIR>::const_iterator i;
	// assume coeffs are >=0
	for (i = data.begin(); i != data.end(); i++) {
		used_coeffs += *i;
	}
	std::map<MonomialOfBits, int>::const_iterator j;
	for (j = used_coeffs.data.begin(); j != used_coeffs.data.end(); j++) {
		// if the coeff is 0, don't care of the associated monomial
		if (j->second == 0)
			continue;
		MonomialOfBits m = j->first;
		// make a mpz_class of coeffs of m
		mpz_class coeffs_of_m(0);
		// old G++ compat: const_ needs to be removed for 3.4.6
		std::vector<ProductBitIR>::const_reverse_iterator it;
		for (it = data.rbegin(); it != data.rend(); it++) {
			coeffs_of_m <<= 1;
			coeffs_of_m += it->getTimes (m);
		}
		// use gmp for the division
		mpz_class quoz (coeffs_of_m / divisor),
			  remz (coeffs_of_m % divisor);
		// convert back to ProductIR
		std::vector<ProductBitIR>::iterator qi = res.quo.data.begin();
		for (; qi != res.quo.data.end(); qi++) {
			// ???z & 1 = 0 or 1, so get_si() is safe
			qi->addToCoeff (m, 
				static_cast<mpz_class>(quoz & 1).get_si());
			quoz >>= 1;
		}
		std::vector<ProductBitIR>::iterator ri = res.rem.data.begin();
		for (; ri != res.rem.data.end(); ri++) {
			// ???z & 1 = 0 or 1, so get_si() is safe
			ri->addToCoeff (m,
				static_cast<mpz_class>(remz & 1).get_si());
			remz >>= 1;
		}
	}
	return res;
}
ProductIR ProductIR::truncate (mpz_class ulps) const
{
	mpz_class cur_err(0), cur_ulp(1);
	std::vector<ProductBitIR>::const_iterator it;
	for (it = data.begin(); it != data.end(); it++) {
		std::map<MonomialOfBits,int>::const_iterator i;
		for (i = it->data.begin(); i != it->data.end(); i++) {
			cur_err += (i->second * cur_ulp);
		}
		if (cur_err > ulps)
			break;
		cur_ulp <<= 1;
	}
	return ProductIR (std::vector<ProductBitIR> (it, data.end()),
	                  msb,
		          mon_size);
}
// must call ProductIR::simplify() on rhs before
Product::Product (const ProductIR& rhs)
	:data(std::vector<ProductBit>
			(rhs.data.size(), ProductBit(rhs.mon_size))
	     )
	,msb (rhs.msb)
	,mon_size (rhs.mon_size)
{
	std::vector<ProductBitIR>::const_iterator i = rhs.data.begin();
	std::vector<ProductBit>::iterator j = data.begin();
	for (; i != rhs.data.end(); i++, j++) {
		*j = ProductBit (*i);
	}
}

ProductIR ProductIR::identity (size_t n)
{
	ProductIR res (n, -1, n);
	MonomialOfBits m (n);
	for (size_t i0 = 0; i0 < n; i0++) {
		for (size_t i = 0; i < n; i++)
			m.data[i] = (i+i0==n-1)?1:0;
		std::map<MonomialOfBits, int> map;
		map[m] = 1;
		ProductBitIR pbi(n);
		pbi.data = map;
		res.data[i0] = pbi;
	}
	return res;
}

