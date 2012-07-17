
#ifndef _testnsevdunit_h
#define _testnsevdunit_h

#include "ap.h"

#include "blas.h"
#include "reflections.h"
#include "rotations.h"
#include "hsschur.h"
#include "hessenberg.h"
#include "nsevd.h"


bool testnonsymmetricevd(bool silent);


/*************************************************************************
Silent unit test
*************************************************************************/
bool testnsevdunit_test_silent();


/*************************************************************************
Unit test
*************************************************************************/
bool testnsevdunit_test();


#endif
