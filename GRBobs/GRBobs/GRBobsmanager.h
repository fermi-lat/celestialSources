/*!
  \class  GRBobsmanager
  
  \brief Spectrum class for many GRBs 
  This class concatenates several GRB one after the other 
  for simulating a series of several GRBs.
  
  \author Nicola Omodei       nicola.omodei@pi.infn.it 
  \author Johann Cohen-Tanugi johann.cohen@pi.infn.it
*/

#ifndef GRBobsmanager_H
#define GRBobsmanager_H
#include "GRBobsConstants.h"

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include "flux/ISpectrum.h"
//#include "facilities/Observer.h"
#include "GRBobsSim.h"
#include "SpectObj/SpectObj.h"

#include "facilities/Util.h"

//class ISpectrum;

class GRBobsmanager : public ISpectrum
{
  
 public:
  /*! This initializes the simulation parsing the parameters.
    
    \param params are set in the xml source library in xml directory.
    They are: 
    - The time of the first burst
    - The time to wait before the next burst
    
    An example the xml source declaration for this spectrum should appears:
    \verbatim
    <source name=" GRBobsmanager_Gal">
    <spectrum escale="GeV"> <SpectrumClass name="GRBobsmanager" params="50 100"/>
    <use_spectrum frame="galaxy"/> 
    </spectrum> </source>
    \endverbatim
  */
  
  GRBobsmanager(const std::string& params);
  
  virtual  ~GRBobsmanager();
   
  /*! If a burst is shining it returns the flux method 
   */
  double flux(double time)const;
  /*! \brief Returns the time interval
   *
   * If a burst is shining it returns the interval method.
   * If not it returns the time to whait for the first photon of the next burst.
   */
  double interval(double time);
  
  //! direction, taken from GRBobsSim
  inline std::pair<double,double>
    dir(double energy) 
    {
      return m_GRB->GRBdir();
    } 

  double energy(double time);
  
  std::string title() const {return "GRBobsmanager";} 
  const char * particleName() const {return "gamma";}
  const char * nameOf() const {return "GRBobsmanager";}
  
  /*! 
    This method parses the parameter list
    \param input is the string to parse
    \param index if the position of the parameter in the input list. 
    \retval output is the value of the parameter as float number.
  */  
  double parseParamList(std::string input, int index);  
  
 private:
  
  SpectObj    *m_spectrum;

  GRBobsSim   *m_GRB;
  GRBobsParameters  *m_par;

  const std::string& m_params;
  std::string paramFile;


  double m_fluence;
  int    m_Npulses;
  double m_ExponentialTau;
  double m_MinPhotonEnergy;
  double m_startTime;
  double m_endTime;
};
#endif