#ifndef __AP_FLOAT_H__
#define __AP_FLOAT_H__

#include <cstdint>
#include <iostream>

class ap_float_base
{
};

template<int e_width, int m_width>
class ap_float : public ap_float_base
{
public:
    ap_float() : ap_float(0.0f) {}

    ap_float(const float& v) : _internal(quantize(v))
    {}

    ap_float& operator=(const float& v)
    {
        _internal = quantize(v);
        return *this;
    }

    ap_float& operator+=(const float& rhs)
    {
        _internal = quantize(_internal + quantize(rhs));
        return *this;
    }

    ap_float& operator-=(const float& rhs)
    {
        _internal = quantize(_internal - quantize(rhs));
        return *this;
    }

    ap_float& operator*=(const float& rhs)
    {
        _internal = quantize(_internal * quantize(rhs));
        return *this;
    }

    ap_float& operator/=(const float& rhs)
    {
        _internal = quantize(_internal / quantize(rhs));
        return *this;
    }

    operator float() const { return float(_internal); }
private:
    float _internal;

    static float quantize(const float& v)
    {
		const uint32_t qm_mask = (1ULL << (31)) | (((1ULL << 23) - 1) & ~((1ULL << (23 - m_width)) - 1));
		const uint32_t e_mask = (1ULL << 8) - 1;
		const uint32_t qe_max = 127 + (1ULL << (e_width - 1));
		const uint32_t qe_min = 127 - (1ULL << (e_width - 1)) + 1;

		uint32_t qm = *((uint32_t *)&v) & qm_mask;
		uint32_t e = *((uint32_t *)&v) >> 23 & e_mask;
		uint32_t qe = e == 0 ? 0 : (e <= qe_min ? qe_min : (e >= qe_max ? qe_max : e));
		uint32_t qout = qm | (qe << 23);

		return *((float *)&qout);
    }
};

template<int e_width, int m_width>
std::ostream& operator<<(std::ostream& os, const ap_float<e_width, m_width>& obj)
{
    os << float(obj);
    return os;
}

template<int e_width, int m_width>
std::istream& operator>>(std::istream& is, ap_float<e_width, m_width>& obj)
{
    float in;
    is >> in;
    obj = in;
    return is;
}

#endif