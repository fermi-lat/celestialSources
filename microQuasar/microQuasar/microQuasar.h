/**
* @file microQuasar.h
* @brief A source class for the flux package that 
* simulates the gamma emission of microQuasars
* @author R. Dubois
*
* $Header$
*/

#ifndef microQuasar_h
#define microQuasar_h

#include <vector>

#include "flux/Spectrum.h"

/**
* @class microQuasar
*
* @brief 
* A source class for the flux package that
* simulates the gamma emission of microQuasars
* @author R. Dubois
*
*/

class microQuasar : public Spectrum {

public:

	/// This constructor is required and used in FluxSource for
	/// "SpectrumClass" sources.
	microQuasar(const std::string &params);

	virtual ~microQuasar(){};


	/// @return Particle type, "gamma".
	virtual const char * particleName() const {return "gamma";}

	/// @return average differential flux integrated over energy (photons/m^2/sr).
	/// @param time Simulation time in seconds.
	virtual double flux(double) const {return 0;};

	/// @return particle energy in MeV
	/// @param xi uniform random deviate on the unir interval
	virtual float operator()(float xi) const;

	/// @return "Effective" solid angle (sr).
	virtual double solidAngle() const {return 0;}

	/// @return Title describing the spectrum.
	virtual std::string title() const {return "microQuasar";}

	/// @return Interval to the next event (seconds)
	virtual double interval(double time);  

	/// @return Photon energy (MeV).
	virtual double energy(double time);

	/// @return Photon direction in (zenith angle (degrees), azimuth (degrees,East=0,North=90deg)).
	virtual std::pair<double, double> dir(double energy){
		return std::make_pair(0,0);
	}

	/// @return energy and photon direction (zenith angle (degrees), azimuth (degrees,East=0,North=90deg)).
	std::pair<double, std::pair<double, double> > photon();

	/// function for generating the orbit modulation
	void modulation(const double x, double& funcValue, double& derivValue);

	/// fiddle of Numerical Recipe's rtsafe to avoid function passing
	double rtsafe(const double x1, const double x2, const double xacc);

	class OrbitalRegion {
	public:
		OrbitalRegion() {
			m_spectralIndex[0] = 2.;
			m_spectralIndex[1] = 1.7;
			m_numOrbitalRegions = 2;
			m_minOrbitalPhase = 0.25;
			m_maxOrbitalPhase = 0.75;
		};
		~OrbitalRegion() {};
		float getMinOrbitalPhase() {return m_minOrbitalPhase;}
		float getMaxOrbitalPhase() {return m_maxOrbitalPhase;}
		float getSpectralIndex(int region) {return m_spectralIndex[region];}
		int findRegion(double time, float period);

	private:
		/// minimum orbital phase
		float m_minOrbitalPhase;
		/// max orbital phase
		float m_maxOrbitalPhase;
		/// number of orbital phases with different spectral indices
		int m_numOrbitalRegions;
		/// spectral index for this region
		float m_spectralIndex[2];
	};

	class DiskCycleProperties {
	public:
			DiskCycleProperties() {
				m_cycleDuration = 0.5 * 86400.;  // 1/2 day
				m_cycleDurationFluctuation = 0.10;  // 10% of duration
			}
			~DiskCycleProperties() {};
			
			float getCycleDuration() {return m_cycleDuration;}
			float getCycleDurationFluctuation() {return m_cycleDurationFluctuation;}

	private:
		/// disk cycle duration
		float m_cycleDuration;
		/// fractional fluctuation in cycle duration
		float m_cycleDurationFluctuation;

	};

	class JetProperties {
	public:
		JetProperties() {
			m_jetOnCycle = 0.25;
			m_jetOnCycleFluctuation = 0.10;
			m_jetOnDuration = 0.25;
			m_jetOnDurationFluctuation = 0.10;
		}
		~JetProperties() {};

		float getJetOnCycle() {return m_jetOnCycle;}
		float getJetOnDuration() {return m_jetOnDuration;}

	private:

		/// onset of jet during cycle (in fraction of the duration)
		float m_jetOnCycle;
		/// fluctuation of jet on onset
		float m_jetOnCycleFluctuation;
		/// duration of jet
		float m_jetOnDuration;
		/// fluctuation of jet duration
		float m_jetOnDurationFluctuation;
		/// current spectral index

	};
protected:

	void makeGrid(unsigned int n, double xmin, double xmax, 
		std::vector<double> &x, bool makeLog=false);
	double interpolate(const std::vector<double> &x, 
		const std::vector<double> &y,
		double xx);


private:
	/// flux (ph s^-1 cm^-2)
	float m_ftot;
	float m_alt;
	// minimum energy
	double m_eMin;
	/// max energy
	double m_eMax;
	/// period used 
	float m_orbitalPeriod;
	/// modulation over the period
	float m_phi0;
	float m_orbitalModulation;
	float m_nTurns;
	// current spectral index in use
	float m_currentSpectralIndex;
	/// randomly generated phase for generating interval
	float m_randPhase;	
	/// current time
	double m_currentTime;
	/// disk-cycle properties
	DiskCycleProperties m_diskProperties;
	/// jet timing properties
	JetProperties m_jetProperties;
	/// allow max two orbital regions for now
	OrbitalRegion m_orbitalRegion;

	std::vector<double> m_arrTimes;
	std::vector<double> m_integralDist;
	void computeIntegralDistribution();


};

#endif 

