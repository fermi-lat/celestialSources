#include "RadiationProcess.h"
#include "GRBConstants.h"

RadiationProcess::RadiationProcess(SpectObj spectrumObj)
  : m_spectrumObj(spectrumObj)
{;}

double RadiationProcess::processFlux(double E, 
				     double ec, 
				     double em)
{
  double value = 0;
  if(ec <= em) //FAST COOLING
    {
      if(E <= ec)
	value = pow(E/ec,0.33333);
      else if (E <= em)
	value = pow(E/ec,-0.5);
      else 
	value = pow(em/ec,-0.5)*pow(E/em,-cst::p/2.);
    }
  else //SLOW COOLING
    {
      if(E <= em)
	value = pow(E/em,0.33333);
      else if (E <= ec)
	value = pow(E/em,-(cst::p-1.)/2.);
      else 
	value = pow(ec/em,-(cst::p-1.)/2.)*pow(E/ec,-cst::p/2.);
    }
  return value;
}

double RadiationProcess::electronDensity(double gi, double gamma_min, 
				       double gamma_max, double dr,
				       double ComovingTime, 
				       double Psyn_0,
				       double tau_0,double N0)
{
  const double N_0  = N0*(cst::p-1.)/(pow(gamma_min,1.0-cst::p)-pow(gamma_max,1.0-cst::p)); 

  double gi2     = pow(gi,2.);
  double Psyn    = Psyn_0*(gi2-1.); //MeV/s
  double tsyn    = ((gi)*cst::mec2)/Psyn;
  double tcross  = dr/(sqrt(1.-1./gi2)*cst::c);

  double N_e = 1;

  if (gi<gamma_min ) 
    N_e *= (gi/gamma_min)*pow(gamma_min,-cst::p); //adim;
  else if (gi>gamma_max) 
    N_e *= 0.0;
  else 
    N_e *= pow(gi,-cst::p); //adim;
  
  if(ComovingTime<tcross)
    N_e *= tsyn/tcross*(1.-exp(-ComovingTime/tsyn));
  else
    N_e *= tsyn/tcross*(1.-exp(-tcross/tsyn))*exp((tcross-ComovingTime)/tsyn);
  return N_e;
}

