// Matrix.cpp: implementation of the CMatrix class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Matrix.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction CMatrix
//////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL(CMatrix, CObject, 1) ;

#ifdef _DEBUG
int CMatrix::m_NextObjectNumber = 1 ;
#endif

CMatrix::CMatrix()
{
#ifdef _DEBUG
	// so we can regonise each individual object
	m_ObjectNumber = m_NextObjectNumber++ ;
	TRACE("Creating CMatrix object %1d - default constructor\n", m_ObjectNumber) ;
#endif
	// default constructor, create a 1 * 1 array
	m_NumColumns = 1 ;
	m_NumRows = 1 ;
	m_pData = NULL ;
	m_pData = AllocateMemory(m_NumColumns, m_NumRows) ;
	IncrementReferenceCount() ;			// count the reference to this memory
}

CMatrix::CMatrix(const CMatrix &other)
{
#ifdef _DEBUG
	// so we can regonise each individual object
	m_ObjectNumber = m_NextObjectNumber++ ;
	TRACE("Creating CMatrix object %1d - copy constructor other = %1d\n", m_ObjectNumber, other.m_ObjectNumber) ;
#endif
	// copy constructor
	m_pData = NULL ;
	// use the other objects data pointer
	m_NumColumns = other.m_NumColumns ;
	m_NumRows = other.m_NumRows ;
	m_pData = other.m_pData ;				// copy the pointer
	IncrementReferenceCount() ;				// this thread can get the mutex multiple times without blocking
}

CMatrix::CMatrix(int nCols, int nRows)
{
#ifdef _DEBUG
	// so we can regonise each individual object
	m_ObjectNumber = m_NextObjectNumber++ ;
	TRACE("Creating CMatrix object %1d - size constructor\n", m_ObjectNumber) ;
#endif
	// size constructor
	ASSERT(nCols > 0) ;					// matrix size error
	ASSERT(nRows > 0) ;					// matrix size error
	m_pData = NULL ;
	m_NumColumns = nCols ;
	m_NumRows = nRows ;
	m_pData = AllocateMemory(m_NumColumns, m_NumRows) ;
	IncrementReferenceCount() ;				// count the reference to this memory
}

CMatrix::CMatrix(int size, bool set_diagonal)
{
	// construct a square matrix with 1.0's on the diagonal if required
#ifdef _DEBUG
	// so we can regonise each individual object
	m_ObjectNumber = m_NextObjectNumber++ ;
	TRACE("Creating CMatrix object %1d - square size constructor\n", m_ObjectNumber) ;
#endif
	// size constructor
	ASSERT(size > 0) ;						// matrix size error
	m_pData = NULL ;
	m_NumColumns = size ;
	m_NumRows = size ;
	m_pData = AllocateMemory(m_NumColumns, m_NumRows) ;
	IncrementReferenceCount() ;				// count the reference to this memory
	// set the dialognal if required
	if (set_diagonal)
		{
		for (int i = 0 ; i < size ; ++i)
			SetElement(i, i, 1.0) ;
		}
}

// creates a CMatrix object from a SafeArray that contains a 2D matrix
// Note that you will probably have to call "VariantClear" to correctly de-allocate
// the safe array you have once you have finished with it.
CMatrix::CMatrix(VARIANT& var)
{
	if ((var.vt & VT_ARRAY) == 0)
		throw "Not a SafeArray" ;
	if ((var.vt & VT_R8) == 0)
		throw "Not a double SafeArray" ;

	SAFEARRAY*		psa = var.parray ;	// get a pointer to the safe array
	if (psa->cDims != 2)
		throw "SafeArray, incorrect number of dimensions" ;

	long			lBound1, lBound2 ;
	long			uBound1, uBound2 ;
	HRESULT			hr ;

	// get the bounds of the matrix in the safe array
	hr = SafeArrayGetLBound(psa, 1, &lBound1);
	if (FAILED(hr))
		throw "SafeArray access error" ;
	hr = SafeArrayGetUBound(psa, 1, &uBound1);
	if (FAILED(hr))
		throw "SafeArray access error" ;

	hr = SafeArrayGetLBound(psa, 2, &lBound2);
	if (FAILED(hr))
		throw "SafeArray access error" ;
	hr = SafeArrayGetUBound(psa, 2, &uBound2);
	if (FAILED(hr))
		throw "SafeArray access error" ;

	double*		dummy = NULL ;						// for access to the data
	m_NumColumns = uBound1 - lBound1 + 1 ;
	m_NumRows = uBound2 - lBound2 + 1 ;
	m_pData = AllocateMemory(m_NumColumns, m_NumRows) ;
	IncrementReferenceCount() ;			// count the reference to this memory

	SafeArrayAccessData(psa, (void**)(&dummy)) ;	// dummy now points to the data
	
	// copy each element across into the matrix to return
	for (int i = 0; i < m_NumColumns ; ++i)
		{
		for (int j = 0; j < m_NumRows ; ++j)
			SetElement(i, j, dummy[(i - lBound1) * m_NumRows + j - lBound2]) ;
		}
	dummy = NULL ;									// no longer valid
	SafeArrayUnaccessData(psa) ;					// release the safe array data pointer
}

CMatrix::~CMatrix()
{
#ifdef _DEBUG
	TRACE("Destroying CMatrix object %1d\n", m_ObjectNumber) ;
#endif
	DecrementAndRelease() ;			// free's m_pData if no other references
}

#ifdef _DEBUG
void CMatrix::Dump(CDumpContext& dc) const
{
	UNUSED_PARAMETER(dc) ;
	TRACE("CMatrix object    #%1d\n", m_ObjectNumber) ;
	TRACE("Num columns     : %1d\n", m_NumColumns) ;
	TRACE("Num rows        : %1d\n", m_NumRows) ;
	TRACE("Data pointer    : %lx\n", m_pData) ;
	TRACE("Reference count : %1d\n", GetReferenceCount()) ;
	for (int i = 0 ; i < m_NumRows ; ++i)
		{
		TRACE("Row %2d,", i) ;
		for (int j = 0 ; j < m_NumColumns ; ++j)
			{
			TRACE("%e,", GetElement(j, i)) ;
			}
		TRACE("\n") ;
		Sleep(m_NumColumns *2) ;		// this is to allow all element data to be traced for very large matrices!
		}
}

void CMatrix::AssertValid() const
{
	ASSERT(m_NumColumns > 0) ;							// matrix size error
	ASSERT(m_NumRows > 0) ;							// matrix size error
	ASSERT(m_pData) ;								// bad pointer error
	ASSERT(FALSE == IsBadReadPtr(m_pData, sizeof(double) * (m_NumColumns * m_NumRows + 1))) ;
}
#endif

void CMatrix::Serialize(CArchive& archive)
{
	CObject::Serialize(archive) ;
	if (archive.IsStoring())
		{
		// writing the matrix
		// write the object header first so we can correctly recognise the object type "CMatrixC"
		long	header1 = 0x434d6174 ;
		long	header2 = 0x72697843 ;
		int		version = 1 ;						// serialization version format number

		archive << header1 ;
		archive << header2 ;
		archive << version ;						// version number of object type

		// now write out the actual matrix
		archive << m_NumColumns ;
		archive << m_NumRows ;
		// this could be done with a archive.Write(m_pData, sizeof(double) * m_NumColumns * m_NumRows)
		// for efficiency (dont forget its a flat array). Not done here for clarity
		for (int i = 0 ; i < m_NumColumns ; ++i)
			{
			for (int j = 0 ; j < m_NumRows ; ++j)
				{
				archive << GetElement(i, j) ;		
				}
			}
		// done!
		}
	else
		{
		// reading the matrix
		// read the object header first so we can correctly recognise the object type "CMatrixC"
		long	header1 = 0 ;
		long	header2 = 0 ;
		int		version = 0 ;				// serialization version format

		archive >> header1 ;
		archive >> header2 ;
		if (header1 != 0x434d6174 || header2 != 0x72697843)
			{
			// incorrect header, cannot load it
			AfxThrowArchiveException(CArchiveException::badClass, NULL) ;
			}
		archive >> version ;				// version number of object type
		ASSERT(version == 1) ;				// only file format number so far

		// now write out the actual matrix
		int		nCols ;
		int		nRows ;
		double	value ;
		archive >> nCols ;
		archive >> nRows ;
		CMatrix loading(nCols, nRows) ;
		for (int i = 0 ; i < nCols ; ++i)
			{
			for (int j = 0 ; j < nRows ; ++j)
				{
				archive >> value ;
				loading.SetElement(i, j, value) ;
				}
			}
		*this = loading ;					// copy the data into ourselves
		// done!
		}
}

double* CMatrix::AllocateMemory(int nCols, int nRows)
{
	ASSERT(nCols > 0) ;							// size error
	ASSERT(nRows > 0) ;							// size error
	// allocates heap memory for an array
	double	*pData = NULL ;

	pData = new double[nCols * nRows + 1] ;			// all in one allocation (+1 for reference count)
	ASSERT(pData != NULL) ;					// allocation error
	ASSERT(FALSE == IsBadReadPtr(pData, sizeof(double) * (nCols * nRows + 1))) ;
	// empty the memory
	memset(pData, 0, sizeof(double) * (nCols * nRows + 1)) ;		// starts with a 0 reference count
	return pData ;
}

CMatrix& CMatrix::operator=(const CMatrix &other)
{
	if (&other == this)
		return *this ;
	// this does the same job as a copy constructor except we have to de-allocate any
	// memory we may have already allocated
	DecrementAndRelease() ;			// free's m_pData if no other references
	// now copy the other matrix into ourselves
	// use the other objects data pointer
	m_NumColumns = other.m_NumColumns ;
	m_NumRows = other.m_NumRows ;
	m_pData = other.m_pData ;				// copy the pointer
	IncrementReferenceCount() ;				// this thread can get the mutex multiple times without blocking
	// finally return a reference to ourselves
	return *this ;
}

bool CMatrix::operator==(const CMatrix &other) const
{
	// only return true if the matrices are exactly the same
	if (&other == this)
		return true ;		// comparing to ourselves
	if (m_pData == other.m_pData)
		return true ;		// both pointing to same data, must be same
	if (m_NumColumns != other.m_NumColumns || m_NumRows != other.m_NumRows)
		return false ;		// different dimensions
	if (memcmp(m_pData, other.m_pData, sizeof(double) * m_NumColumns * m_NumRows) == 0)
		return true ;		// buffers are the same
	return false ;			// must be different
}

CMatrix CMatrix::operator+(const CMatrix &other) const
{
	// first check for a valid addition operation
	if (m_NumColumns != other.m_NumColumns)
		throw "Invalid operation" ;
	if (m_NumRows != other.m_NumRows)
		throw "Invalid operation" ;
	// now that we know that the operation is possible
	ASSERT(FALSE == IsBadReadPtr(other.m_pData, sizeof(double) * other.m_NumColumns * other.m_NumRows)) ;
	// construct the object we are going to return
	CMatrix		result(*this) ;		// copy ourselves
	// now add in the other matrix
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			result.SetElement(i, j, result.GetElement(i, j) + other.GetElement(i, j)) ;
		}
	return result ;
}

CMatrix CMatrix::operator-(const CMatrix &other) const
{
	// first check for a valid subtraction operation
	if (m_NumColumns != other.m_NumColumns)
		throw "Invalid operation" ;
	if (m_NumRows != other.m_NumRows)
		throw "Invalid operation" ;
	// now that we know that the operation is possible
	ASSERT(FALSE == IsBadReadPtr(other.m_pData, sizeof(double) * other.m_NumColumns * other.m_NumRows)) ;
	// construct the object we are going to return
	CMatrix		result(*this) ;		// copy ourselves
	// now subtract the other matrix
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			result.SetElement(i, j, result.GetElement(i, j) - other.GetElement(i, j)) ;
		}
	return result ;
}

CMatrix CMatrix::operator*(const CMatrix &other) const
{
	// first check for a valid multiplication operation
	if (m_NumRows != other.m_NumColumns)
		throw "Matrices do not have common size" ;
	// now that we know that the operation is possible
	ASSERT(FALSE == IsBadReadPtr(other.m_pData, sizeof(double) * other.m_NumColumns * other.m_NumRows)) ;
	// construct the object we are going to return
	CMatrix		result(m_NumColumns, other.m_NumRows) ;

	// e.g.
	// [A][B][C]   [G][H]     [A*G + B*I + C*K][A*H + B*J + C*L]
	// [D][E][F] * [I][J] =   [D*G + E*I + F*K][D*H + E*J + F*L]
	//             [K][L]
	//
	double	 value ;
	for (int i = 0 ; i < result.m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < result.m_NumRows ; ++j)
			{
			value = 0.0 ;
			for (int k = 0 ; k < m_NumRows ; ++k)
				{
				value += GetElement(i, k) * other.GetElement(k, j) ;
				}
			result.SetElement(i, j, value) ;
			}
		}
	return result ;
}

void CMatrix::operator+=(const CMatrix &other)
{
	// first check for a valid addition operation
	if (m_NumColumns != other.m_NumColumns)
		throw "Invalid operation" ;
	if (m_NumRows != other.m_NumRows)
		throw "Invalid operation" ;
	// now that we know that the operation is possible
	ASSERT(FALSE == IsBadReadPtr(other.m_pData, sizeof(double) * other.m_NumColumns * other.m_NumRows)) ;
	// now add in the other matrix
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			SetElement(i, j, GetElement(i, j) + other.GetElement(i, j)) ;
		}
}

void CMatrix::operator-=(const CMatrix &other)
{
	// first check for a valid subtraction operation
	if (m_NumColumns != other.m_NumColumns)
		throw "Invalid operation" ;
	if (m_NumRows != other.m_NumRows)
		throw "Invalid operation" ;
	// now that we know that the operation is possible
	ASSERT(FALSE == IsBadReadPtr(other.m_pData, sizeof(double) * other.m_NumColumns * other.m_NumRows)) ;
	// now subtract the other matrix
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			SetElement(i, j, GetElement(i, j) - other.GetElement(i, j)) ;
		}
}

void CMatrix::operator*=(const CMatrix &other)
{
	// first check for a valid multiplication operation
	if (m_NumRows != other.m_NumColumns)
		throw "Matrices do not have common size" ;

	*this = *this * other ;
}

void CMatrix::operator*=(double value)
{
	// just multiply the elements by the value
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			SetElement(i, j, GetElement(i, j) * value) ;
			}
		}
}

// CMatrixHelper is only used for this to simulate a CMatrix::operator[][]
const CMatrixHelper CMatrix::operator[](int nCol) const
{
	ASSERT(nCol >= 0) ;							// array bounds error
	ASSERT(nCol < m_NumColumns) ;				// array bounds error
	// construc the CMatrixHelper object to allow operator[][] to work
	CMatrixHelper	mh(this, nCol) ;
	return mh ;
}

CMatrixHelper CMatrix::operator[](int nCol)
{
	ASSERT(nCol >= 0) ;							// array bounds error
	ASSERT(nCol < m_NumColumns) ;				// array bounds error
	// construc the CMatrixHelper object to allow operator[][] to work
	CMatrixHelper	mh(this, nCol) ;
	return mh ;
}

bool CMatrix::SetElement(int nCol, int nRow, double value)
{
	// first check the reference count on our data object to see whether we need to create a copy
	if (GetReferenceCount() > 1)
		{
		// we need to make a copy
		double	*pData = m_pData ;				// take a copy of the pointer
		DecrementReferenceCount() ;				// decrement the current reference count
		m_pData = AllocateMemory(m_NumColumns, m_NumRows) ;
		memcpy(m_pData, pData, sizeof(double) * m_NumColumns * m_NumRows) ;
		IncrementReferenceCount() ;				// increment the new data's reference count
		}
	ASSERT(nCol >= 0) ;							// array bounds error
	ASSERT(nCol < m_NumColumns) ;						// array bounds error
	ASSERT(nRow >= 0) ;							// array bounds error
	ASSERT(nRow < m_NumRows) ;						// array bounds error
	ASSERT(m_pData) ;							// bad pointer error
	m_pData[nCol + nRow * m_NumColumns] = value ;
	return true ;
}

#ifdef _DEBUG
// release version is in-line
double CMatrix::GetElement(int nCol, int nRow) const
{
	ASSERT(nCol >= 0) ;							// array bounds error
	ASSERT(nCol < m_NumColumns) ;						// array bounds error
	ASSERT(nRow >= 0) ;							// array bounds error
	ASSERT(nRow < m_NumRows) ;						// array bounds error
	ASSERT(m_pData) ;							// bad pointer error
	return m_pData[nCol + nRow * m_NumColumns] ;
}
#endif

// 
// To avoid big hits when constructing and assigning CMatrix objects, multiple CMatrix's can reference
// the same m_pData member. Only when a matrix becomes different from the other does a new version of the array
// get created and worked with.
//
void CMatrix::IncrementReferenceCount()
{
	// get a pointer to the end of the m_pData object where the reference count resides
	int*	pReference = (int*)&m_pData[m_NumColumns * m_NumRows] ;
	++(*pReference) ;				// increment the reference count
	// done!
}

void CMatrix::DecrementReferenceCount()
{
	// get a pointer to the end of the m_pData object where the reference count resides
	int*	pReference = (int*)&m_pData[m_NumColumns * m_NumRows] ;
	--(*pReference) ;				// decrement the reference count
	// done!
}

void CMatrix::DecrementAndRelease()
{
	// get a pointer to the end of the m_pData object where the reference count resides
	int*	pReference = (int*)&m_pData[m_NumColumns * m_NumRows] ;
	--(*pReference) ;				// decrement the reference count
	if (*pReference == 0)
		{
		// the memory is no longer needed, release it
		delete []m_pData ;
		m_pData = NULL ;
		}
	// done!
}

int CMatrix::GetReferenceCount() const
{
	// get a pointer to the end of the m_pData object where the reference count resides
	int*	pReference = (int*)&m_pData[m_NumColumns * m_NumRows] ;
	return *pReference ;
}

CMatrix CMatrix::GetTransposed() const
{
	CMatrix	transposed(*this) ;		// make a copy of ourselves

	transposed.Transpose() ;
	return transposed ;
}

void CMatrix::Transpose()
{
	// first check the reference count on our data object to see whether we need to create a copy
	CMatrix	mcopy(*this) ;
	// swap the x/y values
	int	copy = m_NumColumns ;
	m_NumColumns = m_NumRows ;
	m_NumRows = copy ;
	// copy across the transposed data
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			SetElement(i, j, mcopy.GetElement(j, i)) ;
		}
}

CMatrix CMatrix::GetInverted() const
{
	// matrix inversion will only work on square matrices
	if (m_NumColumns != m_NumRows)
		throw "Matrix must be square." ;
	// return this matrix inverted
	CMatrix	copy(*this) ;
	copy.Invert() ;

	return copy ;
}

void CMatrix::Invert()
{
	// matrix inversion will only work on square matrices
	// invert ourselves
	if (m_NumColumns != m_NumRows)
		throw "Matrix must be square." ;
	double	e ;

	for (int k = 0 ; k < m_NumColumns ; ++k)
		{
		e = GetElement(k, k) ;
		SetElement(k, k, 1.0) ;
		if (e == 0.0)
			throw "Matrix inversion error" ;
		for (int j = 0 ; j < m_NumColumns ; ++j)
			SetElement(k, j, GetElement(k, j) / e) ;
		for (int i = 0 ; i < m_NumColumns ; ++i)
			{
			if (i != k)
				{
				e = GetElement(i, k) ;
				SetElement(i, k, 0.0) ;
				for (int j = 0 ; j < m_NumColumns ; ++j)
					SetElement(i, j, GetElement(i, j) - e * GetElement(k, j)) ;
				}
			}
		}
}

// A' * A
CMatrix CMatrix::Covariant() const
{
	CMatrix	result ;
	CMatrix trans(GetTransposed()) ;

	result = *this * trans ;
	return result ;
}

CMatrix CMatrix::ExtractSubMatrix(int col_start, int row_start, int col_size, int row_size) const
{
	ASSERT(col_start >= 0) ;						// bad start index
	ASSERT(row_start >= 0) ;						// bad start index
	ASSERT(col_size > 0) ;						// bad size
	ASSERT(row_size > 0) ;						// bad size
	// make sure the requested sub matrix is in the current matrix
	if (col_start + col_size > m_NumColumns)
		throw "Sub matrix is not contained in source" ;
	if (row_start + row_size > m_NumRows)
		throw "Sub matrix is not contained in source" ;

	CMatrix sub(col_size, row_size) ;

	for (int i = 0 ; i < col_size ; ++i)
		{
		for (int j = 0 ; j < row_size ; ++j)
			{
			sub.SetElement(i, j, GetElement(col_start + i, row_start + j)) ;
			}
		}
	return sub ;
}

void CMatrix::SetSubMatrix(int col_start, int row_start, const CMatrix &other)
{
	ASSERT(col_start >= 0) ;						// bad start index
	ASSERT(row_start >= 0) ;						// bad start index
	ASSERT(col_start + other.m_NumColumns <= m_NumColumns) ;	// bad size
	ASSERT(row_start + other.m_NumRows <= m_NumRows) ;	// bad size
	for (int i = 0 ; i < other.m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < other.m_NumRows ; ++j)
			{
			SetElement(col_start + i, row_start + j, other.GetElement(i, j)) ;
			}
		}
}

CMatrix CMatrix::ExtractDiagonal() const
{
	if (m_NumColumns != m_NumRows)
		throw "Can only extract diagonal from square matrix" ;
	CMatrix	diagonal(m_NumColumns, 1) ;

	for (int i = 0 ; i < m_NumColumns ; ++i)
		diagonal.SetElement(i, 0, GetElement(i, i)) ;
	return diagonal ;
}


CMatrix CMatrix::GetConcatinatedColumns(const CMatrix& other) const
{
	if (m_NumRows != other.m_NumRows)
		throw "Cannot concatenate matrices, not same size" ;
	// copy ourselves and then return the concatenated result
	CMatrix		result(*this) ;

	result.ConcatinateColumns(other) ;
	return result ;
}

// concatinate the other matrix to ourselves
void CMatrix::ConcatinateColumns(const CMatrix &other)
{
	if (m_NumRows != other.m_NumRows)
		throw "Cannot concatenate matrices, not same size" ;
	// create a matrix big enough to hold both
	CMatrix		result(m_NumColumns + other.m_NumColumns, m_NumRows) ;

	// now populate it
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			result.SetElement(i, j, GetElement(i, j)) ;
			}
		}
	// now add the other matrix
	for (int i = 0 ; i < other.m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			result.SetElement(i + m_NumColumns, j, other.GetElement(i, j)) ;
			}
		}
	*this = result ;					// assign it to us
}

CMatrix CMatrix::GetConcatinatedRows(const CMatrix& other) const
{
	if (m_NumColumns != other.m_NumColumns)
		throw "Cannot concatenate matrices, not same size" ;
	// copy ourselves and then return the concatenated result
	CMatrix		result(*this) ;

	result.ConcatinateRows(other) ;
	return result ;
}

void CMatrix::ConcatinateRows(const CMatrix &other)
{
	if (m_NumColumns != other.m_NumColumns)
		throw "Cannot concatenate matrices, not same size" ;
	// create a matrix big enough to hold both
	CMatrix		result(m_NumColumns, m_NumRows + other.m_NumRows) ;

	// now populate it
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			result.SetElement(i, j, GetElement(i, j)) ;
			}
		}
	// now add the other matrix
	for (int i = 0 ; i < other.m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			result.SetElement(i, j + m_NumRows, other.GetElement(i, j)) ;
			}
		}
	*this = result ;					// assign it to us
}

void CMatrix::AddColumn(const double *pData)
{
	ASSERT(FALSE == IsBadReadPtr(pData, sizeof(double) * m_NumRows)) ;

	CMatrix	result(m_NumColumns + 1, m_NumRows) ;			// costruct the result

	result.SetSubMatrix(0, 0, *this) ;				// copy ouselves across
	// now add the new row
	for (int i = 0 ; i < m_NumRows ; ++i)
		{
		result.SetElement(m_NumColumns, i, pData[i]) ;
		}
	*this = result ;								// assign result to us
}

void CMatrix::AddRow(const double *pData)
{
	ASSERT(FALSE == IsBadReadPtr(pData, sizeof(double) * m_NumColumns)) ;

	CMatrix	result(m_NumColumns, m_NumRows + 1) ;			// costruct the result

	result.SetSubMatrix(0, 0, *this) ;				// copy ouselves across
	// now add the new row
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		result.SetElement(i, m_NumRows, pData[i]) ;
		}
	*this = result ;								// assign result to us
}

CMatrix	operator*(const CMatrix &other, double value)
{
	CMatrix copy(other) ;

	// just multiply the elements by the value
	for (int i = 0 ; i < copy.m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < copy.m_NumRows ; ++j)
			{
			copy.SetElement(i, j, copy.GetElement(i, j) * value) ;
			}
		}
	return copy ;
}

CMatrix CMatrix::GetSquareMatrix() const
{
	CMatrix	copy(*this) ;

	copy.MakeSquare() ;
	return copy ;
}

void CMatrix::MakeSquare()
{
	// make the current matrix square by either stepping in the x or y directions
	// square to the smallest side
	int size = m_NumColumns ;
	if (size > m_NumRows)
		size = m_NumRows ;

	CMatrix	work(size, size) ;				// construct result
	double	x_step = m_NumColumns / size ;
	double	y_step = m_NumRows / size ;

	for (int i = 0 ; i < size ; ++i)
		{
		for (int j = 0 ; j < size ; ++j)
			work.SetElement(i, j, GetElement((int)(i * x_step), (int)(j * y_step))) ;
		}
	*this = work ;				// copy the result to ourselves
}

CMatrix CMatrix::GetNormalised(double min, double max) const
{
	CMatrix copy(*this) ;
	copy.Normalise(min, max) ;
	return copy ;
}

void CMatrix::Normalise(double min, double max)
{
	// get the lower and upper limit values in the matrix
	// we use the range to normalise
	double	e_min ;
	double	e_max ;

	GetNumericRange(e_min, e_max) ;
	
	double	range = e_max - e_min ;
	double	r_range = max - min ;			// required range
	double	value ;
	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			value = GetElement(i, j) ;
			value -= e_min ;			// 0 - range
			value /= range ;
			value *= r_range ;
			value += min ;
			SetElement(i, j, value) ;
			}
		}
}

// gets the lowest and highest values in the matrix
void CMatrix::GetNumericRange(double &min, double &max) const
{
	double	e_min = GetElement(0, 0) ;
	double	e_max = e_min ;
	double	value ;

	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		for (int j = 0 ; j < m_NumRows ; ++j)
			{
			value = GetElement(i, j) ;
			if (value < e_min)
				e_min = value ;
			else if (value > e_max)
				e_max = value ;
			}
		}
	min = e_min ;
	max = e_max ;
}

double CMatrix::SumColumn(int column) const
{
	ASSERT(column >= 0) ;					// bad column
	ASSERT(column < m_NumColumns) ;				// bad column
	double	sum = 0.0 ;

	for (int i = 0 ; i < m_NumRows ; ++i)
		sum += GetElement(column, i) ;
	return sum ;
}

double CMatrix::SumRow(int row) const
{
	ASSERT(row >= 0) ;						// bad row
	ASSERT(row < m_NumRows) ;					// bad row
	double	sum = 0.0 ;

	for (int i = 0 ; i < m_NumColumns ; ++i)
		sum += GetElement(i, row) ;
	return sum ;
}

double CMatrix::SumColumnSquared(int column) const
{
	double value = SumColumn(column) ;
	return (value * value) ;
}

double CMatrix::SumRowSquared(int row) const
{
	double value = SumRow(row) ;
	return (value * value) ;
}

// returns the minimum value in a row of the matrix
double CMatrix::GetRowMin(int row) const
{
	ASSERT(row >= 0) ;
	ASSERT(row < m_NumRows) ;
	double	value = GetElement(0, row) ;
	for (int i = 1 ; i < m_NumColumns ; ++i)
		{
		if (GetElement(i, row) < value)
			value = GetElement(i, row) ;
		}
	return value ;
}

// returns the maximum value in a row of the matrix
double CMatrix::GetRowMax(int row) const
{
	ASSERT(row >= 0) ;
	ASSERT(row < m_NumRows) ;
	double	value = GetElement(0, row) ;
	for (int i = 1 ; i < m_NumColumns ; ++i)
		{
		if (GetElement(i, row) > value)
			value = GetElement(i, row) ;
		}
	return value ;
}

// returns the minimum value in a column of the matrix
double CMatrix::GetColumnMin(int column) const
{
	ASSERT(column >= 0) ;
	ASSERT(column < m_NumColumns) ;
	double	value = GetElement(column, 0) ;
	for (int i = 1 ; i < m_NumRows ; ++i)
		{
		if (GetElement(column, i) < value)
			value = GetElement(column, i) ;
		}
	return value ;
}

// returns the maximum value in a column of the matrix
double CMatrix::GetColumnMax(int column) const
{
	ASSERT(column >= 0) ;
	ASSERT(column < m_NumColumns) ;
	double	value = GetElement(column, 0) ;
	for (int i = 1 ; i < m_NumRows ; ++i)
		{
		if (GetElement(column, i) > value)
			value = GetElement(column, i) ;
		}
	return value ;
}

// returns the matrix in a SafeArray object which can be passed through COM components and used in VB etc
VARIANT CMatrix::GetSafeArray() const
{
	VARIANT		var ;
	// Set up the VARIANT to hold the SAFEARRAY.
	SAFEARRAY*			psa = NULL ;
	SAFEARRAYBOUND		bounds[2] =	{
									{m_NumColumns, 0},
									{m_NumRows, 0}
									};

	VariantClear(&var) ;
	var.vt = VT_ARRAY | VT_R8 ;			// Double array.
	
	psa = SafeArrayCreate(VT_R8, 2, bounds) ;
	if (psa == NULL)					// failed to create the safe array
		{
		TRACE("Failed to create SAFEARRAY[][]\n") ;
		VariantClear(&var) ;
		return var ;
		}	
	double* dummy = NULL ;	
	SafeArrayAccessData(psa, (void**)(&dummy));	
	// Iterate through the array of doubles, placing each value into the dummy variable.
	for (int i = 0; i < m_NumColumns ; ++i)
		{
		for (int j = 0; j < m_NumRows ; ++j)
			dummy[i * m_NumRows + j] = GetElement(i, j) ;
		}
	SafeArrayUnaccessData(psa) ;
	// Set the array member of the VARIANT to be our newly filled SAFEARRAY.
	var.parray = psa;
	// note that to avoid a leak the variant must be cleared using "VariantClear"
	// to properly de-allocate the safe array just created
	return var ;
}

// copies the matrix to the clipboard as text such as:
// 
// 4.0,50.0,60.0
// 3.52,785.0,56.2
//
void CMatrix::CopyToClipboard() const
{
	// create the text to be copied to the clipboard
	CString	text ;

	for (int i = 0 ; i < m_NumRows ; ++i)
		{
		text += GetRowAsText(i) ;
		text += "\r\n" ;
		}
	// now place the text on the clipboard
	if (OpenClipboard(NULL))
		{
		HGLOBAL		handle ;
		char		*pntr ;

		handle = ::GlobalAlloc(GHND, text.GetLength() + 1) ;
		pntr = (char *)::GlobalLock(handle) ;
		strcpy(pntr, text) ;
		::GlobalUnlock(handle) ;
		EmptyClipboard() ;
		SetClipboardData(CF_TEXT, handle);
		CloseClipboard();
		}
}

void CMatrix::WriteAsCSVFile(const CString& filename) const
{
	// create the file to write to
	CFile	file ;
	
	try {
		if (file.Open(filename, CFile::modeWrite | CFile::modeCreate))
			{
			CString	text ;
			for (int i = 0 ; i < m_NumRows ; ++i)
				{
				text = GetRowAsText(i) ;
				file.Write(text, text.GetLength()) ;
				file.Write("\r\n", 2) ;			// eol
				}
			file.Close() ;
			}
		}
	catch (CFileException &e)
		{
		// we have has a problem, determine what it is
		switch (e.m_cause)
			{
			// errors that should not occur
			case CFileException::none :
			case CFileException::fileNotFound :
			case CFileException::removeCurrentDir :
			case CFileException::endOfFile :
				break ;
			// errors that require us to move the destination file
			case CFileException::badPath :
				AfxMessageBox("Bad file path") ;
				return ;
			case CFileException::directoryFull :
				AfxMessageBox("The destination directory is full") ;
				return ;
			case CFileException::diskFull :
				AfxMessageBox("The destination disk is full") ;
				return ;
			case CFileException::genericException :
				AfxMessageBox("Generic (unknown error)") ;
				return ;
			case CFileException::tooManyOpenFiles :
				AfxMessageBox("Too many files are open") ;
				return ;
			case CFileException::accessDenied :
				AfxMessageBox("Access denied") ;
				return ;
			case CFileException::invalidFile :
				AfxMessageBox("Invalid file or name") ;
				return ;
			case CFileException::badSeek :
				AfxMessageBox("Bad seek on file") ;
				return ;
			case CFileException::hardIO :
				AfxMessageBox("Hardware IO error") ;
				return ;
			case CFileException::sharingViolation :
				AfxMessageBox("File sharing violation") ;
				return ;
			case CFileException::lockViolation :
				AfxMessageBox("File locking violation") ;
				return ;
			}
		}
}

// this function reads a file for "," separated values and creates a matrix object from it
CMatrix CMatrix::ReadFromCSVFile(const CString& filename)
{
	int		col_size = 0 ;
	int		row_size = 0 ;
	CFile	file ;

	try {
		if (file.Open(filename, CFile::modeRead))
			{
			CString	text = ReadLine(file) ;				// get a line from the file
			CString	token ;
			// count how many elements across their are
			int pos = 0 ;
			while (pos < text.GetLength())
				{
				pos = GetStringToken(text, token, pos, ',') ;
				col_size++ ;
				}
			// allocate an array to hold the data
			double	*pData = new double[col_size] ;
			CMatrix	m(col_size, 1) ;					// create a 1 row matrix which we will concatinate rows to

			while (text.GetLength() > 0)
				{
				int i = 0 ;
				pos = 0 ;
				while (pos < text.GetLength())
					{
					pos = GetStringToken(text, token, pos, ',') ;
					pData[i++] = atof(token) ;
					}
				ASSERT(i == col_size) ;
				text = ReadLine(file) ;
				if (row_size == 0)
					{
					// need to copy in the first row of data
					for (int i = 0 ; i < col_size ; ++i)
						m.SetElement(i, 0, pData[i]) ;
					}
				else
					m.AddRow(pData) ;			// append to end of matrix
				row_size++ ;
				}
			delete []pData ;
			file.Close() ;
			return m ;
			}
		}
	catch (CFileException &e)
		{
		// we have has a problem, determine what it is
		switch (e.m_cause)
			{
			// errors that should not occur
			case CFileException::none :
			case CFileException::fileNotFound :
			case CFileException::removeCurrentDir :
			case CFileException::endOfFile :
				break ;
			// errors that require us to move the destination file
			case CFileException::badPath :
				AfxMessageBox("Bad file path") ;
				break ;
			case CFileException::directoryFull :
				AfxMessageBox("The destination directory is full") ;
				break ;
			case CFileException::diskFull :
				AfxMessageBox("The destination disk is full") ;
				break ;
			case CFileException::genericException :
				AfxMessageBox("Generic (unknown error)") ;
				break ;
			case CFileException::tooManyOpenFiles :
				AfxMessageBox("Too many files are open") ;
				break ;
			case CFileException::accessDenied :
				AfxMessageBox("Access denied") ;
				break ;
			case CFileException::invalidFile :
				AfxMessageBox("Invalid file or name") ;
				break ;
			case CFileException::badSeek :
				AfxMessageBox("Bad seek on file") ;
				break ;
			case CFileException::hardIO :
				AfxMessageBox("Hardware IO error") ;
				break ;
			case CFileException::sharingViolation :
				AfxMessageBox("File sharing violation") ;
				break ;
			case CFileException::lockViolation :
				AfxMessageBox("File locking violation") ;
				break ;
			}
		}
	CMatrix problem ;
	return problem ;
}


// returns a row as , separated values
CString CMatrix::GetRowAsText(int row) const
{
	ASSERT(row >= 0) ;						// bad row
	ASSERT(row < m_NumRows) ;					// bad row
	CString	text ;
	CString	token ;

	for (int i = 0 ; i < m_NumColumns ; ++i)
		{
		if (i > 0)
			text += "," ;
		token.Format("%e", GetElement(i, row)) ;
		text += token ;
		}
	return text ;
}

// read a line of text from the current file
CString CMatrix::ReadLine(CFile& file)
{
	CString	line("") ;
	char	ch[3] ;
	DWORD	hBytesRead	= 0 ;

	while (true)
		{
		hBytesRead = file.Read(&ch[0], 2) ;
		if (hBytesRead == 2)
			file.Seek(-1L, CFile::current) ;		
		if (hBytesRead == 0)
			break ;					// end of file
		if (ch[0] == '\n')
			{
			if (ch[1] == '\r')
				file.Seek(1L, CFile::current) ;
			break ;
			}
		if (ch[0] == '\r')
			{
			ch[0] = '\n' ;
			if (ch[1] == '\n')
				file.Seek(1L, CFile::current) ;
			break ;
			}
		if (ch[0] != '\015')
			{
			// ignore LF characters
			ch[1] = '\0' ;
			line += ch ;
			}
		}
	return line ;
}

int CMatrix::GetStringToken(CString source, CString &destination, int start, char ch)
{
	ASSERT(start >= 0) ;

	// at the end of the source string ?
	if (start >= source.GetLength())
		{
		destination = "" ;					// no token available
		return source.GetLength() ;			// return @ end of string
		}
	
	// skip past any termination characters at the start position
	while (start < source.GetLength())
		{
		if (ch == source[start])
			start++ ;
		else
			break ;
		}
	// find the next occurance of the terminating character
	int pos = source.Find(ch, start) ;			// find termination character
	if (pos < 0)
		{
		// terminator not found, just return the remainder of the string
		destination = source.Right(source.GetLength() - start) ;
		return source.GetLength() ;
		}
	// found terminator, get sub string
	destination = source.Mid(start, pos - start) ;
	return pos ;
}
