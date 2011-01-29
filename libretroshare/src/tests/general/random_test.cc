#ifdef LINUX
#include <fenv.h>
#endif
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include "util/utest.h"
#include "util/rsrandom.h"

INITTEST();
typedef double (* Ifctn)( double t);
/* Numerical integration method */
double Simpson3_8( Ifctn f, double a, double b, int N)
{
    int j;
    double l1;
    double h = (b-a)/N;
    double h1 = h/3.0;
    double sum = f(a) + f(b);
 
    for (j=3*N-1; j>0; j--) {
        l1 = (j%3)? 3.0 : 2.0;
        sum += l1*f(a+h1*j) ;
    }
    return h*sum/8.0;
}
 
#define A 12
double Gamma_Spouge( double z )
{
    int k;
    static double cspace[A];
    static double *coefs = NULL;
    double accum;
    double a = A;
 
    if (!coefs) {
        double k1_factrl = 1.0;
        coefs = cspace;
        coefs[0] = sqrt(2.0*M_PI);
        for(k=1; k<A; k++) {
            coefs[k] = exp(a-k) * pow(a-k,k-0.5) / k1_factrl;
            k1_factrl *= -k;
        }
    }
 
    accum = coefs[0];
    for (k=1; k<A; k++) {
        accum += coefs[k]/(z+k);
    }
    accum *= exp(-(z+a)) * pow(z+a, z+0.5);
    return accum/z;
}
 
double aa1;
double f0( double t)
{
	return  pow(t, aa1)*exp(-t); 
}
 
double GammaIncomplete_Q( double a, double x)
{
    double y, h = 1.5e-2;  /* approximate integration step size */
 
    /* this cuts off the tail of the integration to speed things up */
    y = aa1 = a-1;
    while((f0(y) * (x-y) > 2.0e-8) && (y < x))   y += .4;
    if (y>x) y=x;
 
    return 1.0 - Simpson3_8( &f0, 0, y, std::max(5,(int)(y/h)))/Gamma_Spouge(a);
}

double chi2Probability( int dof, double distance)
{
    return GammaIncomplete_Q( 0.5*dof, 0.5*distance);
}

class myThread: public RsThread
{
	public:
		myThread()
		{
			_finished = false ;
		}
		virtual void run()
		{
			// test that random numbers are regularly disposed
			//
			int N = 500 ;
			int B = 8 ;

			std::vector<int> buckets(B,0) ;

			for(int i=0;i<N;++i)
			{
				float f = RSRandom::random_f32() ;
				int b = (int)floor(f*B*0.99999999) ;
				++buckets[b] ;
			}

			// Chi2 test
			//
			float chi2 = 0.0f ;
			float expected = 1.0/(float)B ;

			for(int k=0;k<B;++k)
				chi2 += pow( buckets[k]/float(N) - expected,2)/expected ;

			double chi2prob = chi2Probability(B-1,chi2) ;
			double significance = 0.05 ;

			std::cerr << "Compariing chi2 uniform distance " << chi2 << " to chi2 probability " << chi2prob ;

			if(chi2prob > significance)
				std::cerr << ": passed" << std::endl ;
			else
				std::cerr << ": failed" << std::endl ;

			_finished = true ;
		}

		bool finished() const { return _finished ; }
	private:
		bool _finished ;
};


int main(int argc, char **argv)
{
#ifdef LINUX
	feenableexcept(FE_INVALID) ;
	feenableexcept(FE_DIVBYZERO) ;
#endif
	std::cerr << "Generating random 64 chars string (run that again to test that it's changing): " << RSRandom::random_alphaNumericString(64) << std::endl;
	int nt = 10 ;	// number of threads.
	std::vector<myThread *> threads(nt,(myThread*)NULL) ;

	for(int i=0;i<nt;++i)
	{
		threads[i] = new myThread ;
		threads[i]->start() ;
	}

	while(true)
	{
		bool finished = true ;

		for(int i=0;i<nt;++i)
			if(!threads[i]->finished())
				finished = false ;

		if(finished)
			break ;
	}
	for(int i=0;i<nt;++i)
		delete threads[i] ;

	FINALREPORT("random_test");
	exit(TESTRESULT());
}

