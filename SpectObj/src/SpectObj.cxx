#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
//#include "SpectObj.h"
#include "SpectObj/SpectObj.h"

#define DEBUG 0

const double erg2meV      = 624151.0;

SpectObj::SpectObj(const TH2D* In_Nv, int type)
{
  m_AreaDetector=1.0;
  //  AreaDetector = EventSource::totalArea()
  Nv   = (TH2D*)In_Nv->Clone(); // ph/kev/s/m�
  std::string name;
  GetUniqueName(Nv,name);
  gDirectory->Delete(name.c_str());
  Nv->SetName(name.c_str());
  sourceType = type;
  //  if(DEBUG) 
  std::cout<<type<<" ->SpectObj<-  "<<name<<std::endl;
  m_SpRandGen = new TRandom();
  sourceType = type; //Max

  ne   = Nv->GetNbinsY();
  double* en   = new double[ne+1];
  emin = Nv->GetYaxis()->GetXmin();
  emax = Nv->GetYaxis()->GetXmax();
  
  nt   = Nv->GetNbinsX();
  m_Tmin = Nv->GetXaxis()->GetXmin();
  m_Tmax = Nv->GetXaxis()->GetXmax();
  m_TimeBinWidth   = Nv->GetXaxis()->GetBinWidth(0);
  
  for(int ei = 0 ; ei<=ne ; ei++) 
    {      
      en[ei] =  Nv->GetYaxis()->GetBinLowEdge(ei+1);
    }
  
  double dei;

  for (int ei = 0; ei<ne; ei++)
    {
      dei   = Nv->GetYaxis()->GetBinWidth(ei+1);
      for(int ti = 0; ti<nt; ti++)
	{
	  Nv->SetBinContent(ti+1, ei+1, 
			    Nv->GetBinContent(ti+1, ei+1)*dei*m_TimeBinWidth); //[ph/m�]
	}  
    }
  SetAreaDetector(); // this fix the area to 6 square meters (default value) and rescale the histogram

  gDirectory->Delete("spec");
  gDirectory->Delete("times");
  gDirectory->Delete("Probability");

  spec        = new TH1D("spec","spec",ne,en);
  times       = new TH1D("times","times",nt,m_Tmin,m_Tmax);
  Probability = new TH1D("Probability","Probability",nt,m_Tmin,m_Tmax);
  
    //times = new TH1D("times","times",nt,m_Tmin,m_Tmax);
  GetUniqueName(spec ,name);
  gDirectory->Delete(name.c_str());
  spec->SetName(name.c_str());

  GetUniqueName(times,name);
  gDirectory->Delete(name.c_str());
  times->SetName(name.c_str());

  GetUniqueName(Probability,name);
  gDirectory->Delete(name.c_str());
  Probability->SetName(name.c_str());
  ProbabilityIsComputed=false;

  if (sourceType ==1)
    {
      PeriodicSpectrum = new TH1D("PeriodicSpectrum","PeriodicSpectrum",nt,m_Tmin,m_Tmax);
      GetUniqueName(PeriodicSpectrum,name);
      gDirectory->Delete(name.c_str());
      PeriodicSpectrum->SetName(name.c_str());
    }
  
  delete[] en;
  if(DEBUG)  std::cout<<" SpectObj initialized ! ( " << sourceType <<")"<<std::endl;
  //////////////////////////////////////////////////
}


void SpectObj::SetAreaDetector(double AreaDetector)
{
  std::cout<<"Set the generation area to "<<AreaDetector<<" m2"<<std::endl;
  Nv->Scale(AreaDetector/m_AreaDetector); // ph
  m_AreaDetector = AreaDetector;
}

void SpectObj::GetUniqueName(const void *ptr, std::string & name)
{
  std::ostringstream my_name;
  my_name << reinterpret_cast<int> (ptr);
  name = my_name.str();
}

TH1D *SpectObj::CloneSpectrum()
{
  std::string name;
  TH1D *sp = (TH1D*) spec->Clone();
  GetUniqueName(sp ,name);
  gDirectory->Delete(name.c_str());
  sp->SetName(name.c_str());
  return sp;
}

TH1D *SpectObj::CloneTimes()
{
  TH1D *lc = (TH1D*) times->Clone();
  std::string name;
  GetUniqueName(lc ,name);
  gDirectory->Delete(name.c_str());
  lc->SetName(name.c_str());
  return lc;
} 




//////////////////////////////////////////////////
TH1D *SpectObj::GetSpectrum(double t)
{
  if (t == 0.0) return spec;
  int ti = Nv->GetXaxis()->FindBin(t);
  double dt0 = t - (Nv->GetXaxis()->GetBinCenter(ti));
  TH1D *sp = CloneSpectrum();
  double sp0,sp1,sp2;
  if(dt0>0 && ti<nt)
    {
      for(int ei = 1; ei <= ne; ei++)
	{
	  sp1 = Nv->GetBinContent(ti,ei);
	  sp2 = Nv->GetBinContent(ti+1,ei);
	  sp0 = (sp2-sp1)/m_TimeBinWidth * dt0 + sp1;
	  sp->SetBinContent(ei,sp0);
	}
    }
  else if(dt0<0 && ti>1)
    {
      for(int ei = 1; ei <= ne; ei++)
	{
	  sp1 = Nv->GetBinContent(ti-1,ei);
	  sp2 = Nv->GetBinContent(ti,ei);
	  sp0 = (sp2-sp1)/m_TimeBinWidth * dt0 + sp2;
	  sp->SetBinContent(ei,sp0);
	}
    } 
  else 
    for(int ei = 1; ei <= ne; ei++)
      {
	sp->SetBinContent(ei,Nv->GetBinContent(ti,ei));
      }
  return sp; //ph
}

TH1D *SpectObj::GetTimes(double en)
{
  if (en == 0.0) return times;
  int ei = Nv->GetYaxis()->FindBin(en);

  TH1D *lc = CloneTimes();

  for(int ti = 1; ti <= nt; ti++)
    lc->SetBinContent(ti,Nv->GetBinContent(ti,ei));
  return lc; //ph
}

//////////////////////////////////////////////////
TH1D *SpectObj::Integral_E(double e1, double e2)
{
  int ei1 = Nv->GetYaxis()->FindBin(e1);
  int ei2 = Nv->GetYaxis()->FindBin(e2); 
  return Integral_E(ei1,ei2);
}

TH1D *SpectObj::Integral_E(int ei1, int ei2)
{
  //TH1D *ts = (TH1D*) times->Clone();
  //  ts->SetName("ts");  
  TH1D *ts = CloneTimes();
  for(int i = 1; i<=nt; i++)
    ts->SetBinContent(i,Nv->Integral(i,i,ei1,ei2));
  return ts; //ph
}

double SpectObj::Integral_E(TH1* Sp, double e1, double e2)
{
  // Sp in ph
  int ei1 = Sp->FindBin(e1);
  int ei2  = Sp->FindBin(e2);
  return Integral_E(Sp,ei1,ei2); //ph
}

double SpectObj::Integral_E(TH1* Sp, int ei1, int ei2)
{
  // Sp in ph
  return Sp->Integral(ei1,ei2);//ph
}

//////////////////////////////////////////////////
TH1D *SpectObj::Integral_T(double t1, double t2, double en)
{
  //nv is in ph
  int ti1 = Nv->GetXaxis()->FindBin(t1);
  int ti2 = Nv->GetXaxis()->FindBin(t2);
  int ei  = TMath::Max(1,Nv->GetYaxis()->FindBin(en));
  return Integral_T(ti1,ti2,ei); //ph
}

TH1D *SpectObj::Integral_T(double t1, double t2, double e1, double e2)
{
  //nv is in ph
  int ti1 = Nv->GetXaxis()->FindBin(t1);
  int ti2 = Nv->GetXaxis()->FindBin(t2);
  int ei1  = Nv->GetYaxis()->FindBin(e1);
  int ei2  = Nv->GetYaxis()->FindBin(e2);
  return Integral_T(ti1,ti2,ei1,ei2); //ph
}

TH1D *SpectObj::Integral_T(int ti1, int ti2, int e1)
{
  TH1D *en = CloneSpectrum();
  for(int i = e1; i<=ne; i++)
    en->SetBinContent(i,Nv->Integral(ti1,ti2,i,i));
  return en; // ph
}

TH1D *SpectObj::Integral_T(int ti1, int ti2, int e1, int e2)
{
  TH1D *en = CloneSpectrum();
  for(int i = e1; i<=e2; i++)
    en->SetBinContent(i,Nv->Integral(ti1,ti2,i,i));
  return en; // ph
}

double SpectObj::Integral_T(TH1* Pt, double t1, double t2)
{
  // Pt in ph
  int ti1 = Pt->FindBin(t1);
  int ti2 = Pt->FindBin(t2);
  return Integral_T(Pt,ti1,ti2); //ph
}

double SpectObj::Integral_T(TH1* Pt, int ti1, int ti2)
{
  // Pt in ph
  return Pt->Integral(ti1,ti2); //ph
}

//////////////////////////////////////////////////
TH1D *SpectObj::ComputeProbability(double enph)
{
  ProbabilityIsComputed=true;
  TH1D *P = CloneTimes();
  TH1D *pt = Integral_E(enph,emax); //ph
  for(int ti = 1; ti <= nt; ti++)
    {
      P->SetBinContent(ti,Integral_T(pt,0,ti)); //ph
    }
  //  std::cout<<"delete pt "<<std::endl;

  delete pt;
  return P; //ph
}

//////////////////////////////////////////////////
photon SpectObj::GetPhoton(double t0, double enph)
{
  //  photon ph;
  int ei  = TMath::Max(1,Nv->GetYaxis()->FindBin(enph));
  if(!ProbabilityIsComputed)
    Probability = ComputeProbability(enph);
  
  if (sourceType == 0 ) // Transient
    {
      double time   = 1.0e8; // > 1 year!
      double energy =  enph;
  
      if(t0 > m_Tmax)
	{
	  ph.time   = time;
	  ph.energy = energy;
	  //	  std::cout<< " ! time =  " << ph.time << " Energy  (KeV) " << ph.energy << std::endl;
  	  return ph;
	}
      
      //      TH1D *P = ComputeProbability(enph); //ph
      
      int t1 = Probability->FindBin(t0);
      int t2 = t1;
      double dt0 = t0 - Probability->GetBinCenter(t1);
      double dP0 = 0;
      double myP = m_SpRandGen->Uniform(0.9,1.1);

      if(dt0>0 && t1<nt)
	{
	  dP0 = (Probability->GetBinContent(t1+1) - Probability->GetBinContent(t1))*dt0/m_TimeBinWidth;
	}
      else if(dt0<0 && t1>1)
	{
	  dP0 = (Probability->GetBinContent(t1) - Probability->GetBinContent(t1-1))*dt0/m_TimeBinWidth;
	}
      double P0  = Probability->GetBinContent(t1) + dP0;

      while(Probability->GetBinContent(t2) < P0 + myP && t2 < nt)
	{
	  t2++;
	}
      
      if(DEBUG)
	{
	  std::cout<<Probability->GetBinCenter(t1)<<" "<<Probability->GetBinCenter(t2-1)<<" "<<Probability->GetBinCenter(t2)<<" , "
		   <<Probability->GetBinContent(t2-1)- Probability->GetBinContent(t1)<<" "
		   <<Probability->GetBinContent(t2) - Probability->GetBinContent(t1)<<std::endl;
	}

      if(t2 < nt) // the burst has finished  dp < 1 or dp >=1
	{
	  double dtf = (P0 + myP - Probability->GetBinContent(t2-1))/ 
	    (Probability->GetBinContent(t2) - Probability->GetBinContent(t2-1))*m_TimeBinWidth;
      	  time    = Probability->GetBinCenter(t2-1)+dtf;
	  energy  = Integral_T(t1,t2,ei)->GetRandom();
	}
      
      ph.time   = time;
      ph.energy = energy;
      //      delete P;  

    } 
else if (sourceType == 1) //Periodic //Max
      {
	
	if (!PeriodicSpectrumIsComputed)
	  {
	    PeriodicSpectrum = Integral_T(1,nt,ei);
	    PeriodicSpectrumIsComputed = true;
	  }
	
	double TimeToFirstPeriod = 0.0;
	double TimeFromLastPeriod = 0.0;
	int IntNumPer = 0;
	double ProbRest = 0.0;
	double nextTime = 0.0;
	double InternalTime = t0 - Int_t(t0/m_Tmax)*m_Tmax; // InternalTime is t0 reduced to a period
	//	TH1D *P = ComputeProbability(enph); //ph/m� 
	double Ptot = Probability->GetBinContent(nt) - Probability->GetBinContent(1); //Total proability in a period 

        //First checks if next photon lies in the same period
	if (((Probability->GetBinContent(nt) - Probability->GetBinContent(Nv->GetXaxis()->FindBin(InternalTime))) < 1.0 ))
	  {
	    TimeToFirstPeriod = m_Tmax - InternalTime;
	    // std::cout << "Time to first period " << TimeToFirstPeriod << std::endl;
	  }
	else
	  {
	    TimeToFirstPeriod = 0.0;
	    std::cout << "Next Photon comes within the same period " << std::endl;
	  }

	// If not, proceed to find the integer number of period required to have Prob = 1
	if (TimeToFirstPeriod != 0.0)
	  {
	    ProbRest = 1 - (Probability->GetBinContent(nt) - Probability->GetBinContent(Nv->GetXaxis()->FindBin(InternalTime)));
	    //std::cout << "Computing Integer Periods , Now ProbRest is " << ProbRest << std::endl;
	    IntNumPer = Int_t(ProbRest/Ptot);
	    //std::cout << " Integer Number of Period is " << IntNumPer <<std::endl; 
	  }
	
	// Then checks for the remnant part of the probability
	if (IntNumPer > 0 )
	  {
	    ProbRest = ProbRest - IntNumPer*Ptot;
	    // std::cout << "Computing Residual time , Now ProbRest is " << ProbRest << std::endl;
            int tBinCurrent = 1;
	    while ((Probability->GetBinContent(tBinCurrent) - Probability->GetBinContent(1)) < ProbRest ) 
	      {
		tBinCurrent++;
	      }	 
	    TimeFromLastPeriod = Nv->GetXaxis()->GetBinCenter(tBinCurrent);
	    ProbRest = ProbRest - (Probability->GetBinContent(tBinCurrent) - Probability->GetBinContent(1));
	    TimeFromLastPeriod += m_SpRandGen->Uniform()*m_TimeBinWidth - m_TimeBinWidth/2 ;
	  }
	
	//std::cout << " At End ProbRest is (should be 0 ) " << ProbRest << std::endl;


	
	
	if(DEBUG)
	  {
	    std::cout << " First Step " << t0 
		      << " to " << t0 + TimeToFirstPeriod 
		      << " (" << TimeToFirstPeriod << ")" << std::endl;  
	    std::cout << " Second Step " << t0 + TimeToFirstPeriod 
		      << " to " << t0 + TimeToFirstPeriod + IntNumPer*m_Tmax 
		      << " (" << IntNumPer*m_Tmax << "," << IntNumPer << " periods)" << std::endl;  
	    std::cout << " Third Step " << t0 + TimeToFirstPeriod + IntNumPer*m_Tmax 
		      << " to " << t0 + TimeToFirstPeriod + IntNumPer*m_Tmax + TimeFromLastPeriod 
		      << " (" << TimeFromLastPeriod << ")" << std::endl;  

	    //	    std::cout << "--> Time = " << ph.time << " s. , Energy = " << ph.energy << " keV" << std::endl; 
	  }

	// Now evaluate the Spectrum Histogram
	
	ph.time   = t0 + TimeToFirstPeriod + IntNumPer*m_Tmax +TimeFromLastPeriod;
 	ph.energy = PeriodicSpectrum->GetRandom();
	
      }
  
  if(DEBUG)  std::cout<< " New Photon at ("<<t0<<"): time =  " << ph.time << " Energy  (KeV) " << ph.energy << std::endl;
  return ph;
}

double SpectObj::GetFluence(double BL, double BH)
{
  if(BH<=0) BH = emax;
  int ei1 = Nv->GetYaxis()->FindBin(TMath::Max(emin,BL));
  int ei2 = Nv->GetYaxis()->FindBin(TMath::Min(emax,BH));
  double F=0.0;
  double en;
  for (int ei = ei1; ei<=ei2; ei++)
    {
      en   = Nv->GetYaxis()->GetBinCenter(ei);
      for(int ti = 1; ti<=nt; ti++)
	{
	  F+= Nv->GetBinContent(ti, ei)*en;//[keV]
	}  
    }
  return F*1.0e-7/(erg2meV*m_AreaDetector); //erg/cm�
  //  return Nv->Integral(0,nt,ei1,ei2,"width")*1.0e-7/(m_TimeBinWidth*erg2meV)/m_AreaDetector; //erg/cm�
}


void SpectObj::ScaleAtBATSE(double fluence)
{

  double BATSEL = 20.0;    //20 keV
  double BATSEH = 1.0e+3;  // 1 MeV
  double norm = GetFluence(BATSEL,BATSEH);//KeV/cm�
  Nv->Scale(fluence/norm);
}


double SpectObj::GetT90(double BL, double BH)
{
  if(BL<emin) BL = emin;
  if(BH<=0) BH = emax;
  TH1D* LC     = Integral_E(BL,BH); //ph
  LC->Scale(1.0/LC->Integral());
  int i=1;
  double inte = LC->GetBinContent(i);
  while(inte <= 0.05) 
    {
      inte += LC->GetBinContent(i+1);
      i++;
    }  
  double T05 = TMath::Max(0.0,LC->GetBinCenter(i));
  while(inte <= 0.95) 
    {
      inte += LC->GetBinContent(i+1);
      i++;
    }  
  double T95 = LC->GetBinCenter(i);  
  return T95-T05;
}


//////////////////////////////////////////////////
TH1D *SpectObj::N(TH1D *EN)
{
  //  std::cout<<"delete N "<<std::endl;
  gDirectory->Delete("N");
  TH1D* n = (TH1D*) EN->Clone();
  n->SetName("N");
  for(int i = 1; i <= EN->GetNbinsX();i++)
    n->SetBinContent(i,EN->GetBinContent(i)/EN->GetBinWidth(i));
  return n;
}

//////////////////////////////////////////////////
double SpectObj::flux(double time, double enph)
{
  if (time >= m_Tmax) return 1.0e-6;
  TH1D* fl = GetSpectrum(time);    //ph
  double integral = Integral_E(fl,enph,emax)/m_TimeBinWidth; //ph/s
  //  delete fl;
  return integral/m_AreaDetector;//ph/m2/s
}

double SpectObj::interval(double time, double enph)
{
  
  return  GetPhoton(time,enph).time - time;
}

double SpectObj::energy(double time, double enph)
{
  return ph.energy;
}

//////////////////////////////////////////////////
void SpectObj::SaveParameters(double tstart, std::pair<double,double> direction)
{
  
  double BATSEL = 20.0;
  double BATSEH = 1.0e+3;

  double GBML = 10.0;
  double GBMH = 25.0e+3;
  
  double LATL = 50.0e+3;
  double LATH = emax;

  double fBATSE = GetFluence(BATSEL,BATSEH);
  double fLAT   = GetFluence(LATL,LATH);
  double fGBM   = GetFluence(GBML,GBMH);
  double fTOT   = GetFluence();

  std::cout<<"**************************************************"<<std::endl;
  std::cout<<" Tstart = "<<tstart<<" T90 = "<<GetT90()<<std::endl;
  std::cout<< " GRB Direction :  l = "<<direction.first<<", b = "<<direction.second<<std::endl;
  std::cout<<" BASTE flux ("<<BATSEL<<","<<BATSEH<<") = "<<fBATSE<<" erg/cm^2"<<std::endl;
  std::cout<<" GBM   flux ("<< GBML <<","<< GBMH <<") = "<<fGBM<<" erg/cm^2"<<std::endl;
  std::cout<<" LAT   flux ("<< LATL <<","<< LATH <<") = "<<fLAT<<" erg/cm^2"<<std::endl;
  std::cout<<" GRB   flux ("<< emin <<","<< emax <<") = "<<fTOT<<" erg/cm^2"<<std::endl;
  std::cout<<"**************************************************"<<std::endl;
}

void SpectObj::GetGBM()
{
  /*
  double dt = 0.016;
  double t;
  TF1 band("band",Band,EMIN,1.0e+4,4); 
  TH1D *Sp;

  int ti = Nv->GetXaxis()->FindBin(t);

  TH1D *sp = CloneSpectrum();
  double sp0,sp1,sp2,de;

  double t = 0;
  int ti=0;
  while(t<m_Tmax)
    {
      ti = Nv->GetXaxis()->FindBin(t);
      for(int ei = 1; ei <= ne; ei++)
	{
	  sp1 = (ti==1) ? 0 : Nv->GetBinContent(ti-1,ei);
	  sp2 = Nv->GetBinContent(ti,ei);
	  sp0 = (sp2-sp1)/m_TimeBinWidth * (t - Nv->GetBinCenter(ti-1)) + sp1;
	  de  = Nv->GetYaxis()->GetBinWidth();
	  sp->SetBinContent(ei/dt/de,sp0);
	}
  for (int i=1;i<=nt;i++)
    {
      t = i*dt;
      Sp = GetSpectrum(t); 
      Fv->Fit("band","QR");
      


      for (int j=1;j<=16;j++)
	{
	  std::cout<<Sp->GetBinLowEdge(j)<<" "
		   <<Sp->GetBinCenter(j)<<" "
		   <<Sp->GetBinCenter(j)+Sp->GetBinLowEdge(j)<<" "
		   <<Sp->GetBinContent(j)<<" = "
		   <<Integral_E(Sp,j,j)<<" "<<Integral_E(Sp,j,j+1)<<std::endl;
	}
    }
  */
}
