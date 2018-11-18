/* Copyright 2014-2018 Rsyn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef RSYN_DBU_H
#define RSYN_DBU_H

#include <cstdint>
#include <cmath>

#include <algorithm>
using std::max;
using std::min;
#include <ostream>
using std::ostream;

#include "dim.h"

// -----------------------------------------------------------------------------

typedef std::int64_t DBU;
//typedef float DBU;
typedef double FloatingPointDBU;

// -----------------------------------------------------------------------------

#define MAKE_SELF_OPERATOR( OP ) \
	inline void operator OP ( const DBUxy right ) { x OP right.x; y OP right.y; }

#define MAKE_OPERATOR( OP ) \
	inline DBUxy operator OP ( const DBUxy v0, const DBUxy v1 ) { \
		return DBUxy( v0.x OP v1.x, v0.y OP v1.y ); \
	}

#define MAKE_FUNCTION_1( FUNC ) \
	inline DBUxy FUNC ( const DBUxy v ) { \
		return DBUxy( FUNC (v.x), FUNC (v.y) ); \
	}

#define MAKE_FUNCTION_2( FUNC ) \
	inline DBUxy FUNC ( const DBUxy v0, const DBUxy v1 ) { \
		return DBUxy( FUNC (v0.x, v1.x), FUNC (v0.y, v1.y) ); \
	}

struct DBUxy {
	friend ostream &operator<<( ostream &out, const DBUxy &v ) {
		return out << "(" << v.x << ", " << v.y << ")";
	} // end function

	union {
		struct { DBU x, y; };
		DBU xy[2];
	};

	// Construtctors.
	DBUxy(): x(0), y(0)  {}
	DBUxy( const DBU x, const DBU y ) : x(x), y(y) {}

	explicit DBUxy( const DBU scalar ) : x(scalar), y(scalar) {} // explicit to avoid accidentally assigning to scalar

	// Operators.
	MAKE_SELF_OPERATOR( += );
	MAKE_SELF_OPERATOR( -= );
	MAKE_SELF_OPERATOR( *= );
	MAKE_SELF_OPERATOR( /= );

	MAKE_SELF_OPERATOR( = );

	inline const bool operator == ( const DBUxy v ) const {
		return x == v.x && y == v.y;
	} // end method
	
	inline const bool operator != ( const DBUxy v ) const {
		return x != v.x || y != v.y;
	} // end method

	inline void operator *= ( const FloatingPointDBU scalar) {
		x = (DBU) (x * scalar);
		y = (DBU) (y * scalar);
	} // end method

	inline void operator /= ( const FloatingPointDBU scalar) {
		x = (DBU) (x / scalar);
		y = (DBU) (y / scalar);
	} // end method
	
	      DBU &operator[](const int dimension)       { return xy[dimension]; }
	const DBU &operator[](const int dimension) const { return xy[dimension]; }

	// Methods.
	FloatingPointDBU norm() const {
		return std::sqrt( x*x + y*y );
	} // end method

	DBUxy normalized() const {
		const FloatingPointDBU v = norm();
		return DBUxy( (DBU) (x/v), (DBU) (y/v) );
	} // end method

	DBUxy safeNormalized() const {
		const FloatingPointDBU v = norm();
		if ( v == 0.0 )
			return DBUxy(0,0);
		else
			return DBUxy( (DBU) (x/v), (DBU) (y/v) );
	} // end method

	void apply(const DBU scalar) {
		x = scalar;
		y = scalar;
	} // end method

	void set(const DBU x, const DBU y ) {
		this->x = x;
		this->y = y;
	} // end method

	void scale(const FloatingPointDBU xScaling, const FloatingPointDBU yScaling){
		x = (DBU) (x*xScaling);
		y = (DBU) (y*yScaling);
	} // end method

	void scale(const FloatingPointDBU scaling){
		x = (DBU) (x*scaling);
		y = (DBU) (y*scaling);
	} // end method

	DBU aggregated() const {
		return x + y;
	} // end method

	void abs() {
		x = std::abs(x);
		y = std::abs(y);
	} // end method

	void clear () {
		x = 0;
		y = 0;
	} // end method

	static DBU computeManhattanDistance(const DBUxy p0, const DBUxy p1) {
		return std::abs(p0.x - p1.x) + std::abs(p0.y - p1.y);
	} // end method

}; // end struct

MAKE_OPERATOR( + );
MAKE_OPERATOR( - );
MAKE_OPERATOR( * );
MAKE_OPERATOR( / );

// Guilherme Flach - 2017/03/18
// These template functions were added to resolve ambiguity related to trying to
// cast int to DBU (aka std::int64_t) or FloatingPointDBU (aka double).

template<typename T>
DBUxy operator*( const T scalar, const DBUxy v ) {
	static_assert(std::is_arithmetic<T>::value,
			"Arithmetic value required.");
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
			"Integer or floating point required.");

	// Cast must be performed after the multiplication to avoid wrong results
	// when the scaling factor is in the (0, 1) range.
	return DBUxy( (DBU) (v.x * scalar), (DBU) (v.y * scalar) );
} // end method

template<typename T>
DBUxy operator*( const DBUxy v, const T scalar ) {
	return scalar*v;
} // end method

template<typename T>
DBUxy operator/( const T scalar, const DBUxy v ) {
	static_assert(std::is_arithmetic<T>::value, 
			"Arithmetic value required.");
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
			"Integer or floating point required.");

	// Cast must be performed after the division to avoid wrong results
	// when the scaling factor is in the (0, 1) range.
	return DBUxy( (DBU) (v.x / scalar), (DBU) (v.y / scalar) );
} // end method

template<typename T>
DBUxy operator/( const DBUxy v, const T scalar ) {
	return scalar/v;
} // end method

MAKE_FUNCTION_2(max);
MAKE_FUNCTION_2(min);

// Unary operators.
inline DBUxy operator-( const DBUxy value ) {
	return DBUxy( -value.x, -value.y );
} // end operator

inline DBUxy operator+( const DBUxy value ) {
	return DBUxy( +value.x, +value.y );
} // end operator

// -----------------------------------------------------------------------------
// TODO: Move to a better place...

template<typename T> inline T
roundedUpIntegralDivision(const T numerator, const T denominator) {
	static_assert(std::is_integral<T>::value, "Integer required.");
    return numerator / denominator + (numerator % denominator? 1 : 0);
} // end function

// -----------------------------------------------------------------------------

#undef MAKE_OPERATOR
#undef MAKE_OPERATOR_SCALAR
#undef MAKE_SELF_OPERATOR
#undef MAKE_SELF_OPERATOR_SCALAR
#undef MAKE_FUNCTION_1
#undef MAKE_FUNCTION_2

#endif

