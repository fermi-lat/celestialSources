/*!\class GRBTest
 * \brief Test class for GRB simulation studies.
 * 
 * This class is called from GRB Gaudi Algorithm.
 * It basically compute the spectrum and returns a photons for the montecarlo.
 * It calls the GRBSim class as kernel of the simulation.
 * Some quantities are also calculated to help people understanding 
 * GRB physics.
 *
 */

// Include files
#include "FluxSvc/IFluxSvc.h"
#include "FluxSvc/IFlux.h"

// GlastEvent for creating the McEvent stuff
#include "Event/TopLevel/Event.h"
#include "Event/TopLevel/MCEvent.h"
#include "Event/MonteCarlo/McParticle.h"
#include "Event/TopLevel/EventModel.h"

// Gaudi system includes

#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/IParticlePropertySvc.h"
#include "GaudiKernel/SmartRefVector.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/Algorithm.h"
#include "facilities/Util.h"
#include <list>
#include <string>
#include <vector>
//#include "GaudiKernel/ParticleProperty.h"


// GRB includes
#include "../GRB/GRBConstants.h"
//include files for ROOT...
#include "TTree.h"
#include "TBranch.h"
#include "TObjArray.h"
#include "TFile.h"

namespace channel
{
  // BATSE 1 Channel (MeV):
  const double ch1L      = 50.0e-3;
  const double ch1H      = 100.0e-3;
  // BATSE 2 Channel
  const double ch2L      = 100.0e-3;
  const double ch2H      = 300.0e-3;
  // GBM Energy Range:
  const double ch3L      = 25.0e-3;
  const double ch3H      = 10.0;
  // LAT Energy Range
  const double ch4L      = 20.0;
  const double ch4H      = 1.0e+3;
  const double ch5L      = 1.0e+3;
  const double ch5H      = 300.0e+3;
  //////////////////////////////////////////////////
}

using namespace std;
using namespace channel;

class DataOut{
public:
  DataOut();
  //! destructor
  //  ~DataOut();

  inline void setTime(double value)   {o_time=value;}
  inline void setEnergy(double value) {o_energy=value;}
  
  inline void setPhi(double value)    {o_phi=value;}
  inline void setTheta(double value)  {o_theta=value;}
  
  inline void setGal_l(double value)  {o_l=value;}
  inline void setGal_b(double value)  {o_b=value;}
  
  inline void setSignal(int value)    {o_signal=value;}
public:

  inline double Time()   {return o_time;}
  inline double Energy() {return o_energy;}
  
  inline double Phi()    {return o_phi;}
  inline double Theta()  {return o_theta;}
  
  inline double Gal_l()  {return o_l;}
  inline double Gal_b()  {return o_b;}
  
  inline int   Signal() {return o_signal;}
private:
  double o_time;
  double o_energy;
  
  double o_phi;
  double o_theta;
  
  double o_l;
  double o_b;
  
  int o_signal;
};

class TimeCmp{
public:
//  bool operator()(const DataOut& data1,const DataOut& data2)
  bool operator()(DataOut& data1, DataOut& data2)
  {
    return data1.Time() < data2.Time();    
  }
};

class GRBTest{
    
public:
  //! Constructor
  GRBTest();
  ~GRBTest(){}
  //! Return the pointer to flux service
  inline void setService(IFluxSvc* ptr){m_fsvc = ptr;}
  //! Calculate the fluence in the energy band between e1 and e2.
  double CalculateFluence(double ee/* MeV */,
			  double e1=cst::enmin*1e-6/* MeV */,
			  double e2=cst::enmax*1e-6/* MeV */);
  //! Generate a GRB
  int Start(std::vector<char*> argv);
  //! Help printout utility
  void help();
  //! List all the sources available
  void listSources(const std::list<std::string>& source_list );
  //! List of loaded spectrum objects
  void listSpectra();
  
private:  
  IFlux* m_flux;    // pointer the a flux object
  IFluxSvc* m_fsvc; // pointer to the flux Service  
  int m_loop;
  bool savef_root;
  bool savef_ascii;
  double TIME;
  int EVENTS;
  const char * default_arg;//="GRBSpectrum";
  std::vector<DataOut> theData;  
};

DataOut::DataOut(){}


GRBTest::GRBTest()
{
  cout<<" Starting a new Test"<<endl;
}
//------------------------------------------------------------------------------
double GRBTest::CalculateFluence(double ee/* MeV */,
				 double e1/*=cst::enmin*1e-6 MeV */,
				 double e2/*=cst::enmax*1e-6 MeV */)
{
  double fluence;
  if (ee>=e1 && ee<=e2) 
    {
      fluence=ee;
    }
  else 
    {
      fluence=0.0;
    }
  return fluence;
}

//------------------------------------------------------------------------------
void GRBTest::help() {
  std::cout << 
    "   Simple test program for Transient Sources.\n"
    "   Command line args are \n"
    "      '-events <number of events to create>'\n"
    "      '-time <time in seconds>'    for the maximum time\n"
    "      '-list' lists the available spectra\n"
    "      '-root' <number of the run> to save the events in a ROOT tree"
    "      '-ascii'<number of the run> to save the events in an asciifile"
    "      '-help' for this help"
	    << std::endl;
}

//------------------------------------------------------------------------------
void GRBTest::listSources(const std::list<std::string>& source_list ) {
   std::cout << "List of available sources:" << std::endl;
   for( std::list<std::string>::const_iterator it = source_list.begin(); 
   it != source_list.end(); ++it) { 
      std::cout << '\t'<< *it << std::endl;
   }
}

//------------------------------------------------------------------------------
void GRBTest::listSpectra() {
  std::cout << "List of loaded Spectrum objects: " << std::endl;
  std::list<std::string> spectra(m_fsvc->fluxNames());
  for( std::list<std::string>::const_iterator it = spectra.begin(); 
       it != spectra.end(); ++it) { 
    std::cout << '\t'<< *it << std::endl;
  }
}

//------------------------------------------------------------------------------

int GRBTest::Start(std::vector<char*> argv)
{
  int argc = argv.size();
  //  cout<<argc<<endl;
  int nume,i;
  int num_sources=0;
  double time_max=TIME;  //time to use for flux and rate functions
  int events_max=EVENTS;
  double time,energy,Rate,Area;
  // Vector dir;
  HepVector3D dir,GalDir;

  double cos_theta,phi;
  double b,l;
  int current_arg = 1;
  double fluenceTot;
  double fluence1,fluence2,fluence3,fluence4,fluence5;
  default_arg="GRBSpectrum";
  std::string arg_name(default_arg);
  vector<std::string> sources;

  /*
    cout << "------------------------------------------------------" <<endl;
    cout << " Flux test program: type 'GRBTest.exe -help' for help" <<endl;
    cout << ( ( argc == 1)?  " No command line args, using defaults"
    :  "") <<endl;
  */
  savef_root=false;
  savef_ascii=false;
  while(current_arg < argc)
    {
      arg_name = argv[current_arg];
      cout<<arg_name<<endl;
      if("-help" == arg_name || "help" == arg_name) 
	{ 
	  help();
	  return 0;
	}
      
      else if("-list" == arg_name) 
	{ 
	  listSources(m_fsvc->fluxNames());
	  listSpectra(); 
         return 0; 
      }

      else if("-time" == arg_name) {
	time_max =atof(argv[++current_arg]);
	//cout<<" MAX TIME = "<<time_max<<endl;
      }
      else if("-events" == arg_name) {
	events_max = atoi(argv[++current_arg]);
	//cout<<" MAX NUM OF EVENTS = "<<events_max<<endl;
	if (events_max<1) return 0;
      }
      else if("-root" == arg_name) {
	cout<<" SAVE ROOT" <<endl;
	savef_root=true;
	m_loop = atoi(argv[++current_arg]);//argv[++current_arg];
      }
      else if("-ascii" == arg_name) {
	cout<<" SAVE ASCII" <<endl;
	savef_ascii=true;
	m_loop = atoi(argv[++current_arg]);//argv[++current_arg];
      }
      
      else if('-' == arg_name[0]) {
	std::cerr << "Unrecognized option "<< arg_name << ", -help for help" << std::endl;}
      else
	{
	  sources.push_back(arg_name);
	  num_sources++;
	}
      current_arg++;
   }
  if(0 == sources.size())
    {
      sources.push_back(default_arg);
      num_sources++;
    }
  //  cout<<"Num Sources = "<<num_sources<<endl;
  // Create the file, the tree and the branches...
  TTree* events;        
  TObjArray Forest(0);
  
  char* number;
  char root_name[50];
  char ascii_name[50];
  
  for(i = 0; i < num_sources; i++)
    {  
      int signal=1;     
      if(i>0) signal=0;
      nume=1;
      
      fluenceTot=0.0;
      fluence1=0.0;
      fluence2=0.0;
      fluence3=0.0;
      fluence4=0.0;
      fluence5=0.0;
      
      
      StatusCode sc =  m_fsvc->source(sources[i], m_flux);
      if( sc.isFailure()) 
	{
	  std::cout << "Could not find flux " <<sources[i]<<endl;
	  return sc;
	}
      cout<<" Source Name = "<<sources[i]<<endl;
      number = new char(20);
      
      if (m_loop<10) 
	{
	  sprintf(number,"00%d",m_loop);
	}
      else if (m_loop<100) 
	{
	  sprintf(number,"0%d",m_loop);
	}
      else
	{
	  sprintf(number,"%d",m_loop);
	}
      
      //name=const_cast<char *>(sources[i].c_str());
      //      name=m_source_name;
      if (savef_root==true)
	{
	  sprintf(root_name,"%s%s",sources[i].c_str(),number);
	  events= new TTree(root_name,root_name);
	  events->Branch("energy",&energy,"energy/D");
	  events->Branch("time",&time,"time/D");
	  events->Branch("Rate",&Rate,"Rate/D");
	  events->Branch("cos_theta",&cos_theta,"cos_theta/D");
	  events->Branch("phi",&phi,"phi/D");
	  events->Branch("l",&l,"l/D");
	  events->Branch("b",&b,"b/D");
	  Forest.Add(events);
	}
      //sb pair<double,double> loc=m_fsvc->location();

      //      cout << loc.first << "   " << loc.second <<endl;
      time=0.0;
      double t1;
      t1=time;
      double dt=0.0;

      while(time<time_max && nume<=events_max)
	{
	  //	  cout<<"GPS = "<<m_flux->gpsTime()<<endl;
	  // cout<<"m_flux->generate()"<<endl;
	  m_flux->generate(); 
	  // cout<<"m_flux->time()"<<endl;
	  time=m_flux->time();
	  // cout<<"time = "<<time<<endl;

	  if (time>=1.0e+6) break;
	  dir = m_flux->launchDir();
	  //////////////////////////////////////////////////
	  GalDir=m_flux->transformGlastToGalactic(time)*dir;
	  double x_g,y_g,z_g;

	  x_g=GalDir.x();
	  y_g=GalDir.y();
	  z_g=GalDir.z();

	  if(abs(x_g)<=1e-10) x_g=0.0;
	  if(abs(y_g)<=1e-10) y_g=0.0;
	  if(abs(z_g)<=1e-10) z_g=0.0;
	  
	  b = 360/M_2PI*asin(y_g);
	  l = 360/M_2PI*atan2(x_g,z_g);
	  if(abs(b)<1.0e-10) b=0.;
	  if(abs(l)<1.0e-10) l=0.;
	  
	  // cout<<"m_flux->energy()"<<endl;
	  energy = m_flux->energy(); // kinetic energy in MeV
	  // cout<<"energy = "<<energy<<endl;
	  Area=m_flux->targetArea(); 
	  // cout<<"m_flux->rate()"<<endl;
	  Rate= m_flux->rate();
	  dt=time-t1;

	  // Calculate the Fluences
	  
	  fluenceTot+=CalculateFluence(energy);
	  fluence1+=CalculateFluence(energy,ch1L,ch1H);
	  fluence2+=CalculateFluence(energy,ch2L,ch2H);
	  fluence3+=CalculateFluence(energy,ch3L,ch3H);
	  fluence4+=CalculateFluence(energy,ch4L,ch4H);
	  fluence5+=CalculateFluence(energy,ch5L,ch5H);
	  
	  cos_theta = dir.z();
	  phi = atan2(dir.y(),dir.x());
	  if(phi < 0) phi = 2*M_PI + phi;
	  phi=180*phi/M_PI; //degrees

	  if (savef_root==true) events->Fill();
	  if (savef_ascii==true)
	    {
	      DataOut myData;
     
	      myData.setEnergy(energy);
	      myData.setTime(t1);
	      myData.setPhi(phi);
	      myData.setTheta(acos(cos_theta)*180.0/M_PI); //in degrees
	     
	      myData.setGal_l(l); 
	      myData.setGal_b(b); 
	      
	      myData.setSignal(signal); 

	      theData.push_back(myData);
	    }
	  if (nume%1==0){
	    cout<<
	      "-------- Event Number: "<<nume<<"\n"<<
	      " Time [s] = "<<t1<<"\n"<<
	      " Rate [ph/(s)]= "<<Rate<<"\n"<<
	      " 1/Rate [s]= "<<1/Rate<<"\n"<<
	      " Interval [s]= "<<dt<<"\n"<<
	      " Area [m^2]= "<<Area<<"\n"<<
	      // " --------------------------------\n"<<
	      " Energy of Photon Extracted [ MeV ]= "<<energy<<"\n"<<
	      " Direction: GLAST :Cos(theta) = " << cos_theta <<", phi = "<<phi<<"\n"<<
	      "            GALACTIC :      l = "<<l<<" b = "<<b<<
	      endl;
	  }
	  t1=time;
	  nume++;
	}
      cout<<"Time final="<<t1<<endl;
      cout<<"Number of events processed for this source= "<<nume-1<<endl;
      if (savef_root==true) events->Print();
      cout<<"Total Fluence [erg/cm^2]="<<fluenceTot/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      
      cout<<"Fluence ("<<ch1L<<"  MeV - "<<ch1H<<" MeV) [erg/cm^2]="<<fluence1/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      cout<<"Fluence ("<<ch2L<<"  MeV - "<<ch2H<<" MeV) [erg/cm^2]="<<fluence2/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      cout<<"Fluence ("<<ch3L<<"  MeV - "<<ch3H<<" MeV) [erg/cm^2]="<<fluence3/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      cout<<"Fluence ("<<ch4L<<"  MeV - "<<ch4H<<" MeV) [erg/cm^2]="<<fluence4/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      cout<<"Fluence ("<<ch5L<<"  MeV - "<<ch5H<<" MeV) [erg/cm^2]="<<fluence5/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
      
      if (savef_root==true){
	std::string paramFile = "GRBdata.txt";
	facilities::Util::expandEnvVar(&paramFile);
	std::ofstream fout(paramFile.c_str(),ios::app);
	fout<<t1<<endl;
	
	fout<<(fluenceTot)*(1.0/cst::erg2MeV)<<endl;
	fout<<fluence1/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
	fout<<fluence2/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
	fout<<fluence3/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
	fout<<fluence4/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
	fout<<fluence5/(Area*1.0e+4)*(1.0/cst::erg2MeV)<<endl;
	
	fout.close();
      }
      
    } // For all the sources...
  if (savef_root==true)
    { 
      TFile f("Events.root","UPDATE");
      Forest.Write();
      f.Close();
    } 
  
  if (savef_ascii==true)
    {
      sprintf(ascii_name,"PhotonList%s.txt",number);
      //      std::string photonList = "PhotonList.txt";
      //      std::ofstream phout(photonList.c_str(),ios::out);
      FILE *phout;
      
      //      phout=fopen(photonList.c_str(),"w");
      phout=fopen(ascii_name,"w");
      cout<<" Total Number Of Events = "<<theData.size()<<endl;
      std::vector<DataOut>::iterator itr;
      cout<<"Sorting The Photon List..."<<endl;
      std::sort(theData.begin(), theData.end(), TimeCmp());
      for(itr=theData.begin();itr != theData.end();++itr)
	{
	  fprintf(phout,"%lf\t%lf\t%lf\t%lf\t%d\n",
		  (*itr).Time(),
		  (*itr).Energy(),
		  (*itr).Phi(),
		  (*itr).Theta(),
		  (*itr).Signal());
	  cout<<(*itr).Energy()<<endl;
	  /*
	    phout<<(*itr).Time()<<"\t"
	    <<(*itr).Energy()<<"\t"
	    <<(*itr).Time()<<"\t"
	    <<(*itr).Phi()<<"\t"
	    <<(*itr).Theta()<<"\t"
	    <<(*itr).Signal()<<"\n";
	  */
	}
      //phout.close();
      fclose(phout);
    }
  
}

