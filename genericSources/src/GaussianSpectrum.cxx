/**
 * @file GaussianSpectrum.cxx
 * @brief A simple Spectrum subclass that exercises the flux package.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <cstdlib>

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGauss.h"

#include "facilities/Util.h"

#include "astro/SkyDir.h"

#include "flux/SpectrumFactory.h"
#include "flux/EventSource.h"

#include "genericSources/GaussianSpectrum.h"

#include "celestialSources/ConstParMap.h"

auto GaussianSpectrumFactory() -> ISpectrumFactory & {
   static SpectrumFactory<GaussianSpectrum> myFactory;
   return myFactory;
}

GaussianSpectrum::GaussianSpectrum(const std::string & paramString) 
   : m_flux(1.), m_mean(1e4), m_sigma(1e3), m_l(0), m_b(0) {

   celestialSources::ConstParMap pars(paramString);
   
   m_flux = pars.value("flux");
   m_mean = pars.value("mean");
   m_sigma = pars.value("sigma");
   m_l = pars.value("glon");
   m_b = pars.value("glat");
}

auto GaussianSpectrum::operator()(float xi) const -> float {
   (void)(xi);
   float energy(CLHEP::RandGauss::shoot(m_mean, m_sigma));
   return energy;
}

auto GaussianSpectrum::flux(double time) const -> double {
   (void)(time);
   return m_flux;
}

auto GaussianSpectrum::solidAngle() const -> double {
   return 1;
}

auto GaussianSpectrum::interval(double time) -> double {
   double rate = flux(time)*EventSource::totalArea();
   double xi = CLHEP::RandFlat::shoot();
   return -std::log(1. - xi)/rate;
}

auto GaussianSpectrum::energy(double time) -> double {
   (void)(time);
   double xi(0);
   return (*this)(xi);
}

auto GaussianSpectrum::dir(double energy) -> std::pair<double, double> {
   (void)(energy);
   return std::make_pair(m_l, m_b);
}
