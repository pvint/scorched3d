////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2013
//
//    This file is part of Scorched3D.
//
//    Scorched3D is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Scorched3D is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
////////////////////////////////////////////////////////////////////////////////

#include <common/FixedVector.h>

static FixedVector nullVector;
static FixedVector minVector = FixedVector(fixed::MIN_FIXED, fixed::MIN_FIXED, fixed::MIN_FIXED);
static FixedVector maxVector = FixedVector(fixed::MAX_FIXED, fixed::MAX_FIXED, fixed::MAX_FIXED);

std::string FixedVector::asQuickString()
{
	std::string v1 = V[0].asQuickString();
	std::string v2 = V[0].asQuickString();
	std::string v3 = V[0].asQuickString();
	return S3D::formatStringBuffer("%s,%s,%s", v1.c_str(), v2.c_str(), v3.c_str());
}

FixedVector &FixedVector::getNullVector()
{
	return nullVector;
}

FixedVector &FixedVector::getMaxVector()
{
	return maxVector;
}

FixedVector &FixedVector::getMinVector()
{
	return minVector;	
}

#define sqrt_step(shift) \
    if((0x40000000l >> shift) + root <= value)          \
    {                                                   \
        value -= (0x40000000l >> shift) + root;         \
        root = (root >> 1) | (0x40000000l >> shift);    \
    }                                                   \
    else                                                \
    {                                                   \
        root = root >> 1;                               \
    }

static int64_t iSqrtLL(int64_t value)
{
    int64_t root = 0;

    sqrt_step( 0);
    sqrt_step( 2);
    sqrt_step( 4);
    sqrt_step( 6);
    sqrt_step( 8);
    sqrt_step(10);
    sqrt_step(12);
    sqrt_step(14);
    sqrt_step(16);
    sqrt_step(18);
    sqrt_step(20);
    sqrt_step(22);
    sqrt_step(24);
    sqrt_step(26);
    sqrt_step(28);
    sqrt_step(30);

    // round to the nearest integer, cuts max error in half

    if(root < value)
    {
        ++root;
    }

    return root;
}

fixed FixedVector::Magnitude()
{
	int64_t a = V[0].getInternalData()*V[0].getInternalData()/fixed::FIXED_RESOLUTION;
	int64_t b = V[1].getInternalData()*V[1].getInternalData()/fixed::FIXED_RESOLUTION;
	int64_t c = V[2].getInternalData()*V[2].getInternalData()/fixed::FIXED_RESOLUTION;
	int64_t res = a + b + c;
	int64_t result = iSqrtLL(res);
	result *= int64_t(100);

	return fixed(true, result);
}
