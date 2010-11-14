#pragma once

class CRectReal
{
public:
	CRectReal(void);
	~CRectReal(void);

	double Height() {return bottom - top;}
	double Width() {return right - left;}

	double top;
	double bottom;
	double left;
	double right;
};

