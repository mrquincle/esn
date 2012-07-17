/*************************************************************************
Copyright (c) 1992-2007 The University of Tennessee.  All rights reserved.

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

#include <stdafx.h>
#include "trinverse.h"

/*************************************************************************
Triangular matrix inversion

The subroutine inverts the following types of matrices:
    * upper triangular
    * upper triangular with unit diagonal
    * lower triangular
    * lower triangular with unit diagonal

In case of an upper (lower) triangular matrix,  the  inverse  matrix  will
also be upper (lower) triangular, and after the end of the algorithm,  the
inverse matrix replaces the source matrix. The elements  below (above) the
main diagonal are not changed by the algorithm.

If  the matrix  has a unit diagonal, the inverse matrix also  has  a  unit
diagonal, and the diagonal elements are not passed to the algorithm.

Input parameters:
    A       -   matrix.
                Array whose indexes range within [0..N-1, 0..N-1].
    N       -   size of matrix A.
    IsUpper -   True, if the matrix is upper triangular.
    IsUnitTriangular
            -   True, if the matrix has a unit diagonal.

Output parameters:
    A       -   inverse matrix (if the problem is not degenerate).

Result:
    True, if the matrix is not singular.
    False, if the matrix is singular.

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     February 29, 1992
*************************************************************************/
bool rmatrixtrinverse(ap::real_2d_array& a,
     int n,
     bool isupper,
     bool isunittriangular)
{
    bool result;
    bool nounit;
    int i;
    int j;
    double v;
    double ajj;
    ap::real_1d_array t;

    result = true;
    t.setbounds(0, n-1);
    
    //
    // Test the input parameters.
    //
    nounit = !isunittriangular;
    if( isupper )
    {
        
        //
        // Compute inverse of upper triangular matrix.
        //
        for(j = 0; j <= n-1; j++)
        {
            if( nounit )
            {
                if( a(j,j)==0 )
                {
                    result = false;
                    return result;
                }
                a(j,j) = 1/a(j,j);
                ajj = -a(j,j);
            }
            else
            {
                ajj = -1;
            }
            
            //
            // Compute elements 1:j-1 of j-th column.
            //
            if( j>0 )
            {
                ap::vmove(t.getvector(0, j-1), a.getcolumn(j, 0, j-1));
                for(i = 0; i <= j-1; i++)
                {
                    if( i<j-1 )
                    {
                        v = ap::vdotproduct(&a(i, i+1), &t(i+1), ap::vlen(i+1,j-1));
                    }
                    else
                    {
                        v = 0;
                    }
                    if( nounit )
                    {
                        a(i,j) = v+a(i,i)*t(i);
                    }
                    else
                    {
                        a(i,j) = v+t(i);
                    }
                }
                ap::vmul(a.getcolumn(j, 0, j-1), ajj);
            }
        }
    }
    else
    {
        
        //
        // Compute inverse of lower triangular matrix.
        //
        for(j = n-1; j >= 0; j--)
        {
            if( nounit )
            {
                if( a(j,j)==0 )
                {
                    result = false;
                    return result;
                }
                a(j,j) = 1/a(j,j);
                ajj = -a(j,j);
            }
            else
            {
                ajj = -1;
            }
            if( j<n-1 )
            {
                
                //
                // Compute elements j+1:n of j-th column.
                //
                ap::vmove(t.getvector(j+1, n-1), a.getcolumn(j, j+1, n-1));
                for(i = j+1; i <= n-1; i++)
                {
                    if( i>j+1 )
                    {
                        v = ap::vdotproduct(&a(i, j+1), &t(j+1), ap::vlen(j+1,i-1));
                    }
                    else
                    {
                        v = 0;
                    }
                    if( nounit )
                    {
                        a(i,j) = v+a(i,i)*t(i);
                    }
                    else
                    {
                        a(i,j) = v+t(i);
                    }
                }
                ap::vmul(a.getcolumn(j, j+1, n-1), ajj);
            }
        }
    }
    return result;
}


/*************************************************************************
Obsolete 1-based subroutine.
See RMatrixTRInverse for 0-based replacement.
*************************************************************************/
bool invtriangular(ap::real_2d_array& a,
     int n,
     bool isupper,
     bool isunittriangular)
{
    bool result;
    bool nounit;
    int i;
    int j;
    int nmj;
    int jm1;
    int jp1;
    double v;
    double ajj;
    ap::real_1d_array t;

    result = true;
    t.setbounds(1, n);
    
    //
    // Test the input parameters.
    //
    nounit = !isunittriangular;
    if( isupper )
    {
        
        //
        // Compute inverse of upper triangular matrix.
        //
        for(j = 1; j <= n; j++)
        {
            if( nounit )
            {
                if( a(j,j)==0 )
                {
                    result = false;
                    return result;
                }
                a(j,j) = 1/a(j,j);
                ajj = -a(j,j);
            }
            else
            {
                ajj = -1;
            }
            
            //
            // Compute elements 1:j-1 of j-th column.
            //
            if( j>1 )
            {
                jm1 = j-1;
                ap::vmove(t.getvector(1, jm1), a.getcolumn(j, 1, jm1));
                for(i = 1; i <= j-1; i++)
                {
                    if( i<j-1 )
                    {
                        v = ap::vdotproduct(&a(i, i+1), &t(i+1), ap::vlen(i+1,jm1));
                    }
                    else
                    {
                        v = 0;
                    }
                    if( nounit )
                    {
                        a(i,j) = v+a(i,i)*t(i);
                    }
                    else
                    {
                        a(i,j) = v+t(i);
                    }
                }
                ap::vmul(a.getcolumn(j, 1, jm1), ajj);
            }
        }
    }
    else
    {
        
        //
        // Compute inverse of lower triangular matrix.
        //
        for(j = n; j >= 1; j--)
        {
            if( nounit )
            {
                if( a(j,j)==0 )
                {
                    result = false;
                    return result;
                }
                a(j,j) = 1/a(j,j);
                ajj = -a(j,j);
            }
            else
            {
                ajj = -1;
            }
            if( j<n )
            {
                
                //
                // Compute elements j+1:n of j-th column.
                //
                nmj = n-j;
                jp1 = j+1;
                ap::vmove(t.getvector(jp1, n), a.getcolumn(j, jp1, n));
                for(i = j+1; i <= n; i++)
                {
                    if( i>j+1 )
                    {
                        v = ap::vdotproduct(&a(i, jp1), &t(jp1), ap::vlen(jp1,i-1));
                    }
                    else
                    {
                        v = 0;
                    }
                    if( nounit )
                    {
                        a(i,j) = v+a(i,i)*t(i);
                    }
                    else
                    {
                        a(i,j) = v+t(i);
                    }
                }
                ap::vmul(a.getcolumn(j, jp1, n), ajj);
            }
        }
    }
    return result;
}



