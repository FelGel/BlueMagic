// Matrix.h: interface for the CMatrix class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MATRIX_H__D58D0A47_68B4_11D6_AD90_00B0D0652E95__INCLUDED_)
#define AFX_MATRIX_H__D58D0A47_68B4_11D6_AD90_00B0D0652E95__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// ===================
// | IMPORTANT NOTE: |
// ============================================================================
// | Due to bug in Matrix class, ROWs are COLUMNSs and COLUMNSs are ROWs !!!  |
// ============================================================================
//																	R.C.

// 
// Class : CMatrix
// Written by : R.I.Allen
// 
// This is an encapsulation of a matrix class that allows all standard operations to be
// performed on it. It also includes some odd procedures for importing and exporting matrices.
// 
// Data is stored for the matrix in a flat allocation with 1 additional entry at the end which is
// used as a reference counter for how many objects are using that data section. When one matrix
// differs from another when referencing the same memory, a copy is made at that point, and they
// go their separate ways. This is very useful when returing CMatrix objects which then need to be
// assigned as it avoid additional memory allocation/deallocation calls.
//
#define UNUSED_PARAMETER(x) x
#include <math.h>

class CMatrix : public CObject  
{
public:
	DECLARE_SERIAL(CMatrix) ;
public:
	friend class	CMatrixHelper ;						// used for operator[][]
	// construction and destruction
					CMatrix() ;							// default constructor
					CMatrix(const CMatrix &other) ;		// copy constructor
					CMatrix(int nCols, int nRows) ;		// constructs an empty matrix of this size
					CMatrix(int size, bool set_diagonal = true) ;	// creates a square matrix
					CMatrix(VARIANT& var) ;				// from a SAFEARRAY variant
	virtual			~CMatrix();							// destructor
	void			Serialize(CArchive& archive) ;		// serialization
    
	// matrix mathematical operations
	CMatrix&		operator=(const CMatrix &other) ;
	CMatrix			operator+(const CMatrix &other) const ;
	CMatrix			operator-(const CMatrix &other) const ;
	CMatrix			operator*(const CMatrix &other) const ;
	void			operator+=(const CMatrix &other) ;
	void			operator-=(const CMatrix &other) ;
	void			operator*=(const CMatrix &other) ;
	void			operator*=(double value) ;
	friend CMatrix	operator*(const CMatrix &other, double value) ;
	bool			operator==(const CMatrix &other) const ;
	const CMatrixHelper	operator[](int nCol) const ;			// reading version
	CMatrixHelper	operator[](int nCol) ;					// writing version

	// element access
	bool			SetElement(int nCol, int nRow, double value) ;
#ifdef _DEBUG
	double			GetElement(int nCol, int nRow) const ;
#else
	inline double	GetElement(int nCol, int nRow) const { return m_pData[nCol + nRow * m_NumColumns] ; } ;
#endif
	inline int		GetNumColumns() const { return m_NumColumns ; } ;
	inline int		GetNumRows() const { return m_NumRows ; } ;
	double			SumColumn(int col) const ;
	double			SumRow(int row) const ;
	double			SumColumnSquared(int col) const ;
	double			SumRowSquared(int row) const ;
	double			GetRowMin(int row) const ;
	double			GetRowMax(int row) const ;
	double			GetColumnMin(int col) const ;
	double			GetColumnMax(int col) const ;
	// matrix transposition
	CMatrix			GetTransposed() const ;
	void			Transpose() ;
	// matrix inversion
	CMatrix			GetInverted() const ;
	void			Invert() ;
	// covariant (A' * A)
	CMatrix			Covariant() const ;
	// normalisation
	CMatrix			GetNormalised(double min, double max) const ;
	void			Normalise(double min, double max) ;
	// ranges functions
	void			GetNumericRange(double &min, double &max) const ;
	// matrix concatenation
	CMatrix			GetConcatinatedColumns(const CMatrix& other) const ;
	void			ConcatinateColumns(const CMatrix &other) ;
	CMatrix			GetConcatinatedRows(const CMatrix& other) const ;
	void			ConcatinateRows(const CMatrix &other) ;
	// adds an new row / column to the matrix
	void			AddColumn(const double *pData) ;
	void			AddRow(const double *pData) ;
	// sub matrix extraction, setting
	CMatrix			ExtractSubMatrix(int col_start, int row_start, int col_size, int row_size) const ;
	void			SetSubMatrix(int col_start, int row_start, const CMatrix &other) ;
	CMatrix			ExtractDiagonal() const ;
	// squaring the matrix functions
	CMatrix			GetSquareMatrix() const ;
	void			MakeSquare() ;
	// export functions/import
	VARIANT			GetSafeArray() const ;
	void			CopyToClipboard() const ;
	void			WriteAsCSVFile(const CString& filename) const ;
	static CMatrix	ReadFromCSVFile(const CString& filename) ;

	// virtual functions
#ifdef _DEBUG
	virtual void	Dump(CDumpContext& dc) const ;
	virtual void	AssertValid() const ;
#endif

private:
	// internal variables
	int				m_NumColumns ;			// number of columns in matrix
	int				m_NumRows ;				// number of rows in matrix
	double*			m_pData ;				// pointer to data, may be shared among objects
#ifdef _DEBUG
	// variables used in debug for obejct counting
	static int		m_NextObjectNumber ;
	int				m_ObjectNumber ;
#endif
private:
	// private internal functions
	double*			AllocateMemory(int nCols, int nROws) ;
	// reference counting functions
	void			IncrementReferenceCount() ;	// increments the m_pData reference count
	void			DecrementReferenceCount() ;	// decrements the m_pData reference count
	void			DecrementAndRelease() ;		// decrements the count and releases the memory if required
	int				GetReferenceCount() const ;	// returns the m_pData's reference count
	// helper functions
	CString			GetRowAsText(int row) const ;
	static CString	ReadLine(CFile &file) ;		// reads a \r\n delimited line of text from a file
	static int		GetStringToken(CString source, CString &destination, int start, char ch) ;
};

// this class is used to help the operator[][] on a matrix object work correctly
// it only provides the operator[]
class CMatrixHelper
{
public:
	friend class	CMatrix ;
protected:
	// protected constructor so only friend class can construct
	// a CMatrixHelper object
					CMatrixHelper(CMatrix* pMatrix, int col) : m_pMatrix(pMatrix), m_pMatrixConst(NULL)
						{
						m_Col = col ;
						} ;
					CMatrixHelper(const CMatrix* const pMatrix, int col) : m_pMatrixConst(pMatrix), m_pMatrix(NULL)
						{
						m_Col = col ;
						} ;
					CMatrixHelper(CMatrixHelper& other) : m_pMatrix(other.m_pMatrix), m_pMatrixConst(other.m_pMatrixConst)
						{
						m_Col = other.m_Col ;
						} ;
public:
					~CMatrixHelper()
						{
						} ;
	double			operator[](int row) const
						{
						ASSERT(row >= 0) ;									// array bounds error
						ASSERT(row < m_pMatrixConst->m_NumRows) ;				// array bounds error
						return m_pMatrixConst->m_pData[row * m_pMatrixConst->m_NumColumns + m_Col] ;
						} ;
	double&			operator[](int row)
						{
						// first check the reference count on our data object to see whether we need to create a copy
						if (m_pMatrix->GetReferenceCount() > 1)
							{
							// we need to make a copy
							double	*pData = m_pMatrix->m_pData ;			// take a copy of the pointer
							m_pMatrix->DecrementReferenceCount() ;			// decrement the current reference count
							m_pMatrix->m_pData = m_pMatrix->AllocateMemory(m_pMatrix->m_NumColumns, m_pMatrix->m_NumRows) ;
							memcpy(m_pMatrix->m_pData, pData, sizeof(double) * m_pMatrix->m_NumColumns * m_pMatrix->m_NumRows) ;
							m_pMatrix->IncrementReferenceCount() ;			// increment the new data's reference count
							}
						ASSERT(row >= 0) ;									// array bounds error
						ASSERT(row < m_pMatrix->m_NumRows) ;					// array bounds error
						ASSERT(m_pMatrix->m_pData) ;
						return m_pMatrix->m_pData[m_Col + row * m_pMatrix->m_NumColumns] ;
						} ;
private:
	// operator= is private to stop nefarious programmers!
	void			operator=(CMatrixHelper& other)
						{
						m_pMatrix = other.m_pMatrix ;
						m_pMatrixConst = other.m_pMatrixConst ;
						m_Col = other.m_Col ;
						} ;
	CMatrix*		m_pMatrix ;
	const CMatrix*	m_pMatrixConst ;
	int				m_Col ;						// column index for operator[][]
} ;

#endif // !defined(AFX_MATRIX_H__D58D0A47_68B4_11D6_AD90_00B0D0652E95__INCLUDED_)
