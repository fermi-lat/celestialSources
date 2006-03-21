// FILE: GrbGlobalData.cxx



#include <algorithm>              // for transform
#include <numeric>                // for accumulate

#include "GrbGlobalData.h"
#include "GRBobsUtilities.h"
#include "GRBobsConstants.h"
#include "GRBsimvecCreator.h"
#include "CLHEP/Random/RandFlat.h"

const double logEcen=log10(240.);
const double logE=log10(1.75);


using namespace grbobstypes;


// static declaration
GrbGlobalData	*GrbGlobalData::s_instance = 0;



// default constructor
GrbGlobalData::GrbGlobalData(CLHEP::HepRandomEngine *engine) 
{
    grbGlobal(engine);	
}




// destructor
GrbGlobalData::~GrbGlobalData ()
{}




// instance()
// Returns the singleton instance of the class
GrbGlobalData	*GrbGlobalData::instance(CLHEP::HepRandomEngine *engine) 
{ 
    return (s_instance != 0) ? s_instance : (s_instance = new GrbGlobalData(engine)); 
}




// kill()
// deletes and releases memory associated with the instance of this class
void GrbGlobalData::kill()
{
    delete s_instance;
    s_instance = 0;
}




// grbGlobal(engine)
// Generates samples from global distributions for GRBs: duration, peak flux and spectral power-law index.
void GrbGlobalData::grbGlobal(CLHEP::HepRandomEngine *engine)
{
    long nlong = 0L;
    getDuration(engine, nlong);
    
    getFlux(engine, nlong);
    
    getPowerLawIndex(engine);
    
    // free up memory allocated to install data for the calculations of duration, flux and power law indices
    GRBsimvecCreator::kill();	
}




// index(HepRandomEngine *engine, const long diff, const long minval, const std::vector<long> &in)
// returns the index i to the last element of "in" vector such that in[i] < some random value.
long GrbGlobalData::index(CLHEP::HepRandomEngine *engine, const long diff, 
                          const long minval, const std::vector<long> &in) const
{
    bool found=0;
    long i;
    
    while (!found)
    {
        // use random number to generate a threshold value
        const long value = long(engine->flat() * diff + minval);
        
        // pick last index i into "in" such that in[i] < value
        for (i=in.size()-1; i>=0 && !found; --i)
            if (in[i] < value)
                found = 1;
    }
    
    return i+1;
}




// evaluate(HepRandomEngine *engine, const long diff, const long minval, const std::vector<long> &in,
//			const std::vector<double> &v)
// Picks hi and lo values from the vector "v" and interpolates these to generate the return value.
double GrbGlobalData::evaluate(CLHEP::HepRandomEngine *engine, const long diff, 
                               const long minval, const std::vector<long> &in,
                               const std::vector<double> &v) const
{
    // find index loIndex such that in[loIndex] < some random number
    long loIndex = index(engine, diff, minval, in);
    
    double value_lo = v[loIndex+1];
    double value_hi = v[loIndex+2];
    
    return value_lo + engine->flat()*(value_hi - value_lo);
}




// computeFlux(HepRandomEngine *engine, const long diff, const long minval, const std::vector<long> &in,
//			   const std::vector<double> &v)
// Picks hi and lo values from the vector "v" and uses the method "result" to interpolate these 
// to generate the return value.
double GrbGlobalData::computeFlux(CLHEP::HepRandomEngine *engine, const long diff, 
                                  const long minval, const std::vector<long> &in,
                                  const std::vector<double> &v) const
{
    // find index loIndex such that in[loIndex] < some random number
    long loIndex = index(engine, diff, minval, in);
    
    double flux_hi = v[loIndex];
    double flux_lo = v[loIndex+1];
    
    return GRBobsUtilities::result(engine, flux_lo, flux_hi, grbcst::alpha);
}




// getDuration(HepRandomEngine *engine, long &nlong)
//		Chooses durations from the BATSE bimodal duration distribution, where the measurement process is described by 
//		Bonnell et al. (1997, ApJ, 490, 79).  The parent sample is same as for peak fluxes: from GRB 910421 (trig #105)
//		to GRB 990123 (trig #7343).  This partial sample (1262) includes bursts where backgrounds could be fitted, and
//		peak fluxes subsequently measured.  The sample spans 7.75 years.
void GrbGlobalData::getDuration(CLHEP::HepRandomEngine *engine, long &nlong)
{
    std::vector<double>  loEdges = GRBsimvecCreator::instance()->dur_loEdge();	
    std::vector<int>     longs   = GRBsimvecCreator::instance()->dur_long();
    std::vector<int>     shorts  = GRBsimvecCreator::instance()->dur_short();
    
    // Create a sum of longs and shorts in a new vector ndurs
    std::vector<int> ndurs;
    std::transform(longs.begin(), longs.end(), shorts.begin(), 
        std::back_inserter(ndurs), std::plus<int>());
    
    std::vector<long> intgdurdist;
    GRBobsUtilities::cumulativeSum(ndurs, intgdurdist);
    
    // because intgdurdist is a cumulative sum of histplaw elements, we know that its 0th element is the minimum and
    //		last element is the max
    long diff = intgdurdist[intgdurdist.size()-1] - intgdurdist[0];
    double dur=0;
    
    m_duration.reserve(grbcst::nbsim);
    for (int isim=0; isim<grbcst::nbsim; ++isim)
    {
        m_duration.push_back(dur=evaluate(engine, diff, intgdurdist[0], 
            intgdurdist, loEdges));
        if (dur > 2.0)
            ++nlong;
    }
}




// getFLux(HepRandomEngine *engine, long nlong)
//		Chooses peak fluxes from the BATSE log N - log P;  see Bonnell et al. 1997, ApJ, 490, 79, 
//		which duplicates the procedure specified by Pendleton*.  The measurement procedure is applied 
//		uniformly for that part of the BATSE sample from GRB 910421 (trig #105) to GRB 990123 (trig #7343).  
//
//		This partial sample (1262) includes bursts where backgrounds could be fitted,
//		and peak fluxes subsequently measured.  It spans 7.75 years.  Therefore,
//		in order to draw from a PF distribution representing 1 year, we truncate
//		at the eighth brightest burst in 7.75 ~ 8 years.  The peak flux measure
//		in Bonnell et al. is for 256-ms accumulations.
//
//		*Pendleton used a different PF estimation technique for the initial BATSE Catalog.
void GrbGlobalData::getFlux(CLHEP::HepRandomEngine *engine, long nlong)
{
    std::vector<long>    m = GRBsimvecCreator::instance()->flux_m();
    std::vector<long>    n = GRBsimvecCreator::instance()->flux_n();
    std::vector<double>  p = GRBsimvecCreator::instance()->flux_p();
    std::vector<double>  q = GRBsimvecCreator::instance()->flux_q();
    
    DoubleConstIter it0 = std::max_element(p.begin(), p.end());
    DoubleConstIter it1 = std::max_element(q.begin(), q.end());
    double maxP = std::max<double> (*it0, *it1);
    
    LongConstIter it2 = std::max_element(n.begin(), n.end());
    LongConstIter it3 = std::min_element(n.begin(), n.end());
    long diff = *it2 - *it3;
    
    double f=0;
    long isim;
    
    m_flux.reserve(grbcst::nbsim);
    m_fraction.reserve(grbcst::nbsim);
    for (isim=0; isim<nlong; ++isim)
    {
        m_flux.push_back(f=computeFlux(engine, diff, *it3, n, p));
        m_fraction.push_back(f/maxP);
    }
    
    it2 = std::max_element(m.begin(), m.end());
    it3 = std::min_element(m.begin(), m.end());
    diff = *it2 - *it3;
    
    for (isim=nlong; isim<grbcst::nbsim; ++isim)
    {
        m_flux.push_back(f=computeFlux(engine, diff, *it3, m, q));
        m_fraction.push_back(f/maxP);
    }
}




// powerLawIndex(HepRandomEngine *engine, const std::vector<int> &histpl, const double factor, std::vector<double> &vect)
//		returns broken power law indices for n bursts
//
//void GrbGlobalData::powerLawIndex(HepRandomEngine *engine, 
//                                  const std::vector<int> &histpl, 
//                                  const std::vector<double> &loEdges, 
//                                  const double factor, 
//                                  std::vector<double> &vect)
void GrbGlobalData::powerLawIndex(CLHEP::HepRandomEngine *engine, 
const std::vector<int> &histpl, const double factor, std::vector<double> &vect)
{
    std::vector<long>  intgplawdist;
    GRBobsUtilities::cumulativeSum(histpl, intgplawdist);
    
    // because intgplawdist is a cumulative sum of histplaw elements, we know that its 0th element is the minimum and
    //		last element is the max
    long diff = intgplawdist[intgplawdist.size()-1] - intgplawdist[0];
    
    // Code for NEW LAT/GBM
    std::vector<int>::size_type sz = histpl.size();
    std::vector<double> loEdges(sz+1);
    for (std::vector<int>::size_type i=0; i<=sz; ++i)
        loEdges[i] = factor - i*0.1;
    
    for (long isim=0; isim<grbcst::nbsim; ++isim)
        vect.push_back(evaluate(engine, diff, intgplawdist[0], intgplawdist, loEdges));
}





// getPowerLawIndex(HepRandomEngine *engine)
//		Chooses spectral power-law indices from the BATSE power-law distribution, 
//		as measured by Preece et al. (ApJS 2000, 126, 19)
void GrbGlobalData::getPowerLawIndex(CLHEP::HepRandomEngine *engine)
{
    // Code for OLD LAT
    //std::vector<double>  loEdges  = GRBsimvecCreator::instance()->pl_loEdge();
    //std::vector<int>     histpl = GRBsimvecCreator::instance()->pl_histplaw();
    //m_beta.reserve(grbcst::nbsim);
    //powerLawIndex(engine, histpl, loEdges, 7.2, m_beta);
    
    // Code for NEW LAT/GBM
    std::vector<int>  histpl = GRBsimvecCreator::instance()->pl_histbeta();
    m_beta.reserve(grbcst::nbsim);
    powerLawIndex(engine, histpl, 7.2, m_beta);
    
    // Determine alpha
    histpl = GRBsimvecCreator::instance()->pl_histalpha();
    m_alpha.reserve(grbcst::nbsim);
    powerLawIndex(engine, histpl, 2.5, m_alpha);
    
    // Determine epeak
    m_epeak.resize(grbcst::nbsim,0);
    std::transform(m_epeak.begin(), m_epeak.end(), m_epeak.begin(), calcEpeak(engine));
}



// calcEpeak::operator () (double x)
//		returns epeak for current burst
double GrbGlobalData::calcEpeak::operator () (double x)
{
    double logsigma=0;
    
    for (int i=0; i<12; ++i)
        logsigma += m_engine->flat();
    logsigma -= 6;
    
    //double logsigma = m_engine->flat();
    return logEcen + logE*logsigma;
}
