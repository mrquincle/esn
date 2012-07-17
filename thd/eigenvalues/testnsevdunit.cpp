

#include <stdio.h>
#include "testnsevdunit.h"

static void fillsparsea(ap::real_2d_array& a, int n, double sparcity);
static void testnsevdproblem(const ap::real_2d_array& a,
     int n,
     double& vecerr,
     double& valonlydiff,
     bool& wfailed);

bool testnonsymmetricevd(bool silent)
{
    bool result;
    ap::real_2d_array a;
    int n;
    int i;
    int j;
    int gpass;
    bool waserrors;
    bool wfailed;
    double vecerr;
    double valonlydiff;
    double threshold;

    vecerr = 0;
    valonlydiff = 0;
    wfailed = false;
    waserrors = false;
    threshold = 100*ap::machineepsilon;

    //
    // First set: N = 1..10
    //
    for(n = 1; n <= 10; n++)
    {
        a.setbounds(0, n-1, 0, n-1);

        //
        // zero matrix
        //
        for(i = 0; i <= n-1; i++)
        {
            for(j = 0; j <= n-1; j++)
            {
                a(i,j) = 0;
            }
        }
        testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

        //
        // Dense and sparse matrices
        //
        for(gpass = 1; gpass <= 1; gpass++)
        {

            //
            // Dense matrix
            //
            for(i = 0; i <= n-1; i++)
            {
                for(j = 0; j <= n-1; j++)
                {
                    a(i,j) = 2*ap::randomreal()-1;
                }
            }
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

            //
            // Very matrix
            //
            fillsparsea(a, n, 0.98);
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

            //
            // Incredible sparse matrix
            //
            fillsparsea(a, n, 0.995);
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);
        }
    }

    //
    // Second set: N = 70..72
    //
    for(n = 70; n <= 72; n++)
    {
        a.setbounds(0, n-1, 0, n-1);

        //
        // zero matrix
        //
        for(i = 0; i <= n-1; i++)
        {
            for(j = 0; j <= n-1; j++)
            {
                a(i,j) = 0;
            }
        }
        testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

        //
        // Dense and sparse matrices
        //
        for(gpass = 1; gpass <= 1; gpass++)
        {

            //
            // Dense matrix
            //
            for(i = 0; i <= n-1; i++)
            {
                for(j = 0; j <= n-1; j++)
                {
                    a(i,j) = 2*ap::randomreal()-1;
                }
            }
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

            //
            // Very matrix
            //
            fillsparsea(a, n, 0.98);
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);

            //
            // Incredible sparse matrix
            //
            fillsparsea(a, n, 0.995);
            testnsevdproblem(a, n, vecerr, valonlydiff, wfailed);
        }
    }

    //
    // report
    //
    waserrors = valonlydiff>1000*threshold||vecerr>threshold||wfailed;
    if( !silent )
    {
        printf("TESTING NONSYMMETTRIC EVD\n");
        printf("Av-lambdav error:                        %5.3le\n",
            double(vecerr));
        printf("Values only difference:                  %5.3le\n",
            double(valonlydiff));
        printf("Always converged:                        ");
        if( !wfailed )
        {
            printf("YES\n");
        }
        else
        {
            printf("NO\n");
        }
        printf("Threshold:                               %5.3le\n",
            double(threshold));
        if( waserrors )
        {
            printf("TEST FAILED\n");
        }
        else
        {
            printf("TEST PASSED\n");
        }
        printf("\n\n");
    }
    result = !waserrors;
    return result;
}


static void fillsparsea(ap::real_2d_array& a, int n, double sparcity)
{
    int i;
    int j;

    for(i = 0; i <= n-1; i++)
    {
        for(j = 0; j <= n-1; j++)
        {
            if( ap::randomreal()>=sparcity )
            {
                a(i,j) = 2*ap::randomreal()-1;
            }
            else
            {
                a(i,j) = 0;
            }
        }
    }
}


static void testnsevdproblem(const ap::real_2d_array& a,
     int n,
     double& vecerr,
     double& valonlydiff,
     bool& wfailed)
{
    double mx;
    int i;
    int j;
    int k;
    int vjob;
    bool needl;
    bool needr;
    ap::real_1d_array wr0;
    ap::real_1d_array wi0;
    ap::real_1d_array wr1;
    ap::real_1d_array wi1;
    ap::real_1d_array wr0s;
    ap::real_1d_array wi0s;
    ap::real_1d_array wr1s;
    ap::real_1d_array wi1s;
    ap::real_2d_array vl;
    ap::real_2d_array vr;
    ap::real_1d_array vec1r;
    ap::real_1d_array vec1i;
    ap::real_1d_array vec2r;
    ap::real_1d_array vec2i;
    ap::real_1d_array vec3r;
    ap::real_1d_array vec3i;
    double curwr;
    double curwi;
    double vt;
    double tmp;

    vec1r.setbounds(0, n-1);
    vec2r.setbounds(0, n-1);
    vec3r.setbounds(0, n-1);
    vec1i.setbounds(0, n-1);
    vec2i.setbounds(0, n-1);
    vec3i.setbounds(0, n-1);
    wr0s.setbounds(0, n-1);
    wr1s.setbounds(0, n-1);
    wi0s.setbounds(0, n-1);
    wi1s.setbounds(0, n-1);
    mx = 0;
    for(i = 0; i <= n-1; i++)
    {
        for(j = 0; j <= n-1; j++)
        {
            if( fabs(a(i,j))>mx )
            {
                mx = fabs(a(i,j));
            }
        }
    }
    if( mx==0 )
    {
        mx = 1;
    }

    //
    // Load values-only
    //
    if( !rmatrixevd(a, n, 0, wr0, wi0, vl, vr) )
    {
        wfailed = false;
        return;
    }

    //
    // Test different jobs
    //
    for(vjob = 1; vjob <= 3; vjob++)
    {
        needr = vjob==1||vjob==3;
        needl = vjob==2||vjob==3;
        if( !rmatrixevd(a, n, vjob, wr1, wi1, vl, vr) )
        {
            wfailed = false;
            return;
        }

        //
        // Test values:
        // 1. sort by real part
        // 2. test
        //
        ap::vmove(&wr0s(0), &wr0(0), ap::vlen(0,n-1));
        ap::vmove(&wi0s(0), &wi0(0), ap::vlen(0,n-1));
        for(i = 0; i <= n-1; i++)
        {
            for(j = 0; j <= n-2-i; j++)
            {
                if( wr0s(j)>wr0s(j+1) )
                {
                    tmp = wr0s(j);
                    wr0s(j) = wr0s(j+1);
                    wr0s(j+1) = tmp;
                    tmp = wi0s(j);
                    wi0s(j) = wi0s(j+1);
                    wi0s(j+1) = tmp;
                }
            }
        }
        ap::vmove(&wr1s(0), &wr1(0), ap::vlen(0,n-1));
        ap::vmove(&wi1s(0), &wi1(0), ap::vlen(0,n-1));
        for(i = 0; i <= n-1; i++)
        {
            for(j = 0; j <= n-2-i; j++)
            {
                if( wr1s(j)>wr1s(j+1) )
                {
                    tmp = wr1s(j);
                    wr1s(j) = wr1s(j+1);
                    wr1s(j+1) = tmp;
                    tmp = wi1s(j);
                    wi1s(j) = wi1s(j+1);
                    wi1s(j+1) = tmp;
                }
            }
        }
        for(i = 0; i <= n-1; i++)
        {
            valonlydiff = ap::maxreal(valonlydiff, fabs(wr0s(i)-wr1s(i)));
            valonlydiff = ap::maxreal(valonlydiff, fabs(wi0s(i)-wi1s(i)));
        }

        //
        // Test right vectors
        //
        if( needr )
        {
            k = 0;
            while(k<=n-1)
            {
                if( wi1(k)==0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vr.getcolumn(k, 0, n-1));
                    for(i = 0; i <= n-1; i++)
                    {
                        vec1i(i) = 0;
                    }
                    curwr = wr1(k);
                    curwi = 0;
                }
                if( wi1(k)>0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vr.getcolumn(k, 0, n-1));
                    ap::vmove(vec1i.getvector(0, n-1), vr.getcolumn(k+1, 0, n-1));
                    curwr = wr1(k);
                    curwi = wi1(k);
                }
                if( wi1(k)<0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vr.getcolumn(k-1, 0, n-1));
                    ap::vmoveneg(vec1i.getvector(0, n-1), vr.getcolumn(k, 0, n-1));
                    curwr = wr1(k);
                    curwi = wi1(k);
                }
                for(i = 0; i <= n-1; i++)
                {
                    vt = ap::vdotproduct(&a(i, 0), &vec1r(0), ap::vlen(0,n-1));
                    vec2r(i) = vt;
                    vt = ap::vdotproduct(&a(i, 0), &vec1i(0), ap::vlen(0,n-1));
                    vec2i(i) = vt;
                }
                ap::vmove(&vec3r(0), &vec1r(0), ap::vlen(0,n-1), curwr);
                ap::vsub(&vec3r(0), &vec1i(0), ap::vlen(0,n-1), curwi);
                ap::vmove(&vec3i(0), &vec1r(0), ap::vlen(0,n-1), curwi);
                ap::vadd(&vec3i(0), &vec1i(0), ap::vlen(0,n-1), curwr);
                for(i = 0; i <= n-1; i++)
                {
                    vecerr = ap::maxreal(vecerr, fabs(vec2r(i)-vec3r(i)));
                    vecerr = ap::maxreal(vecerr, fabs(vec2i(i)-vec3i(i)));
                }
                k = k+1;
            }
        }

        //
        // Test left vectors
        //
        if( needl )
        {
            k = 0;
            while(k<=n-1)
            {
                if( wi1(k)==0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vl.getcolumn(k, 0, n-1));
                    for(i = 0; i <= n-1; i++)
                    {
                        vec1i(i) = 0;
                    }
                    curwr = wr1(k);
                    curwi = 0;
                }
                if( wi1(k)>0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vl.getcolumn(k, 0, n-1));
                    ap::vmove(vec1i.getvector(0, n-1), vl.getcolumn(k+1, 0, n-1));
                    curwr = wr1(k);
                    curwi = wi1(k);
                }
                if( wi1(k)<0 )
                {
                    ap::vmove(vec1r.getvector(0, n-1), vl.getcolumn(k-1, 0, n-1));
                    ap::vmoveneg(vec1i.getvector(0, n-1), vl.getcolumn(k, 0, n-1));
                    curwr = wr1(k);
                    curwi = wi1(k);
                }
                for(j = 0; j <= n-1; j++)
                {
                    vt = ap::vdotproduct(vec1r.getvector(0, n-1), a.getcolumn(j, 0, n-1));
                    vec2r(j) = vt;
                    vt = ap::vdotproduct(vec1i.getvector(0, n-1), a.getcolumn(j, 0, n-1));
                    vec2i(j) = -vt;
                }
                ap::vmove(&vec3r(0), &vec1r(0), ap::vlen(0,n-1), curwr);
                ap::vadd(&vec3r(0), &vec1i(0), ap::vlen(0,n-1), curwi);
                ap::vmove(&vec3i(0), &vec1r(0), ap::vlen(0,n-1), curwi);
                ap::vsub(&vec3i(0), &vec1i(0), ap::vlen(0,n-1), curwr);
                for(i = 0; i <= n-1; i++)
                {
                    vecerr = ap::maxreal(vecerr, fabs(vec2r(i)-vec3r(i)));
                    vecerr = ap::maxreal(vecerr, fabs(vec2i(i)-vec3i(i)));
                }
                k = k+1;
            }
        }
    }
}


/*************************************************************************
Silent unit test
*************************************************************************/
bool testnsevdunit_test_silent()
{
    bool result;

    result = testnonsymmetricevd(true);
    return result;
}


/*************************************************************************
Unit test
*************************************************************************/
bool testnsevdunit_test()
{
    bool result;

    result = testnonsymmetricevd(false);
    return result;
}



