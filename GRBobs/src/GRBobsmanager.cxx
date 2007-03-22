#include "GRBobs/GRBobsmanager.h"
#include <iostream>
#include <iomanip>
#include <ostream>
#include <fstream>
#include <stdexcept>
#include "flux/SpectrumFactory.h" 
#include "astro/EarthCoordinate.h"
#include "astro/GPS.h"
#include "astro/SkyDir.h"
#include "astro/PointingTransform.h"
#include "astro/JulianDate.h"
#include "ConstParMap.h"
#include "CLHEP/Vector/ThreeVector.h"
#define DEBUG 0


ISpectrumFactory &GRBobsmanagerFactory() 
{
  static SpectrumFactory<GRBobsmanager> myFactory;
  return myFactory;
}

void ComputeCoordinates(double time, double l, double b, double& ra, double& dec, double& theta, double& phi)
{
  astro::SkyDir sky(l,b,astro::SkyDir::GALACTIC);
  Hep3Vector skydir=sky.dir();
  HepRotation rottoglast =   astro::GPS::instance()->transformToGlast(time,astro::GPS::CELESTIAL);
  Hep3Vector scdir = rottoglast * skydir;
  ra    = sky.ra();
  dec   = sky.dec();
  theta = 90. - scdir.theta()*180.0/M_PI;
  phi   = scdir.phi()*180.0/M_PI;
}

GRBobsmanager::GRBobsmanager(const std::string& params)
  : m_params(params)
{

  if(DEBUG) std::cout<<" GRBobsmanager::GRBobsmanager "<<std::endl;
  //  Field Of View for generating bursts degrees above the XY plane.
  const double FOV = -100;
  if(FOV>-90) std::cout<<"WARNING!! GRBobs: FOV = "<<FOV<<std::endl;

  m_GenerateGBMOutputs = false;
  //  facilities::Util::expandEnvVar(&paramFile);  
  m_l=-200;
  m_b=-200;

  if (params.find("=") == std::string::npos) 
    {
      m_GRBnumber = (long) floor(65540+parseParamList(params,0));
      m_startTime       = parseParamList(params,0)+Spectrum::startTime();
      m_GRB_duration    = parseParamList(params,1);
      m_fluence         = parseParamList(params,2);
      m_z               = parseParamList(params,3);
      m_alpha           = parseParamList(params,4);
      m_beta            = parseParamList(params,5);
      m_epeak           = parseParamList(params,6);
      m_MinPhotonEnergy = parseParamList(params,7)*1.0e3; //MeV
      m_essc_esyn       = parseParamList(params,8);
      m_fssc_fsyn       = parseParamList(params,9);
      
      if(parseParamList(params,10)!=0) m_GenerateGBMOutputs = true;
      
      m_LATphotons     = parseParamList(params,11);
      m_EC_delay       = parseParamList(params,12);
      m_EC_duration    = parseParamList(params,13);
      m_CutOffEnergy   = parseParamList(params,14);
    }
  else
    {
      GRBobs::ConstParMap parmap(params);
      m_startTime       =  parmap.value("tstart");
      m_GRBnumber = (long) floor(65540+m_startTime);
      m_startTime       += Spectrum::startTime();
      m_GRB_duration    =  parmap.value("duration");
      if(params.find("fluence")!= std::string::npos)
	{
	  m_fluence         = parmap.value("fluence");
	}
      else
	{
	  m_fluence         = parmap.value("peakFlux");
	}
      m_l = parmap.value("l");
      m_b = parmap.value("b");
      
      m_z               = parmap.value("redshift");
      m_alpha           = parmap.value("alpha");
      m_beta            = parmap.value("beta");
      m_epeak           = parmap.value("Ep");
      m_MinPhotonEnergy = parmap.value("emin")*1.0e3; //MeV
      m_essc_esyn       = parmap.value("essc_esyn");
      m_fssc_fsyn       = parmap.value("Fssc_Fsyn");
      
      if(parmap.value("GBM")!=0) m_GenerateGBMOutputs = true;

      m_LATphotons     = parmap.value("EC_NLAT");
      m_EC_delay       = parmap.value("EC_delay");
      m_EC_duration    = parmap.value("EC_duration");
      m_CutOffEnergy   = parmap.value("EC_CutOff");
    }
  
  if(DEBUG)
    {
      std::cout<<"-------Read the following parameters --------"<<std::endl;
      std::cout<<" m_startTime         = "<<m_startTime<<std::endl;
      std::cout<<"m_GRB_duration       = "<<m_GRB_duration<<std::endl;
      std::cout<<" fluence/PF          = "<<m_fluence<<std::endl;
      std::cout<<"m_z                  = "<<m_z<<std::endl;
      std::cout<<"m_alpha              = "<<m_alpha<<std::endl;
      std::cout<<"m_beta               = "<<m_beta<<std::endl;
      std::cout<<"m_epeak              = "<<m_epeak<<std::endl;
      std::cout<<"m_MinPhotonEnergy    = "<<m_MinPhotonEnergy<<std::endl;
      std::cout<<" m_essc_esyn         = "<<m_essc_esyn<<std::endl;
      std::cout<<"m_fssc_fsyn          = "<<m_fssc_fsyn<<std::endl;
      std::cout<<"m_GenerateGBMOutputs = "<<m_GenerateGBMOutputs<<std::endl;
      std::cout<<"m_LATphotons         = "<<m_LATphotons<<std::endl;
      std::cout<<"m_EC_delay           = "<<m_EC_delay <<std::endl;
      std::cout<<"m_EC_duration        = "<<m_EC_duration<<std::endl;
      std::cout<<"m_CutOffEnergy       = "<<m_CutOffEnergy <<std::endl;
    }
  m_par = new GRBobsParameters();
  
  m_par->SetGRBNumber(m_GRBnumber);
  m_par->SetFluence(m_fluence);
  m_par->SetPeakFlux(m_fluence);
  m_par->SetDuration(m_GRB_duration);
  m_par->SetAlphaBeta(m_alpha,m_beta);
  m_par->SetEpeak(m_epeak);
  m_par->SetEssc_Esyn(m_essc_esyn);
  m_par->SetFssc_Fsyn(m_fssc_fsyn);
  m_par->SetMinPhotonEnergy(m_MinPhotonEnergy); //keV  
  m_par->SetRedshift(m_z);
  m_theta = -1000000.0;
  
  //  std::cout<<m_par->rnd->GetSeed()<<std::endl;
  //  m_GRBnumber=m_par->rnd->GetSeed();
  if (m_l >= -180 && m_l <= 180 && m_b >= -90 && m_b <= 90)
    {
      try 
	{
	  ComputeCoordinates(m_startTime,m_l,m_b,m_ra,m_dec,m_theta,m_phi);
	  m_grbdeleted      = false;
	}
      catch(const std::exception &e)
	{
	  m_grbdeleted=true;
	}
    }
  else
    while(m_theta<FOV)
      {
	m_par->SetGalDir(-200,-200); //this generates random direction in the sky
	m_GalDir = m_par->GetGalDir();      
	m_l = m_GalDir.first;
	m_b = m_GalDir.second;
	
	try{	
	  ComputeCoordinates(m_startTime,m_l,m_b,m_ra,m_dec,m_theta,m_phi);
	  m_grbdeleted      = false;
	}
	catch(const std::exception &e)
	  {
	    m_grbdeleted=true;
	    break;
	  }
      }
  m_par->SetGalDir(m_l,m_b); //this generates random direction in the sky
  m_GalDir = m_par->GetGalDir();      
  //PROMPT
  
  //  astro::EarthCoordinate earthpos()
  
  
  //  m_inSAA =   astro::EarthCoordinate::insideSAA();
  m_endTime    = m_startTime  +    m_GRB_duration; //check this in GRBobsSim !!!!!
  m_GRBend     = m_endTime;
  //AG
  if(m_LATphotons>0)
    {
      m_startTime_EC = m_startTime    + m_EC_delay;
      m_endTime_EC   = m_startTime_EC + m_EC_duration;
      m_GRBend = TMath::Max(m_endTime_EC,m_GRBend);  
    }
  
  if(DEBUG) std::cout<<m_startTime<<std::endl;
  //m_par->SetGRBNumber(m_GRBnumber);
  //  std::cout<<m_par->rnd->GetSeed()<<std::endl;
  m_grbGenerated    = false;
  //////////////////////////////////////////////////
}

std::string GRBobsmanager::GetGRBname()
{
  ////DATE AND GRBNAME
  /// GRB are named as customary GRBOBSYYMMDDXXXX
  //  The mission start and launch date are retrieved, 
  //  and the current burst's start time is added to them to form a Julian Date.
  //  Note: the argument of the JulianDate constructor is in day.

  astro::JulianDate JD(astro::JulianDate::missionStart() +
		       m_startTime/astro::JulianDate::secondsPerDay);

  int An,Me,Gio;
  m_Rest=((int) m_startTime % 86400) + m_startTime - floor(m_startTime);
  m_Frac=(m_Rest/86400.);
  int FracI=(int)(m_Frac*1000.0);
  double utc;
  JD.getGregorianDate(An,Me,Gio,utc);  
  An-=2000;

  std::ostringstream ostr;
  
  if(An<10) 
    {
      ostr<<"0";
      ostr<<An;
    }
  else
    {
      ostr<<An;
    }
  
  if(Me<10) 
    {
      ostr<<"0";
      ostr<<Me;
    }
  else
    {
      ostr<<Me;
    }
  
  if(Gio<10) 
    {
      ostr<<"0";
      ostr<<Gio;
    }
  else
    {
      ostr<<Gio;
    }
  
  if (FracI<10)
    {
      ostr<<"00";
      ostr<<FracI;
    }
  else if(FracI<100) 
    {
      ostr<<"0";
      ostr<<FracI;
    }
  else 
    ostr<<FracI;

  std::string GRBname = ostr.str();
  
  if(DEBUG) std::cout<<"GENERATE GRB ("<<GRBname<<")"<<std::endl;
  
  //..................................................  //
  return GRBname;
}

GRBobsmanager::~GRBobsmanager() 
{  
  DeleteGRB();
}

void GRBobsmanager::GenerateGRB()
{
  using namespace std;

  if(m_grbdeleted) return;

  /////GRB GRNERATION////////////////////////////////
  m_GRB      = new GRBobsSim(m_par);
  TH2D *h;
  h = m_GRB->MakeGRB();
  m_spectrum = new SpectObj(m_GRB->CutOff(h,m_CutOffEnergy) , 0, m_z);
  m_spectrum->SetAreaDetector(EventSource::totalArea());
  //  m_endTime    = m_startTime  + m_GRB->Tmax();
  PromptEmission = new SpectralComponent(m_spectrum,m_startTime,m_endTime);
  //cout<<"Generate PROMPT emission ("<<m_startTime<<" "<<m_endTime<<")"<<endl;
  if(m_LATphotons>0)
    {
      //      m_startTime_EC = m_startTime    + m_EC_delay;
      //      m_endTime_EC   = m_startTime_EC + m_EC_duration;
      h = m_GRB->MakeGRB_ExtraComponent(m_EC_duration,m_LATphotons);
      m_spectrum1 = new SpectObj(m_GRB->CutOff(h,m_CutOffEnergy),0, m_z);
      m_spectrum1->SetAreaDetector(EventSource::totalArea());
      AfterGlowEmission = new SpectralComponent(m_spectrum1,m_startTime_EC,m_endTime_EC);
      //      cout<<"Generate AFTERGLOW Emission ("<<m_startTime_EC<<" "<<m_endTime_EC<<")"<<endl;
    }
  
  //////////////////////////////////////////////////
  string GRBname = GetGRBname();
  
  /*  if(m_grbocculted)
      {
      cout<<"This GRB is occulted by the Earth"<<endl;
      }
  else 
  */
  if(m_GenerateGBMOutputs)
    {
      m_GRB->SaveGBMDefinition(GRBname,m_ra,m_dec,m_theta,m_phi,m_Rest);
      m_GRB->GetGBMFlux(GRBname);
    }
  
  string name = "GRBOBS_";
  name+=GRBname; 
  name+="_PAR.txt";
  //..................................................//
  ofstream os(name.c_str(),ios::out);  
  os<<"  GRBName      T_start         T_end        L        B       Ra      Dec    Theta      Phi        z     Flux    Alpha     Beta    Epeak     Essc     Fssc      Eco    N_ext  Del_ext  Dur_ext "<<endl;
  os<<setw(9)<<GRBname<<setw(13)<<setprecision(10)<<m_startTime<<" "<<setw(13)<<m_GRBend;
  os<<" "<<setprecision(4)<<setw(8)<<m_l<<" "<<setw(8)<<m_b;
  os<<" "<<setw(8)<<m_ra<<" "<<setw(8)<<m_dec;
  os<<" "<<setw(8)<<m_theta<<" "<<setw(8)<<m_phi;
  os<<" "<<setw(8)<<m_z<<" "<<setw(8)<<m_fluence;
  os<<" "<<setw(8)<<m_alpha<<" "<<setw(8)<<m_beta<<" "<<setw(8)<<m_epeak;
  os<<" "<<setw(8)<<m_essc_esyn<<" "<<setw(8)<<m_fssc_fsyn<<" "<<setw(8)<<m_CutOffEnergy;
  os<<" "<<setw(8)<<m_LATphotons <<" "<< setw(8)<<m_EC_delay<<" "<<setw(8)<<m_EC_duration<<endl;
  os.close();  

  cout<<"GRB"<<GRBname;
  if( m_z >0)  cout<<" redshift = "<<m_z;
  cout<<setprecision(10)<<" t start "<<m_startTime<<", tend "<<m_endTime
	   <<" l,b = "<<m_l<<", "<<m_b<<" elevation,phi(deg) = "<<m_theta<<", "<<m_phi;
  if(m_par->GetNormType()=='P')
    cout<<" Peak Flux = "<<m_fluence<<" 1/cm^2/s "<<endl;
  else 
    cout<<" Fluence = "<<m_fluence<<" erg/cm^2"<<endl;
  if(m_LATphotons>0) 
    {
      cout<<"Generate AFTERGLOW Emission ("<<setprecision(10)<<m_startTime_EC<<" "<<m_endTime_EC<<")"<<endl;
    }
  m_grbGenerated=true;
}

void GRBobsmanager::DeleteGRB()
{
  delete m_par;
  if(m_grbGenerated)
    {
      delete m_GRB;
      delete m_spectrum;
      delete PromptEmission;
      if(m_LATphotons>0) 
	{
	  delete AfterGlowEmission;
	  delete m_spectrum1;
	}
    }
  m_grbdeleted=true;
}

//return flux, given a time
double GRBobsmanager::flux(double time) const
{
  double flux = 0.0;
  //////////////////////////////////////////////////
  
  if(m_grbdeleted) 
    flux = 0.0;
  else if(time <= m_startTime || time>  m_GRBend) 
    {
      if(m_grbGenerated) const_cast<GRBobsmanager*>(this)->DeleteGRB();
      flux = 0.0;
    }
  else 
    {
      if(!m_grbGenerated) const_cast<GRBobsmanager*>(this)->GenerateGRB();
      flux   = PromptEmission->flux(time,m_MinPhotonEnergy);
      if(m_LATphotons>0) flux  += AfterGlowEmission->flux(time,m_MinPhotonEnergy);
    }
  if(DEBUG && flux) std::cout<<"flux("<<time<<") = "<<flux<<std::endl;
  return flux;
}

double GRBobsmanager::interval(double time)
{  
  if(DEBUG) std::cout<<"interval at "<<time<<std::endl;
  double inte, inte_prompt, inte_ag;
  inte_ag=1e20;

  if(m_grbdeleted) 
    {
      inte = 1e10;
    }
  else if(time < m_startTime) 
    {
      inte = m_startTime - time;
    }
  else if(time<m_GRBend) //During the prompt emission
    {
      if(!m_grbGenerated) GenerateGRB();
      inte_prompt = PromptEmission->interval(time,m_MinPhotonEnergy);
      
      if(m_LATphotons>0) inte_ag = AfterGlowEmission->interval(time,m_MinPhotonEnergy);
      if (inte_prompt<=inte_ag) 
	{ 
	  inte  = inte_prompt;
	  PromptEmission->SetResiduals(0.0);
	  if(m_LATphotons>0) AfterGlowEmission->SetResiduals(inte);
	}
      else
	{ 
	  inte  = inte_ag;
	  PromptEmission->SetResiduals(inte);
	  if(m_LATphotons>0) AfterGlowEmission->SetResiduals(0.0);
	}
    }
  else  
    {
      inte = 1e10;
      if(m_grbGenerated) DeleteGRB();
    }
  if(DEBUG) std::cout<<"interval("<<time<<") = "<<inte<<std::endl;
  return inte;
}

double GRBobsmanager::energy(double time)
{
  if(DEBUG) std::cout<<"energy at "<<time<<std::endl;
  double ene=0.1;
  double ene_ag=0.0;
  if(m_grbGenerated && !m_grbdeleted)
    {
      if(m_LATphotons>0) ene_ag = AfterGlowEmission->energyMeV(time,m_MinPhotonEnergy);
      ene = TMath::Max(PromptEmission->energyMeV(time,m_MinPhotonEnergy),ene_ag);
    }
  if(DEBUG) 
    std::cout<<"energy("<<time<<") = "<<ene<<std::endl;
  return ene;
}

double GRBobsmanager::parseParamList(std::string input, unsigned int index)
{
  std::vector<double> output;
  unsigned int i=0;
  for(;!input.empty() && i!=std::string::npos;){
    double f = ::atof( input.c_str() );
    output.push_back(f);
    i=input.find_first_of(",");
    input= input.substr(i+1);
  } 
  if(index>=output.size()) return 0.0;
  return output[index];
}


