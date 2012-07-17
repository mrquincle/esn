/*************************************************************************
Copyright (c) 1992-2007 The University of Tennessee. All rights reserved.

Contributors:
    * Sergey Bochkanov (ALGLIB project). Translation from FORTRAN to
      pseudocode.

See subroutines comments for additional copyrights.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer listed
  in this license in the documentation and/or other materials
  provided with the distribution.

- Neither the name of the copyright holders nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#ifndef _lu_h
#define _lu_h

#include "../eigenvalues/ap.h"
#include "ialglib.h"

/*************************************************************************
LU decomposition of a general matrix of size MxN

The subroutine calculates the LU decomposition of a rectangular general
matrix with partial pivoting (with row permutations).

Input parameters:
    A   -   matrix A whose indexes range within [0..M-1, 0..N-1].
    M   -   number of rows in matrix A.
    N   -   number of columns in matrix A.

Output parameters:
    A   -   matrices L and U in compact form (see below).
            Array whose indexes range within [0..M-1, 0..N-1].
    Pivots - permutation matrix in compact form (see below).
            Array whose index ranges within [0..Min(M-1,N-1)].

Matrix A is represented as A = P * L * U, where P is a permutation matrix,
matrix L - lower triangular (or lower trapezoid, if M>N) matrix,
U - upper triangular (or upper trapezoid, if M<N) matrix.

Let M be equal to 4 and N be equal to 3:

                   (  1          )    ( U11 U12 U13  )
A = P1 * P2 * P3 * ( L21  1      )  * (     U22 U23  )
                   ( L31 L32  1  )    (         U33  )
                   ( L41 L42 L43 )

Matrix L has size MxMin(M,N), matrix U has size Min(M,N)xN, matrix P(i) is
a permutation of the identity matrix of size MxM with numbers I and Pivots[I].

The algorithm returns array Pivots and the following matrix which replaces
matrix A and contains matrices L and U in compact form (the example applies
to M=4, N=3).

 ( U11 U12 U13 )
 ( L21 U22 U23 )
 ( L31 L32 U33 )
 ( L41 L42 L43 )

As we can see, the unit diagonal isn't stored.

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     June 30, 1992
*************************************************************************/
void rmatrixlu(ap::real_2d_array& a,
     int m,
     int n,
     ap::integer_1d_array& pivots);


/*************************************************************************
Obsolete 1-based subroutine. Left for backward compatibility.
See RMatrixLU for 0-based replacement.
*************************************************************************/
void ludecomposition(ap::real_2d_array& a,
     int m,
     int n,
     ap::integer_1d_array& pivots);


/*************************************************************************
Obsolete 1-based subroutine. Left for backward compatibility.
*************************************************************************/
void ludecompositionunpacked(ap::real_2d_array a,
     int m,
     int n,
     ap::real_2d_array& l,
     ap::real_2d_array& u,
     ap::integer_1d_array& pivots);


#endif
