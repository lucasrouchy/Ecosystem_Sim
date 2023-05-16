#include <stdio.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

unsigned int seed = 0;

int	NowYear;		// 2023 - 2028
int	NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// rye grass height in inches
int	NowNumRabbits;		// number of rabbits in the current population
int NowNumCoyotes;		// number of coyotes in the current population

const float RYEGRASS_GROWS_PER_MONTH =		20.0;
const float ONE_RABBITS_EATS_PER_MONTH =	 1.0;

const float AVG_PRECIP_PER_MONTH =	       12.0;	// average
const float AMP_PRECIP_PER_MONTH =		4.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				60.0;
const float MIDPRECIP =				14.0;

float Ranf( unsigned int *seedp,  float low, float high ) 
{
		float r = (float) rand_r( seedp ) / (float)RAND_MAX ;              // 0 - RAND_MAX

		return(   low  +  r * ( high - low )   );
}


void InitBarrier( int n )
{
        NumInThreadTeam = n;
        NumAtBarrier = 0;
	omp_init_lock( &Lock );
}

void WaitBarrier( )
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
					NumGone = 0;
					NumAtBarrier = 0;
					
					while( NumGone != NumInThreadTeam-1 );
					omp_unset_lock( &Lock );
					return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}

float Sqr( float x )
{
        return x*x;
}

void Rabbits( )
{
    while( NowYear < 2029 )
    {
        WaitBarrier( );
		int nextNumRabbits = NowNumRabbits;
		int carryingCapacity = (int)(NowHeight);
		// int diff = carryingCapacity - NowNumRabbits;
		// int change = (diff > 0) ? 1 : -1;
		
		WaitBarrier( );
		if (nextNumRabbits < carryingCapacity)
            nextNumRabbits++;
        else if (nextNumRabbits > carryingCapacity)
            nextNumRabbits--;
		WaitBarrier( );
		if (NowNumCoyotes > NowNumRabbits/3)
			nextNumRabbits--;
		else if (NowNumCoyotes < NowNumRabbits/6)
			nextNumRabbits++;
		
		WaitBarrier( );
        NowNumRabbits = nextNumRabbits;
		if( NowNumRabbits < 0 ) NowNumRabbits = 0;
		
    }
}

void RyeGrass()
{	
	
	while( NowYear < 2029 )
	{	
		float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );
		float temp = AVG_TEMP - AMP_TEMP * cos( ang );

		NowTemp = temp + Ranf( &seed, -RANDOM_TEMP, RANDOM_TEMP );

		float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
		
		NowPrecip = precip + Ranf( &seed,  -RANDOM_PRECIP, RANDOM_PRECIP );
		if( NowPrecip < 0. ) NowPrecip = 0.;

		float tempFactor = exp(   -Sqr(  ( NowTemp - MIDTEMP ) / 10.  )   );
		float precipFactor = exp(   -Sqr(  ( NowPrecip - MIDPRECIP ) / 10.  )   );
		
		WaitBarrier( );
		float nextHeight = NowHeight;
		
		WaitBarrier( );
		nextHeight += tempFactor * precipFactor * RYEGRASS_GROWS_PER_MONTH;
		
		nextHeight -= (float)NowNumRabbits * ONE_RABBITS_EATS_PER_MONTH;

		
		WaitBarrier( );
		if( nextHeight < 0. ) nextHeight = 0.;
		NowHeight = nextHeight;
		
	}	
}

void Watcher() 
{
	while (NowYear < 2029) {
		//increment the month
		WaitBarrier();
		NowMonth++;
		WaitBarrier();
		if (NowMonth == 12) {
			
			NowYear++;
			NowMonth = 0;
		}
		WaitBarrier();
		printf("%d,%d,%d,%d,%f,%f,%f\n", NowMonth, NowYear, NowNumRabbits, NowNumCoyotes, NowHeight, NowTemp, NowPrecip);
	
	}    
}

void Coyotes()
{
	while (NowYear < 2029){
		WaitBarrier();
		int nextNumCoyotes = NowNumCoyotes;
		int carryingCapacity = (int)(NowNumRabbits);
		WaitBarrier();
		if (NowNumRabbits > 10){
			nextNumCoyotes++;
		}
		else if (NowNumCoyotes > NowNumRabbits/6){
			nextNumCoyotes--;
		}
		WaitBarrier();
		NowNumCoyotes = nextNumCoyotes;
		if (NowNumCoyotes < 0) NowNumCoyotes = 0;		
	}
}

int main(){
		// starting date and time:
	NowMonth =    0;
	NowYear  = 2023;
	
	float x = Ranf( &seed, -1.f, 1.f );
	// starting state:
	NowNumRabbits = 8;
	NowHeight =  40;
	NowNumCoyotes = 1;

	omp_set_num_threads( 4 );
	InitBarrier( 4);
	
	#pragma omp parallel sections
	{
		#pragma omp section
		{
			Rabbits( );
		}

		#pragma omp section
		{
			RyeGrass();
		}

		#pragma omp section
		{
			Watcher( );
		}

		#pragma omp section
		{
			Coyotes( );	
		}
	}
	
	return 0;	
}

