
#ifndef GRBmanager_H
#define GRBmanager_H

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include "FluxSvc/ISpectrum.h"
#include "facilities/Observer.h"
#include "src/GPS.h"
#include "CLHEP/Random/RandomEngine.h"
#include "GRBSpectrum.h"

class ISpectrum;

class GRBmanager : public ISpectrum
{

 public:
  
  //! Constructor: takes a file with some parameters as argument.
  /*! \param params file of parameters 
   */
  GRBmanager(const std::string& params);
  /*! Destructor
   */
  virtual  ~GRBmanager(); 
  
  float operator() (float u) const;
  
  double flux(double time)const; //{return m_GRB->flux(time);}
  double rate(double time) const;//{return m_GRB->rate(time);}
  double interval(double time);//{return m_GRB->interval(time);}//const;
  double solidAngle() const;//{return m_GRB->solidAngle();}
  
  std::pair<float,float> dir(float energy) const;// {return m_GRB->dir(energy);}
  std::pair<double,double> dir(double energy, HepRandomEngine* engine){
    return dir(energy);} 
  
  double energySrc(HepRandomEngine*, double /*time*/ );//double energySrc(HepRandomEngine* engine, double time ) {return m_GRB->energySrc(engine,time);}
  //! inherited from Spectrum
  std::string title() const {return "GRBmanager";} 
  const char * particleName() const {return "gamma";}
  const char * nameOf() const {return "GRBmanager";}
  
 private:
  const std::string& m_params;
  GRBSpectrum* m_GRB;
  double m_Time;
  double m_timeToWait;
};
#endif
