#pragma once

#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <math.h>

#ifdef USE_SSE_INSTRUCTIONS
#include <Math/sse_block.h>
#endif

template<class FLOAT> class DaubechyWavelets
{
	public:
		typedef enum { DWT_DAUB02=2, DWT_DAUB04=4, DWT_DAUB12=12, DWT_DAUB20=20 } WaveletType ;
		typedef enum { DWT_FORWARD=1, DWT_BACKWARD=0 } TransformType ;

		static void DWT2D(FLOAT *data,unsigned long int W,unsigned long int H,WaveletType type,TransformType tr)
		{
			unsigned long int nn[2] = {W,H} ;
			wtn(&data[-1], &nn[-1],2, tr, waveletFilter(type), pwt) ;
		}
		static void DWT1D(FLOAT *data,unsigned long int W,WaveletType type,TransformType tr)
		{
			unsigned long int nn[1] = {W} ;
			wtn(&data[-1], &nn[-1],1, tr, waveletFilter(type), pwt) ;
		}


	private:
		class wavefilt
		{
			public:
				wavefilt(int n)
				{
					int k;
					FLOAT sig = -1.0;
					static const FLOAT c2[5]={  0.0, sqrt(2.0)/2.0, sqrt(2.0)/2.0, 0.0, 0.0 };

					static const FLOAT c4[5]={  0.0, 0.4829629131445341, 0.8365163037378079, 0.2241438680420134,-0.1294095225512604 };

					static const FLOAT c12[13]={0.0,0.111540743350, 0.494623890398, 0.751133908021,
						0.315250351709,-0.226264693965,-0.129766867567,
						0.097501605587, 0.027522865530,-0.031582039318,
						0.000553842201, 0.004777257511,-0.001077301085};

					static const FLOAT c20[21]={0.0,0.026670057901, 0.188176800078, 0.527201188932,
						0.688459039454, 0.281172343661,-0.249846424327,
						-0.195946274377, 0.127369340336, 0.093057364604,
						-0.071394147166,-0.029457536822, 0.033212674059,
						0.003606553567,-0.010733175483, 0.001395351747,
						0.001992405295,-0.000685856695,-0.000116466855,
						0.000093588670,-0.000013264203 };

					ncof= (n==2)?4:n;
					const FLOAT *tmpcc ;
					cc.resize(ncof+1) ;
					cr.resize(ncof+1) ;

					if (n == 2) 
					{
						tmpcc=c2;
						cc[1] = tmpcc[1] ;
						cc[2] = tmpcc[2] ;
						cc[3] = 0.0f ;
						cc[4] = 0.0f ;
						cr[1] = tmpcc[1] ;
						cr[2] =-tmpcc[2] ;
						cr[3] = 0.0f ;
						cr[4] = 0.0f ;

						ioff = joff = -1 ;
					}
					else 
					{
						if (n == 4) 
							tmpcc=c4;
						else if (n == 12) 
							tmpcc=c12;
						else if (n == 20) 
							tmpcc=c20;
						else
							throw std::runtime_error("unimplemented value n in pwtset");

						for (k=1;k<=n;k++) 
						{
							cc[k] = tmpcc[k] ;
							cr[ncof+1-k]=sig*tmpcc[k];
							sig = -sig;
						}
						ioff = joff = -(n >> 1);
					}
				}

				~wavefilt() {}

				int ncof,ioff,joff;
				std::vector<FLOAT> cc;
				std::vector<FLOAT> cr;
		} ;

		static const wavefilt& waveletFilter(WaveletType type)
		{
			static wavefilt *daub02filt = NULL ;
			static wavefilt *daub04filt = NULL ;
			static wavefilt *daub12filt = NULL ;
			static wavefilt *daub20filt = NULL ;

			switch(type)
			{
				case DWT_DAUB02: if(daub02filt == NULL)
										  daub02filt = new wavefilt(2) ;
									  return *daub02filt ;

				case DWT_DAUB04: if(daub04filt == NULL)
										  daub04filt = new wavefilt(4) ;
									  return *daub04filt ;

				case DWT_DAUB12: if(daub12filt == NULL)
										  daub12filt = new wavefilt(12) ;
									  return *daub12filt ;

				case DWT_DAUB20: if(daub20filt == NULL)
										  daub20filt = new wavefilt(20) ;
									  return *daub20filt ;

				default:
								 throw std::runtime_error("Unknown wavelet type.") ;
			}
		}

	static void pwt(FLOAT a[], unsigned long n, int isign,const wavefilt& wfilt)
	{
/********************** BEGIN SIGNED PART *************************/
/**         md5sum = 2b9e1e38ac690f50806873cdb4a061ea            **/
/**         Validation date = 08/10/10                           **/
/******************************************************************/
		unsigned long i,ii,ni,nj ;

		if (n < 4) 
			return;

		FLOAT *wksp=new FLOAT[n+1];//vector(1,n);
		FLOAT ai,ai1 ;
		unsigned long int nmod=wfilt.ncof*n;
		unsigned long int n1=n-1;
		unsigned long int nh=n >> 1;

		memset(wksp,0,(n+1)*sizeof(FLOAT)) ;

		if (isign == DWT_FORWARD)
			for (ii=1,i=1;i<=n;i+=2,ii++) 
			{
				ni=i+nmod+wfilt.ioff;
				nj=i+nmod+wfilt.joff;

#ifdef USE_SSE_INSTRUCTIONS
#warning Using SSE2 Instruction set for wavelet internal loops
				for (int k=1;k<=wfilt.ncof;k+=4) 
				{
					int jf=ni+k;
					int jr=nj+k;

					sse_block w1(wfilt.cc[k],wfilt.cc[k+1],wfilt.cc[k+2],wfilt.cc[k+3]) ;
					sse_block w2(wfilt.cr[k],wfilt.cr[k+1],wfilt.cr[k+2],wfilt.cr[k+3]) ;

					sse_block a1( a[1+((jf+0)&n1)], a[1+((jf+1)&n1)], a[1+((jf+2)&n1)], a[1+((jf+3)&n1)]) ;
					sse_block a2( a[1+((jr+0)&n1)], a[1+((jr+1)&n1)], a[1+((jr+2)&n1)], a[1+((jr+3)&n1)]) ;

					sse_block wk1( w1*a1 ) ;
					sse_block wk2( w2*a2 ) ;

					wksp[ii   ] += wk1.sum() ;
					wksp[ii+nh] += wk2.sum() ;
				}
#else
				for (int k=1;k<=wfilt.ncof;k++) 
				{
					int jf=n1 & (ni+k);
					int jr=n1 & (nj+k);
					wksp[ii] 	+= wfilt.cc[k]*a[jf+1];
					wksp[ii+nh] += wfilt.cr[k]*a[jr+1];
				}
#endif
			}
		else 
			for (ii=1,i=1;i<=n;i+=2,ii++) 
			{
				ai=a[ii];
				ai1=a[ii+nh];
				ni=i+nmod+wfilt.ioff;
				nj=i+nmod+wfilt.joff;

#ifdef USE_SSE_INSTRUCTIONS
				sse_block ai_sse( ai,ai,ai,ai ) ;
				sse_block ai1_sse( ai1,ai1,ai1,ai1 ) ;

				for (int k=1;k<=wfilt.ncof;k+=4) 
				{
					int jf=ni+k ;
					int jr=nj+k ;	// in fact we have jf==jr, so the code is simpler.

					sse_block w1(wksp[1+((jf+0) & n1)],wksp[1+((jf+1) & n1)],wksp[1+((jf+2) & n1)],wksp[1+((jf+3) & n1)]) ;

					w1 += sse_block(wfilt.cc[k+0],wfilt.cc[k+1],wfilt.cc[k+2],wfilt.cc[k+3]) * ai_sse ;
					w1 += sse_block(wfilt.cr[k+0],wfilt.cr[k+1],wfilt.cr[k+2],wfilt.cr[k+3]) * ai1_sse ;

					wksp[1+((jr+0) & n1)] = w1[0] ;
					wksp[1+((jr+1) & n1)] = w1[1] ;
					wksp[1+((jr+2) & n1)] = w1[2] ;
					wksp[1+((jr+3) & n1)] = w1[3] ;
				}
#else
				for (int k=1;k<=wfilt.ncof;++k) 
				{
					wksp[(n1 & (ni+k))+1] += wfilt.cc[k]*ai;
					wksp[(n1 & (nj+k))+1] += wfilt.cr[k]*ai1;
				}
#endif
			}

		for (uint j=1;j<=n;j++)
			a[j]=wksp[j];

		delete[] wksp ;//free_vector(wksp,1,n);
/********************** END SIGNED PART *************************/
	}

	static void wtn(FLOAT a[], unsigned long nn[], int ndim, int isign, const wavefilt& w,void (*wtstep)(FLOAT [], unsigned long, int,const wavefilt&))
	{
		unsigned long i1,i2,i3,k,n,nnew,nprev=1,nt,ntot=1;
		int idim;
		FLOAT *wksp;

		for (idim=1;idim<=ndim;idim++)  
			ntot *= nn[idim];

		wksp=new FLOAT[ntot+1] ; //vector(1,ntot);

		for (idim=1;idim<=ndim;idim++) 
		{
			n=nn[idim];
			nnew=n*nprev;

			if (n > 4) 
				for (i2=0;i2<ntot;i2+=nnew) 
					for (i1=1;i1<=nprev;i1++) 
					{
						for (i3=i1+i2,k=1;k<=n;k++,i3+=nprev) wksp[k]=a[i3];

						if(isign == DWT_FORWARD)
							for(nt=n;nt>=4;nt >>= 1)
								(*wtstep)(wksp,nt,isign,w);
						else 
							for(nt=4;nt<=n;nt <<= 1)
								(*wtstep)(wksp,nt,isign,w);

						for (i3=i1+i2,k=1;k<=n;k++,i3+=nprev) a[i3]=wksp[k];
					}

			nprev=nnew;
		}
		delete[] wksp ;//free_vector(wksp,1,ntot);
	}
};

