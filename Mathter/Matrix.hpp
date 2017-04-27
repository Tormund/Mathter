#pragma once

// Remove goddamn fucking bullshit crapware winapi macros.
#if _MSC_VER && defined(min)
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#define MATHTER_MINMAX
#endif


#include <type_traits>
#include <iostream> // debug only
#include <algorithm>

#include "Vector.hpp"

namespace mathter {



//------------------------------------------------------------------------------
// Matrix base class only allocating the memory
//------------------------------------------------------------------------------

template <class T, int Rows, int Columns, eMatrixOrder Order = eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout Layout = eMatrixLayout::ROW_MAJOR, bool Packed = false>
class MatrixData {
public:
	constexpr int ColumnCount() const {
		return Columns;
	}
	constexpr int RowCount() const {
		return Rows;
	}
	constexpr int Width() const {
		return Columns;
	}
	constexpr int Height() const {
		return Rows;
	}
protected:
	// Rows equal height, Columns equal width, row-major has column-sized stripes
	static constexpr int StripeDim = Layout == eMatrixLayout::ROW_MAJOR ? Columns : Rows;
	static constexpr int StripeCount = Layout == eMatrixLayout::ROW_MAJOR ? Rows : Columns;

	Vector<T, StripeDim, Packed> stripes[StripeCount];

	// Get element
	inline T& GetElement(int row, int col) {
		return GetElementImpl(col, row, std::integral_constant<bool, Layout == eMatrixLayout::ROW_MAJOR>());
	}
	inline T GetElement(int row, int col) const {
		return GetElementImpl(col, row, std::integral_constant<bool, Layout == eMatrixLayout::ROW_MAJOR>());
	}
private:
	inline T& GetElementImpl(int col, int row, std::true_type) {
		return stripes[row][col];
	}
	inline T GetElementImpl(int col, int row, std::true_type) const {
		return stripes[row][col];
	}
	inline T& GetElementImpl(int col, int row, std::false_type) {
		return stripes[col][row];
	}
	inline T GetElementImpl(int col, int row, std::false_type) const {
		return stripes[col][row];
	}
};


//------------------------------------------------------------------------------
// Matrix operations
//------------------------------------------------------------------------------

// Empty
template <class T>
class Empty {};

template <bool Enable, class Module>
using MatrixModule = typename std::conditional<Enable, Module, Empty<Module>>::type;


// Decompositions
template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class DecompositionLU {
	using MatrixT = Matrix<T, Dim, Dim, Order, Layout, Packed>;
public:
	DecompositionLU(const MatrixT& arg) {
		arg.DecomposeLU(L, U);
		T prod = L(0, 0);
		T sum = abs(prod);
		for (int i = 1; i < Dim; ++i) {
			prod *= L(i, i);
			sum += abs(L(i, i));
		}
		sum /= Dim;
		solvable = abs(prod) / sum > T(1e-6);
	}

	bool Solve(Vector<float, Dim, Packed>& x, const Vector<T, Dim, Packed>& b);

	MatrixT L, U;
	bool solvable;
};


// Square matrices
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixSquare {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
protected:
	friend class MatrixT;
	using Inherit = Empty<MatrixSquare>;
};

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixSquare<T, Dim, Dim, Order, Layout, Packed> {
	using MatrixT = Matrix<T, Dim, Dim, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
public:
	template <class T2, eMatrixOrder Order2, eMatrixLayout Layout2>
	MatrixT& operator*=(const Matrix<T2, Dim, Dim, Order2, Layout2, Packed>& rhs) {
		self() = operator*<T, T2, Dim, Dim, Dim, Order, Order2, Layout, Layout2, Packed, T>(self(), rhs);
		return self();
	}

	T Trace() const;
	T Determinant() const;
	MatrixT& Transpose();
	MatrixT& Invert();
	MatrixT Inverse() const;

	void DecomposeLU(MatrixT& L, MatrixT& U) const;
	mathter::DecompositionLU<T, Dim, Order, Layout, Packed> DecompositionLU() const {
		return mathter::DecompositionLU<T, Dim, Order, Layout, Packed>(self());
	}
protected:
	friend class MatrixT;
	using Inherit = MatrixSquare;
};



// Rotation 2D functions
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixRotation2D {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
protected:
	static constexpr bool Enable2DRotation =
		(Columns == 3 && Rows == 3)
		|| (Columns == 2 && Rows == 3 && Order == eMatrixOrder::FOLLOW_VECTOR)
		|| (Columns == 3 && Rows == 2 && Order == eMatrixOrder::PRECEDE_VECTOR)
		|| (Columns == 2 && Rows == 2);
public:
	static MatrixT Rotation(T angle);
	MatrixT& SetRotation(T angle) { 
		*this = Rotation(angle);
		return *this;
	}
protected:
	friend class MatrixT;
	using Inherit = MatrixModule<Enable2DRotation, MatrixRotation2D>;
};



// Rotation 3D functions
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixRotation3D {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
protected:
	static constexpr bool Enable3DRotation =
		(Columns == 4 && Rows == 4)
		|| (Columns == 3 && Rows == 4 && Order == eMatrixOrder::FOLLOW_VECTOR)
		|| (Columns == 4 && Rows == 3 && Order == eMatrixOrder::PRECEDE_VECTOR)
		|| (Columns == 3 && Rows == 3);
public:
	// Static rotation
	template <int Axis>
	static MatrixT RotationAxis(T angle);

	static MatrixT RotationX(T angle);
	static MatrixT RotationY(T angle);
	static MatrixT RotationZ(T angle);

	template <int FirstAxis, int SecondAxis, int ThirdAxis>
	static MatrixT RotationAxis3(T angle1, T angle2, T angle3);

	static MatrixT RotationEuler(float z1, float x2, float z3);
	static MatrixT RotationRPY(float x1, float y2, float z3);
	template <class U, bool Vpacked>
	static MatrixT RotationAxisAngle(const Vector<U, 3, Vpacked>& axis, T angle);

	// Member rotation
	template <int Axis>
	MatrixT& SetRotationAxis(T angle) {	self() = Rotation<Axis>(angle); return self(); }

	MatrixT& SetRotationX(T angle) { self() = RotationX(angle); return self(); }
	MatrixT& SetRotationY(T angle) { self() = RotationY(angle); return self(); }
	MatrixT& SetRotationZ(T angle) { self() = RotationZ(angle); return self(); }

	template <int FirstAxis, int SecondAxis, int ThirdAxis>
	MatrixT& SetRotationAxis3(T angle1, T angle2, T angle3) { self() = Rotation<FirstAxis, SecondAxis, ThirdAxis>(angle1, angle2, angle3); return self(); }

	MatrixT& SetRotationEuler(float z1, float x2, float z3) { self() = RotationEuler(z1, x3, z3); return self(); }
	MatrixT& SetRotationRPY(float x1, float y2, float z3) { self() = RotationRPY(x1, y2, z3); return self(); }
	template <class U, bool Vpacked>
	MatrixT& SetRotationAxisAngle(const Vector<U, 3, Vpacked>& axis, T angle) { self() = Rotation(axis, angle); return self(); }
protected:
	friend class MatrixT;
	using Inherit = MatrixModule<Enable3DRotation, MatrixRotation3D>;
};



// Translation functions
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixTranslation {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
protected:
	static constexpr bool EnableTranslation =
		(Rows < Columns ? Rows : Columns) > 0 &&
		(Order == eMatrixOrder::FOLLOW_VECTOR && Rows - 1 <= Columns && Columns <= Rows)
		|| (Order == eMatrixOrder::PRECEDE_VECTOR && Columns - 1 <= Rows && Rows <= Columns);
	static constexpr int TranslationDim = Rows == Columns ? Rows - 1 : std::min(Rows, Columns);
public:
	template <class... Args, typename std::enable_if<(impl::All<impl::IsScalar, typename std::decay<Args>::type...>::value), int>::type = 0>
	static MatrixT Translation(Args&&... args) {
		static_assert(sizeof...(Args) == TranslationDim, "Number of arguments must match the dimension of translation.");

		MatrixT m;
		m.SetIdentity();
		T tableArgs[sizeof...(Args)] = { (T)std::forward<Args>(args)... };
		if (Order == eMatrixOrder::FOLLOW_VECTOR) {
			for (int i = 0; i < sizeof...(Args); ++i) {
				m(Rows - 1, i) = std::move(tableArgs[i]); //+++
			}
		}
		else {
			for (int i = 0; i < sizeof...(Args); ++i) {
				m(Columns - 1, i) = std::move(tableArgs[i]); //+++
			}
		}
		return m;
	}

	template <class Vt, bool Vpacked>
	static MatrixT Translation(const Vector<Vt, TranslationDim, Vpacked>& translation) {
		MatrixT m;
		m.SetIdentity();
		if (Order == eMatrixOrder::FOLLOW_VECTOR) {
			for (int i = 0; i < translation.Dimension(); ++i) {
				m(Rows - 1, i) = translation(i); //+++
			}
		}
		else {
			for (int i = 0; i < translation.Dimension(); ++i) {
				m(i, Columns - 1) = translation(i); //+++
			}
		}
		return m;
	}

	template <class... Args>
	MatrixT& SetTranslation(Args&&... args) { self() = Translation(std::forward<Args>(args)...); return self(); }

	template <class Vt, bool Vpacked>
	MatrixT& SetTranslation(const Vector<Vt, TranslationDim, Vpacked>& translation) { self() = Translation(translation); return self(); }
protected:
	friend class MatrixT;
	using Inherit = MatrixModule<EnableTranslation, MatrixTranslation>;
};


// Scale functions
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixScale {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
public:
	template <class... Args, typename std::enable_if<(impl::All<impl::IsScalar, Args...>::value), int>::type = 0>
	static MatrixT Scale(Args&&... args) {
		static_assert(sizeof...(Args) <= std::min(Rows, Columns), "You must provide scales for dimensions equal to matrix dimension");
		MatrixT m;
		m.SetZero();
		T tableArgs[sizeof...(Args)] = { (T)std::forward<Args>(args)... };
		int i;
		for (i = 0; i < sizeof...(Args); ++i) {
			m(i, i) = std::move(tableArgs[i]);
		}
		for (; i < std::min(Rows, Columns); ++i) {
			m(i, i) = T(1);
		}
		return m;
	}

	template <class Vt, int Vdim, bool Vpacked>
	static void Scale(const Vector<Vt, Vdim, Vpacked>& scale) {
		static_assert(Vdim < std::min(Rows, Columns), "Vector dimension must be smaller than or equal to matrix dimension.");
		MatrixT m;
		m.SetIdentity();
		int i;
		for (i = 0; i < scale.Dimension(); ++i) {
			m(i, i) = std::move(scale(i));
		}
		for (; i < std::min(Rows, Columns); ++i) {
			m(i, i) = T(1);
		}
		return m;
	}

	template <class... Args>
	MatrixT& SetScale(Args&&... args) { self() = Scale(std::forward<Args>(args)...); return self(); }

	template <class Vt, int Vdim, bool Vpacked>
	MatrixT& SetScale(const Vector<Vt, Vdim, Vpacked>& translation) { self() = Scale(translation); return self(); }
protected:
	friend class MatrixT;
	using Inherit = MatrixScale;
};



// View and projection transforms
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class MatrixProjective {
	using MatrixT = Matrix<T, Rows, Columns, Order, Layout, Packed>;
	MatrixT& self() { return *static_cast<MatrixT*>(this); }
	const MatrixT& self() const { return *static_cast<const MatrixT*>(this); }
public:

};




//------------------------------------------------------------------------------
// Global Matrix function prototypes
//------------------------------------------------------------------------------

// Same layout
template <
	class T,
	class U,
	int Rows,
	int Columns,
	eMatrixOrder Order1,
	eMatrixOrder Order2,
	eMatrixLayout SameLayout,
	bool Packed,
	class V = decltype(T() + U())
>
Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator+(
	const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>&,
	const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>&);


template <
	class T,
	class U,
	int Rows,
	int Columns,
	eMatrixOrder Order1,
	eMatrixOrder Order2,
	eMatrixLayout SameLayout,
	bool Packed,
	class V = decltype(T() - U())
>
Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator-(
	const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>&,
	const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>&);


// Opposite layout
template <
	class T,
	class U,
	int Rows,
	int Columns,
	eMatrixOrder Order1,
	eMatrixOrder Order2,
	eMatrixLayout Layout1,
	eMatrixLayout Layout2,
	bool Packed,
	class V = decltype(T() + U()),
	class = typename std::enable_if<Layout1 != Layout2>::type
>
Matrix<U, Rows, Columns, Order1, Layout1, Packed> operator+(
	const Matrix<T, Rows, Columns, Order1, Layout1, Packed>&,
	const Matrix<U, Rows, Columns, Order2, Layout2, Packed>&);

template <
	class T,
	class U,
	int Rows,
	int Columns,
	eMatrixOrder Order1,
	eMatrixOrder Order2,
	eMatrixLayout Layout1,
	eMatrixLayout Layout2,
	bool Packed,
	class V = decltype(T() - U()),
	class = typename std::enable_if<Layout1 != Layout2>::type
>
Matrix<U, Rows, Columns, Order1, Layout1, Packed> operator-(
	const Matrix<T, Rows, Columns, Order1, Layout1, Packed>&,
	const Matrix<U, Rows, Columns, Order2, Layout2, Packed>&);




//------------------------------------------------------------------------------
// Matrix class providing the common interface for all matrices
//------------------------------------------------------------------------------

template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
class __declspec(empty_bases) Matrix
	: public MatrixData<T, Rows, Columns, Order, Layout, Packed>,
	public MatrixSquare<T, Rows, Columns, Order, Layout, Packed>::Inherit,
	public MatrixRotation2D<T, Rows, Columns, Order, Layout, Packed>::Inherit,
	public MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::Inherit,
	public MatrixTranslation<T, Rows, Columns, Order, Layout, Packed>::Inherit,
	public MatrixScale<T, Rows, Columns, Order, Layout, Packed>::Inherit
{
	static_assert(Columns >= 1 && Rows >= 1, "Dimensions must be positive integers.");
protected:
	using MatrixData<T, Rows, Columns, Order, Layout, Packed>::GetElement;
	using MatrixData::stripes;

	template <class T2, int Dim, eMatrixOrder Order2, eMatrixLayout Layout2, bool Packed2>
	friend class mathter::DecompositionLU;
public:
	static void DumpLayout(std::ostream& os) {
		Matrix* ptr = reinterpret_cast<Matrix*>(1000);
		using T1 = MatrixData<T, Rows, Columns, Order, Layout, Packed>;
		using T2 = MatrixSquare<T, Rows, Columns, Order, Layout, Packed>::Inherit;
		using T3 = MatrixRotation2D<T, Rows, Columns, Order, Layout, Packed>::Inherit;
		using T4 = MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::Inherit;
		using T5 = MatrixTranslation<T, Rows, Columns, Order, Layout, Packed>::Inherit;
		using T6 = MatrixScale<T, Rows, Columns, Order, Layout, Packed>::Inherit;
		os << "MatrixData:        " << (intptr_t)static_cast<T1*>(ptr) - 1000 << " -> " << sizeof(T1) << std::endl;
		os << "MatrixSquare:      " << (intptr_t)static_cast<T2*>(ptr) - 1000 << " -> " << sizeof(T2) << std::endl;
		os << "MatrixRotation2D:  " << (intptr_t)static_cast<T3*>(ptr) - 1000 << " -> " << sizeof(T3) << std::endl;
		os << "MatrixRotation3D:  " << (intptr_t)static_cast<T4*>(ptr) - 1000 << " -> " << sizeof(T4) << std::endl;
		os << "MatrixTranslation: " << (intptr_t)static_cast<T5*>(ptr) - 1000 << " -> " << sizeof(T5) << std::endl;
		os << "MatrixScale:       " << (intptr_t)static_cast<T6*>(ptr) - 1000 << " -> " << sizeof(T6) << std::endl;
	}

	//--------------------------------------------
	// Constructors
	//--------------------------------------------

	Matrix() = default;
	
	template <class T2, eMatrixOrder Order2, eMatrixLayout Layout2>
	Matrix(const Matrix<T2, Rows, Columns, Order2, Layout2, Packed>& rhs) {
		for (int i = 0; i < RowCount(); ++i) {
			for (int j = 0; j < ColumnCount(); ++j) {
				(*this)(i,j) = rhs(i, j); //+++
			}
		}
	}

	template <class T2, eMatrixOrder Order2, eMatrixLayout Layout2>
	explicit Matrix(const Matrix<T2, Rows, Columns, Order2, Layout2, !Packed>& rhs) {
		for (int i = 0; i < RowCount(); ++i) {
			for (int j = 0; j < ColumnCount(); ++j) {
				(*this)(i, j) = rhs(i, j); //+++
			}
		}
	}

	template <class H, class... Args, typename std::enable_if<impl::All<impl::IsScalar, H, Args...>::value, int>::type = 0>
	Matrix(H h, Args... args) {
		static_assert(sizeof(Matrix) == sizeof(stripes), "Compiler did not optimize matrix size.");

		static_assert(1 + sizeof...(Args) == Columns*Rows, "All elements of matrix have to be initialized.");
		Assign<0, 0>(h, args...);
	}


	//--------------------------------------------
	// Accessors
	//--------------------------------------------

	// General matrix indexing
	T& operator()(int row, int col) {
		return GetElement(row, col); //+++
	}
	T operator()(int row, int col) const {
		return GetElement(row, col);
	}

	// Column and row vector simple indexing
	template <class = typename std::enable_if<(Columns == 1 && Rows > 1) || (Columns > 1 && Rows == 1)>::type>
	T& operator()(int idx) {
		return GetElement(Rows == 1 ? 0 : idx, Columns == 1 ? 0 : idx); //+++
	}
	template <class = typename std::enable_if<(Columns == 1 && Rows > 1) || (Columns > 1 && Rows == 1)>::type>
	T operator()(int idx) const {
		return GetElement(Rows == 1 ? 0 : idx, Columns == 1 ? 0 : idx); //+++
	}

	//--------------------------------------------
	// Compare
	//--------------------------------------------

	template <eMatrixOrder Order2, eMatrixLayout Layout2, bool Packed2>
	bool operator==(const Matrix<T, Rows, Columns, Order2, Layout2, Packed2>& rhs) const {
		bool equal = true;
		for (int i = 0; i < StripeCount; ++i) {
			equal = equal && stripes[i] == rhs.stripes[i];
		}
		return equal;
	}

	template <eMatrixOrder Order2, eMatrixLayout Layout2, bool Packed2>
	bool operator!=(const Matrix<T, Rows, Columns, Order2, Layout2, Packed2>& rhs) const {
		return !(*this == rhs);
	}

	template <eMatrixOrder Order2, eMatrixLayout Layout2, bool Packed2, class = typename std::enable_if<std::is_floating_point<T>::value>::type>
	bool AlmostEqual(const Matrix<T, Rows, Columns, Order2, Layout2, Packed2>& rhs) const {
		bool equal = true;
		for (int i = 0; i < RowCount(); ++i) {
			for (int j = 0; j < ColumnCount(); ++j) {
				equal = equal && impl::AlmostEqual((*this)(i, j), rhs(i, j));
			}
		}
		return equal;
	}

	//--------------------------------------------
	// Arithmetic
	//--------------------------------------------

	// Non-modifying external operators

	// Same layout
	template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout SameLayout, bool Packed, class V>
	friend Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator+(
		const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>&,
		const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>&);

	template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout SameLayout, bool Packed, class V>
	friend Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator-(
		const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>&,
		const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>&);
	

	// Multiplication row-row
	template <class T, class U, int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V>
	friend auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::ROW_MAJOR, Packed>& lhs,
						  const Matrix<U, Match, Columns2, Order2, eMatrixLayout::ROW_MAJOR, Packed>& rhs)
		->Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::ROW_MAJOR, Packed>;

	// Multiplication row-col
	template <class T, class U, int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V>
	friend auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::ROW_MAJOR, Packed>& lhs,
						  const Matrix<U, Match, Columns2, Order2, eMatrixLayout::COLUMN_MAJOR, Packed>& rhs)
		->Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::ROW_MAJOR, Packed>;

	// Multiplication col-col
	template <class T, class U, int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V>
	friend auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::COLUMN_MAJOR, Packed>& lhs,
				   const Matrix<U, Match, Columns2, Order2, eMatrixLayout::COLUMN_MAJOR, Packed>& rhs)
		->Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::COLUMN_MAJOR, Packed>;


	// Scalar multiplication
	Matrix operator*(T s) const {
		return Matrix(*this) *= s;
	}

	Matrix operator/(T s) const {
		return Matrix(*this) /= s;
	}

	// Unary signs
	Matrix operator+() const {
		return Matrix(*this);
	}

	Matrix operator-() const {
		return Matrix(*this) *= T(-1);
	}


	// Internal modifying operators

	// Addition, subtraction
	template <class U, eMatrixOrder Order2, eMatrixLayout Layout2>
	Matrix& operator+=(const Matrix<U, Rows, Columns, Order2, Layout2, Packed>& rhs) {
		*this = operator+<T, U, Rows, Columns, Order, Order2, Layout2, Packed, T>(*this, rhs);
	}

	template <class U, eMatrixOrder Order2, eMatrixLayout Layout2>
	Matrix& operator-=(const Matrix<U, Rows, Columns, Order2, Layout2, Packed>& rhs) {
		*this = operator-<T, U, Rows, Columns, Order, Order2, Layout2, Packed, T>(*this, rhs);
	}

	// Scalar multiplication
	Matrix& operator*=(T s) {
		for (auto& stripe : stripes) {
			stripe *= s;
		}
		return *this;
	}
	Matrix& operator/=(T s) {
		*this *= (1 / s);
		return *this;
	}


	//--------------------------------------------
	// Matrix functions
	//--------------------------------------------
	auto Transposed() const -> Matrix<T, Columns, Rows, Order, Layout, Packed> {
		Matrix<T, Columns, Rows, Order, Layout, Packed> result;
		for (int i = 0; i < RowCount(); ++i) {
			for (int j = 0; j < ColumnCount(); ++j) {
				result(j,i) = (*this)(i,j); //+++
			}
		}
		return result;
	}

	static Matrix Zero() {
		Matrix m;
		for (auto& stripe : m.stripes) {
			stripe.Spread(T(0));
		}
		return m;
	}

	Matrix& SetZero() {
		*this = Zero();
		return *this;
	}

	static Matrix Identity();
	Matrix& SetIdentity();


	//--------------------------------------------
	// Matrix-vector arithmetic
	//--------------------------------------------
	template <class Vt, class Mt, int Vd, int Mcol, eMatrixOrder Morder, bool Packed, class Rt>
	friend Vector<Rt, Mcol, Packed> operator*(const Vector<Vt, Vd, Packed>& vec, const Matrix<Mt, Vd, Mcol, Morder, eMatrixLayout::ROW_MAJOR, Packed>& mat);

	template <class Vt, class Mt, int Vd, int Mrow, eMatrixOrder Morder, bool Packed, class Rt>
	friend Vector<Rt, Mrow, Packed> operator*(const Matrix<Mt, Mrow, Vd, Morder, eMatrixLayout::ROW_MAJOR, Packed>& mat, const Vector<Vt, Vd, Packed>& vec);


protected:
	//--------------------------------------------
	// Helpers
	//--------------------------------------------

	template <int i, int j, class Head, class... Args> //+++
	void Assign(Head head, Args... args) { 
		(*this)(i, j) = head; //+++
		Assign<((j != Columns - 1) ? i : (i + 1)), ((j + 1) % Columns)>(args...); //+++
	}

	template <int, int>
	void Assign() {}
};



//------------------------------------------------------------------------------
// Matrix-Matrix arithmetic
//------------------------------------------------------------------------------

// Macros for manual matrix multiplication loop unrolling

// Row-major * Row-major
#define MATHTER_MATMUL_EXPAND(...) __VA_ARGS__

#define MATHTER_MATMUL_RR_FACTOR(X, Y) rhs.stripes[X] * lhs(Y, X) //+++

#define MATHTER_MATMUL_RR_STRIPE_1(Y) MATHTER_MATMUL_RR_FACTOR(0, Y)
#define MATHTER_MATMUL_RR_STRIPE_2(Y) MATHTER_MATMUL_RR_STRIPE_1(Y) + MATHTER_MATMUL_RR_FACTOR(1, Y)
#define MATHTER_MATMUL_RR_STRIPE_3(Y) MATHTER_MATMUL_RR_STRIPE_2(Y) + MATHTER_MATMUL_RR_FACTOR(2, Y)
#define MATHTER_MATMUL_RR_STRIPE_4(Y) MATHTER_MATMUL_RR_STRIPE_3(Y) + MATHTER_MATMUL_RR_FACTOR(3, Y)
#define MATHTER_MATMUL_RR_STRIPE(CX, Y) MATHTER_MATMUL_EXPAND(MATHTER_MATMUL_RR_STRIPE_ ## CX)(Y)

#define MATHTER_MATMUL_RR_ARRAY_1(CX) result.stripes[0] = MATHTER_MATMUL_RR_STRIPE(CX, 0) ;
#define MATHTER_MATMUL_RR_ARRAY_2(CX) MATHTER_MATMUL_RR_ARRAY_1(CX) result.stripes[1] = MATHTER_MATMUL_RR_STRIPE(CX, 1) ;
#define MATHTER_MATMUL_RR_ARRAY_3(CX) MATHTER_MATMUL_RR_ARRAY_2(CX) result.stripes[2] = MATHTER_MATMUL_RR_STRIPE(CX, 2) ;
#define MATHTER_MATMUL_RR_ARRAY_4(CX) MATHTER_MATMUL_RR_ARRAY_3(CX) result.stripes[3] = MATHTER_MATMUL_RR_STRIPE(CX, 3) ;

#define MATHTER_MATMUL_RR_ARRAY(CX, CY) MATHTER_MATMUL_EXPAND(MATHTER_MATMUL_RR_ARRAY_ ## CY)(CX)

#define MATHTER_MATMUL_RR_UNROLL(MATCH, ROWS1) if (Rows1 == ROWS1 && Match == MATCH ) { MATHTER_MATMUL_RR_ARRAY(MATCH, ROWS1) return result; }

// Column-major * Column-major
#define MATHTER_MATMUL_CC_FACTOR(X, Y) lhs.stripes[Y] * rhs(Y, X) //+++

#define MATHTER_MATMUL_CC_STRIPE_1(X) MATHTER_MATMUL_CC_FACTOR(X, 0)
#define MATHTER_MATMUL_CC_STRIPE_2(X) MATHTER_MATMUL_CC_STRIPE_1(X) + MATHTER_MATMUL_CC_FACTOR(X, 1)
#define MATHTER_MATMUL_CC_STRIPE_3(X) MATHTER_MATMUL_CC_STRIPE_2(X) + MATHTER_MATMUL_CC_FACTOR(X, 2)
#define MATHTER_MATMUL_CC_STRIPE_4(X) MATHTER_MATMUL_CC_STRIPE_3(X) + MATHTER_MATMUL_CC_FACTOR(X, 3)
#define MATHTER_MATMUL_CC_STRIPE(CY, X) MATHTER_MATMUL_EXPAND(MATHTER_MATMUL_CC_STRIPE_ ## CY)(X)

#define MATHTER_MATMUL_CC_ARRAY_1(CY) result.stripes[0] = MATHTER_MATMUL_CC_STRIPE(CY, 0) ;
#define MATHTER_MATMUL_CC_ARRAY_2(CY) MATHTER_MATMUL_CC_ARRAY_1(CY) result.stripes[1] = MATHTER_MATMUL_CC_STRIPE(CY, 1) ;
#define MATHTER_MATMUL_CC_ARRAY_3(CY) MATHTER_MATMUL_CC_ARRAY_2(CY) result.stripes[2] = MATHTER_MATMUL_CC_STRIPE(CY, 2) ;
#define MATHTER_MATMUL_CC_ARRAY_4(CY) MATHTER_MATMUL_CC_ARRAY_3(CY) result.stripes[3] = MATHTER_MATMUL_CC_STRIPE(CY, 3) ;

#define MATHTER_MATMUL_CC_ARRAY(CX, CY) MATHTER_MATMUL_EXPAND(MATHTER_MATMUL_CC_ARRAY_ ## CX)(CY)

#define MATHTER_MATMUL_CC_UNROLL(COLUMNS2, MATCH) if (Columns2 == COLUMNS2 && Match == MATCH ) { MATHTER_MATMUL_CC_ARRAY(COLUMNS2, MATCH) return result; }



template <class T, class U, int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V = MatMulElemT<T, U>>
auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::ROW_MAJOR, Packed>& lhs,
			   const Matrix<U, Match, Columns2, Order2, eMatrixLayout::ROW_MAJOR, Packed>& rhs)
	-> Matrix<V, Rows1, Columns2,  Order1, eMatrixLayout::ROW_MAJOR, Packed>
{
	Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::ROW_MAJOR, Packed> result;

	MATHTER_MATMUL_RR_UNROLL(2, 2);
	MATHTER_MATMUL_RR_UNROLL(2, 3);
	MATHTER_MATMUL_RR_UNROLL(2, 4);

	MATHTER_MATMUL_RR_UNROLL(3, 2);
	MATHTER_MATMUL_RR_UNROLL(3, 3);
	MATHTER_MATMUL_RR_UNROLL(3, 4);

	MATHTER_MATMUL_RR_UNROLL(4, 2);
	MATHTER_MATMUL_RR_UNROLL(4, 3);
	MATHTER_MATMUL_RR_UNROLL(4, 4);

	// general algorithm
	for (int i = 0; i < Rows1; ++i) {
		result.stripes[i] = rhs.stripes[0] * lhs(i, 0); //+++
	}
	for (int j = 1; j < Match; ++j) {
		for (int i = 0; i < Rows1; ++i) {
			result.stripes[i] += rhs.stripes[j] * lhs(i, j); //+++
		}
	}

	return result;
}

template <class T, class U,  int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V = MatMulElemT<T, U>>
auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::ROW_MAJOR, Packed>& lhs,
			   const Matrix<U, Match, Columns2, Order2, eMatrixLayout::COLUMN_MAJOR, Packed>& rhs)
	-> Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::ROW_MAJOR, Packed>
{
	Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::ROW_MAJOR, Packed> result;

	for (int j = 0; j < Columns2; ++j) {
		for (int i = 0; i < Rows1; ++i) {
			result(i, j) = Vector<T, Match, Packed>::Dot(lhs.stripes[i], rhs.stripes[j]);
		}
	}

	return result;
}

template <class T, class U, int Rows1, int Match, int Columns2, eMatrixOrder Order1, eMatrixOrder Order2, bool Packed, class V = MatMulElemT<T, U>>
auto operator*(const Matrix<T, Rows1, Match, Order1, eMatrixLayout::COLUMN_MAJOR, Packed>& lhs,
			   const Matrix<U, Match, Columns2, Order2, eMatrixLayout::COLUMN_MAJOR, Packed>& rhs)
	->Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::COLUMN_MAJOR, Packed>
{
	Matrix<V, Rows1, Columns2, Order1, eMatrixLayout::COLUMN_MAJOR, Packed> result;

	MATHTER_MATMUL_CC_UNROLL(2, 2);
	MATHTER_MATMUL_CC_UNROLL(2, 3);
	MATHTER_MATMUL_CC_UNROLL(2, 4);

	MATHTER_MATMUL_CC_UNROLL(3, 2);
	MATHTER_MATMUL_CC_UNROLL(3, 3);
	MATHTER_MATMUL_CC_UNROLL(3, 4);

	MATHTER_MATMUL_CC_UNROLL(4, 2);
	MATHTER_MATMUL_CC_UNROLL(4, 3);
	MATHTER_MATMUL_CC_UNROLL(4, 4);	

	// general algorithm
	for (int j = 0; j < Columns2; ++j) {
		result.stripes[j] = lhs.stripes[0] * rhs(0, j);
	}
	for (int i = 1; i < Match; ++i) {
		for (int j = 0; j < Columns2; ++j) {
			result.stripes[j] += lhs.stripes[i] * rhs(i, j);
		}
	}

	return result;
}



// Same layout
template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout SameLayout, bool Packed, class V>
Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator+(
	const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>& lhs,
	const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>& rhs)
{
	Matrix<U, Rows, Columns, Order1, SameLayout, Packed> result;
	for (int i = 0; i < result.StripeCount; ++i) {
		result.stripes[i] = lhs.stripes[i] + rhs.stripes[i];
	}
	return result;
}

template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout SameLayout, bool Packed, class V>
Matrix<U, Rows, Columns, Order1, SameLayout, Packed> operator-(
	const Matrix<T, Rows, Columns, Order1, SameLayout, Packed>& lhs,
	const Matrix<U, Rows, Columns, Order2, SameLayout, Packed>& rhs)
{
	Matrix<U, Rows, Columns, Order1, SameLayout, Packed> result;
	for (int i = 0; i < result.StripeCount; ++i) {
		result.stripes[i] = lhs.stripes[i] - rhs.stripes[i];
	}
	return result;
}


// Opposite layout
template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout Layout1, eMatrixLayout Layout2, bool Packed, class V, class>
Matrix<U, Rows, Columns, Order1, Layout1, Packed> operator+(
	const Matrix<T, Rows, Columns, Order1, Layout1, Packed>& lhs,
	const Matrix<U, Rows, Columns, Order2, Layout2, Packed>& rhs)
{
	Matrix<U, Rows, Columns, Order1, Layout1, Packed> result;
	for (int i = 0; i < result.RowCount(); ++i) {
		for (int j = 0; j < result.ColumnCount(); ++j) {
			result(i, j) = lhs(i, j) + rhs(i, j);
		}
	}
	return result;
}

template <class T, class U, int Rows, int Columns, eMatrixOrder Order1, eMatrixOrder Order2, eMatrixLayout Layout1, eMatrixLayout Layout2, bool Packed, class V, class>
Matrix<U, Rows, Columns, Order1, Layout1, Packed> operator-(
	const Matrix<T, Rows, Columns, Order1, Layout1, Packed>& lhs,
	const Matrix<U, Rows, Columns, Order2, Layout2, Packed>& rhs)
{
	Matrix<U, Rows, Columns, Order1, Layout1, Packed> result;
	for (int i = 0; i < result.RowCount(); ++i) {
		for (int j = 0; j < result.ColumnCount(); ++j) {
			result(i, j) = lhs(i, j) - rhs(i, j);
		}
	}
	return result;
}



//------------------------------------------------------------------------------
// Matrix-Scalar arithmetic
//------------------------------------------------------------------------------


template <class U, class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed, class = typename std::enable_if<impl::IsScalar<U>::value>::type>
Matrix<T, Rows, Columns, Order, Layout, Packed> operator*(U s, Matrix<T, Rows, Columns, Order, Layout, Packed> mat) {
	mat *= s;
	return mat;
}

template <class U, class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed, class = typename std::enable_if<impl::IsScalar<U>::value>::type>
Matrix<T, Rows, Columns, Order, Layout, Packed> operator/(U s, Matrix<T, Rows, Columns, Order, Layout, Packed> mat) {
	mat /= s;
	return mat;
}


//------------------------------------------------------------------------------
// General matrix function
//------------------------------------------------------------------------------
template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto Matrix<T, Rows, Columns, Order, Layout, Packed>::Identity() -> Matrix<T, Rows, Columns, Order, Layout, Packed> {
	Matrix<T, Rows, Columns, Order, Layout, Packed> res;

	res.SetZero();
	for (int i = 0; i < std::min(Rows, Columns); ++i) {
		res(i, i) = T(1);
	}

	return res;
}

template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto Matrix<T, Rows, Columns,  Order, Layout, Packed>::SetIdentity() ->Matrix<T, Rows, Columns, Order, Layout, Packed>& {
	*this = Identity();
	return *this;
}



//------------------------------------------------------------------------------
// Matrix-vector arithmetic
//------------------------------------------------------------------------------

// v*M
template <class Vt, class Mt, int Vd, int Mcol, eMatrixOrder Morder, bool Packed, class Rt = MatMulElemT<Vt, Mt>>
Vector<Rt, Mcol, Packed> operator*(const Vector<Vt, Vd, Packed>& vec, const Matrix<Mt, Vd, Mcol, Morder, eMatrixLayout::ROW_MAJOR, Packed>& mat) {
	Vector<Rt, Mcol, Packed> result;
	result = vec(0) * mat.stripes[0];
	for (int i = 1; i < Vd; ++i) {
		result += vec(i) * mat.stripes[i];
	}
	return result;
}

// M*v
template <class Vt, class Mt, int Vd, int Mrow, eMatrixOrder Morder, bool Packed, class Rt = MatMulElemT<Vt, Mt>>
Vector<Rt, Mrow, Packed> operator*(const Matrix<Mt, Mrow, Vd, Morder, eMatrixLayout::ROW_MAJOR, Packed>& mat, const Vector<Vt, Vd, Packed>& vec) {
	Vector<Rt, Mrow, Packed> result;
	for (int i = 0; i < Mrow; ++i) {
		result(i) = vec.Dot(vec, mat.stripes[i]);
	}
	return result;
}

// v*=M
template <class Vt, class Mt, int Vd, eMatrixOrder Morder, eMatrixLayout Layout, bool Packed>
Vector<Vt, Vd, Packed>& operator*=(Vector<Vt, Vd, Packed>& vec, const Matrix<Mt, Vd, Vd, Morder, Layout, Packed>& mat) {
	vec = operator*<Vt, Mt, Vd, Vd, Morder, Packed, Vt>(vec, mat);
	return vec;
}




//------------------------------------------------------------------------------
// Square matrices
//------------------------------------------------------------------------------

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
T MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::Trace() const {
	T sum = self()(0, 0);
	for (int i = 1; i < Dim; ++i) {
		sum += self()(i, i);
	}
	return sum;
}

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
T MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::Determinant() const {
	// only works for Crout's algorithm, where U's diagonal is 1s
	MatrixT L, U;
	self().DecomposeLU(L, U);
	T prod = L(0, 0);
	for (int i = 1; i < L.RowCount(); ++i) {
		prod *= L(i, i);
	}
	return prod;
}

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::Transpose() -> MatrixT& {
	self() = self().Transposed();
	return self();
}

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::Invert() -> MatrixT& {
	*this = Inverse();
	return self();
}

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::Inverse() const -> MatrixT {
	MatrixT ret;

	mathter::DecompositionLU<T, Dim, Order, Layout, Packed> LU = self().DecompositionLU();

	Vector<T, Dim, Packed> b(0);
	Vector<T, Dim, Packed> x;
	for (int col = 0; col < Dim; ++col) {
		b(std::max(0, col - 1)) = 0;
		b(col) = 1;
		LU.Solve(x, b);
		for (int i = 0; i < Dim; ++i) {
			ret(i, col) = x(i);
		}
	}

	return ret;
}


// From: https://www.gamedev.net/resources/_/technical/math-and-physics/matrix-inversion-using-lu-decomposition-r3637
template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
void MatrixSquare<T, Dim, Dim, Order, Layout, Packed>::DecomposeLU(MatrixT& L, MatrixT& U) const {
	const auto& A = self();
	constexpr int n = Dim;

	for (int i = 0; i < n; ++i) {
		for (int j = i + 1; j < n; ++j) {
			L(i, j) = 0;
		}
		for (int j = 0; j <= i; ++j) {
			U(i, j) = i == j;
		}
	}

	// Crout's algorithm
	for (int i = 0; i < n; ++i) {
		L(i, 0) = A(i, 0);
	}
	for (int j = 1; j < n; ++j) {
		U(0, j) = A(0, j) / L(0, 0);
	}

	for (int j = 1; j < n-1; ++j) {
		for (int i = j; i < n; ++i) {
			float Lij;
			Lij = A(i, j);
			for (int k = 0; k <= j - 1; ++k) {
				Lij -= L(i, k)*U(k, j);
			}
			L(i, j) = Lij;
		}
		for (int k = j; k < n; ++k) {
			float Ujk;
			Ujk = A(j, k);
			for (int i = 0; i <= j - 1; ++i) {
				Ujk -= L(j, i)*U(i, k);
			}
			Ujk /= L(j, j);
			U(j, k) = Ujk;
		}
	}

	L(n - 1, n - 1) = A(n - 1, n - 1);
	for (int k = 0; k < n - 1; ++k) {
		L(n - 1, n - 1) -= L(n - 1, k)*U(k, n - 1);
	}
}


//------------------------------------------------------------------------------
// Rotation 2D
//------------------------------------------------------------------------------


template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation2D<T, Rows, Columns, Order, Layout, Packed>::Rotation(T angle) -> MatrixT {
	MatrixT m;

	T C = cos(angle);
	T S = sin(angle);

	auto elem = [&m](int i, int j) -> T& {
		return Order == eMatrixOrder::FOLLOW_VECTOR ? m(i, j) : m(j, i);
	};

	// Indices according to follow vector order
	elem(0, 0) = C;		elem(0, 1) = S; //+++
	elem(1, 0) = -S;	elem(1, 1) = C;

	// Rest
	for (int j = 0; j < m.ColumnCount(); ++j) {
		for (int i = (j < 2 ? 2 : 0); i < m.RowCount(); ++i) {
			m(i, j) = T(j == i); //+++
		}
	}

	return m;
}


//------------------------------------------------------------------------------
// Rotation 3D
//------------------------------------------------------------------------------

template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
template <int Axis>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationAxis(T angle) -> MatrixT
{
	MatrixT m;

	T C = cos(angle);
	T S = sin(angle);

	auto elem = [&m](int i, int j) -> T& {
		return Order == eMatrixOrder::FOLLOW_VECTOR ? m(i, j) : m(j, i);
	};

	static_assert(0 <= Axis && Axis < 3, "You may choose X=0, Y=1 or Z=2 axes.");

	// Indices according to follow vector order
	if (Axis == 0) {
		// Rotate around X
		elem(0, 0) = 1;		elem(0, 1) = 0;		elem(0, 2) = 0;
		elem(1, 0) = 0;		elem(1, 1) = C;		elem(1, 2) = S;
		elem(2, 0) = 0;		elem(2, 1) = -S;	elem(2, 2) = C;
	}
	else if (Axis == 1) {
		// Rotate around Y
		elem(0, 0) = C;		elem(0, 1) = 0;		elem(0, 2) = -S;
		elem(1, 0) = 0;		elem(1, 1) = 1;		elem(1, 2) = 0;
		elem(2, 0) = S;		elem(2, 1) = 0;		elem(2, 2) = C;
	}
	else {
		// Rotate around Z
		elem(0, 0) = C;		elem(0, 1) = S;		elem(0, 2) = 0;
		elem(1, 0) = -S;	elem(1, 1) = C;		elem(1, 2) = 0;
		elem(2, 0) = 0;		elem(2, 1) = 0;		elem(2, 2) = 1;
	}

	// Rest
	for (int j = 3; j < m.ColumnCount(); ++j) {
		for (int i = (j < 3 ? 3 : 0); i < m.RowCount(); ++i) {
			m(i, j) = T(j == i);
		}
	}

	return m;
}


template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationX(T angle)-> MatrixT
{
	return RotationAxis<0>(angle);
}


template <class T,  int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationY(T angle) -> MatrixT
{
	return RotationAxis<1>(angle);
}


template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationZ(T angle) -> MatrixT
{
	return RotationAxis<2>(angle);
}


template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
template <int Axis1, int Axis2, int Axis3>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationAxis3(T angle1, T angle2, T angle3) -> MatrixT
{
	return RotationAxis<Axis1>(angle1) * RotationAxis<Axis2>(angle2) * RotationAxis<Axis3>(angle3);
}


template <class T, int Rows, int Columns, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationEuler(float z1, float x2, float z3) -> MatrixT
{
	return RotationAxis3<2, 0, 2>(z1, x2, z3);
}


template <class T, int Rows, int Columns,  eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationRPY(float x1, float y2, float z3) -> MatrixT
{
	return RotationAxis3<0, 1, 2>(x1, y2, z3);
}


template <class T, int Rows, int Columns,  eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
template <class U, bool Vpacked>
auto MatrixRotation3D<T, Rows, Columns, Order, Layout, Packed>::RotationAxisAngle(const Vector<U, 3, Vpacked>& axis, T angle) -> MatrixT
{
	MatrixT m;

	T C = cos(angle);
	T S = sin(angle);

	// 3x3 rotation sub-matrix
	using RotMat = Matrix<T, 3, 3, eMatrixOrder::FOLLOW_VECTOR>;
	Matrix<T, 3, 1, eMatrixOrder::FOLLOW_VECTOR> u(axis(0), axis(1), axis(2));
	RotMat cross = {
		0, -u(2), u(1),
		u(2), 0, -u(0),
		-u(1), u(0), 0
	};
	RotMat rot = C*RotMat::Identity() + S*cross + (1 - C)*(u*u.Transposed());


	// Elements
	auto elem = [&m](int i, int j) -> T& {
		return Order == eMatrixOrder::PRECEDE_VECTOR ? m(i, j) : m(j, i);
	};
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 3; ++i) {
			elem(i, j) = rot(i, j);
		}
	}

	// Rest
	for (int j = 3; j < m.Width(); ++j) {
		for (int i = (j < 3 ? 3 : 0); i < m.Height(); ++i) {
			m(i, j) = T(j == i);
		}
	}

	return m;
}


//------------------------------------------------------------------------------
// Decompositions
//------------------------------------------------------------------------------

template <class T, int Dim, eMatrixOrder Order, eMatrixLayout Layout, bool Packed>
bool DecompositionLU<T, Dim, Order, Layout, Packed>::Solve(Vector<float, Dim, Packed>& x, const Vector<T, Dim, Packed>& b) {
	if (!solvable) {
		for (int i = 0; i < Dim; ++i) {
			x(i) = T(0);
		}
		return false;
	}
	
	// Solve Ld = b
	Matrix<T, Dim, Dim + 1, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, Packed> L_b;

	for (int i = 0; i < Dim; ++i) {
		for (int j = 0; j < Dim; ++j) {
			L_b(i, j) = L(i, j);
		}
		L_b(i, Dim) = b(i);
	}

	for (int i = 0; i < Dim - 1; ++i) {
		for (int i2 = i + 1; i2 < Dim; ++i2) {
			L_b.stripes[i] /= L_b(i, i);
			T coeff = L_b(i2, i);
			L_b.stripes[i2] -= L_b.stripes[i] * coeff;
		}
	}
	L_b.stripes[Dim - 1] /= L_b(Dim - 1, Dim - 1);

	// Solve Ux = d
	Matrix<T, Dim, Dim + 1, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, Packed> U_d;
	for (int i = 0; i < Dim; ++i) {
		for (int j = 0; j < Dim; ++j) {
			U_d(i, j) = U(i, j);
		}
		U_d(i, Dim) = L_b(i, Dim);
	}

	// only works for Crout's algorithm, where U's diagonal is 1s
	for (int i = Dim - 1; i > 0; --i) {
		for (int i2 = i - 1; i2 >= 0; --i2) {
			T coeff = U_d(i2, i);
			U_d.stripes[i2] -= U_d.stripes[i] * coeff;
		}
	}

	// Output resulting vector
	for (int i = 0; i < Dim; ++i) {
		x(i) = U_d(i, Dim);
	}

	return true;
}


} // namespace mathter




template <class T, int Rows, int Columns, mathter::eMatrixOrder Order, mathter::eMatrixLayout Layout, bool Packed>
std::ostream& operator<<(std::ostream& os, const mathter::Matrix<T, Rows, Columns, Order, Layout, Packed>& mat) {
	for (int i = 0; i < mat.Height(); ++i) {
		os << "[";
		for (int j = 0; j < mat.Width(); ++j) {
			os << mat(i, j) << (j == mat.Width() - 1 ? "" : "\t");
		}
		os << "]\n";
	}
	return os;
}



// Remove goddamn fucking bullshit crapware winapi macros.
#if defined(MATHTER_MINMAX)
#pragma pop_macro("min")
#pragma pop_macro("max")
#endif
