#include <fstream>
#include <iostream>

#include "GRBobs/GRBobsConstants.h"
#include "GRBobs/GRBobsengine.h"
#include "GRBobs/GRBobsSim.h"
#include "GRBobs/GRBobsPulse.h"

#include "TFile.h"

#include "TCanvas.h"
using namespace ObsCst;
//////////////////////////////////////////////////
GRBobsSim::GRBobsSim(GRBobsParameters *params)
  : m_params(params)
{
  m_GRBengine = new GRBobsengine(params);
  m_fluence = m_params->GetFluence();
}

void GRBobsSim::GetUniqueName(const void *ptr, std::string & name)
{
  std::ostringstream my_name;
  my_name << reinterpret_cast<int> (ptr);
  name = my_name.str();
  gDirectory->Delete(name.c_str());
}
TH2D* GRBobsSim::MakeGRB()
{
  double *e = new double[Ebin +1];
  for(int i = 0; i<=Ebin; i++)
    {
     e[i] = emin*pow(de,1.0*i); //keV
    }
  
  //////////////////////////////////////////////////  
  double s_TimeBinWidth=TimeBinWidth;
  std::vector<GRBobsPulse*> Pulses = m_GRBengine->CreatePulsesVector();
  m_tfinal=0.0;
  
  std::vector<GRBobsPulse*>::iterator pos;
  for(pos = Pulses.begin(); pos !=  Pulses.end(); ++pos)
    {
      m_tfinal=TMath::Max(m_tfinal,(*pos)->GetEndTime());      
    }
  
  m_tbin = TMath::Max(10,int(m_tfinal/s_TimeBinWidth));
  m_tbin = TMath::Min(10000,m_tbin);
  s_TimeBinWidth = m_tfinal/m_tbin;
  //  double dt = m_tfinal/(m_tbin-1);
  gDirectory->Delete("Nv");
  m_Nv = new TH2D("Nv","Nv",m_tbin,0.,m_tfinal,Ebin, e);
  
  std::string name;
  GetUniqueName(m_Nv,name);
  m_Nv->SetName(name.c_str());
  
  double t = 0.0;
  for(int ti = 0; ti<m_tbin; ti++)
    {
      t = ti*s_TimeBinWidth;
      for(int ei = 0; ei < Ebin; ei++)
	{
	  double nv = 0.0;//m_Nv->GetBinContent(ti+1, ei+1);
	  
	  for(pos = Pulses.begin(); pos !=  Pulses.end(); ++pos)
	    {	
	      nv += (*pos)->PulseShape(t,e[ei]);
	    }
	  m_Nv->SetBinContent(ti+1, ei+1, nv);
	  // [ph/(cm� s keV)]
	}
    }

  TH2D *nph = Nph(m_Nv); //ph/cm�
  
  int ei1 = nph->GetYaxis()->FindBin(BATSE1);
  int ei2 = nph->GetYaxis()->FindBin(BATSE5);
  double F=0.0;
  double en;
  for (int ei = ei1; ei<=ei2; ei++)
    {
      en   = nph->GetYaxis()->GetBinCenter(ei);
      for(int ti = 1; ti<=m_tbin; ti++)
	{
	  F+= nph->GetBinContent(ti, ei) * en;//[keV/cm�]
	}  
    }
  double norm = F*1.0e-3/(erg2meV); //erg/cm�  
  //  double norm = nph->Integral(0,m_tbin,ei1,ei2,"width")*(1.0e-3)/(erg2meV)/s_TimeBinWidth; //erg/m�
  
  // IMPORTANT m_Nv has to  be in [ph/(m� s keV)]
  m_Nv->Scale(1.0e4 * m_fluence/norm);
  Pulses.erase(Pulses.begin(), Pulses.end());
  delete e;
  delete nph;
  //SaveNv(m_Nv);
  return m_Nv;
}
//////////////////////////////////////////////////
TH2D *GRBobsSim::Nph(const TH2D *Nv)
{

  TH2D *Nph = (TH2D*) Nv->Clone(); // [ph/(m� s keV)]  
  std::string name;
  GetUniqueName(Nph,name);
  Nph->SetName(name.c_str());
  
  double dei;
  double deltat = Nv->GetXaxis()->GetBinWidth(1);
  
  for (int ei = 0; ei<Ebin; ei++)
    {
      dei   = Nv->GetYaxis()->GetBinWidth(ei+1);
      for(int ti = 0; ti<m_tbin; ti++)
	{
	  Nph->SetBinContent(ti+1, ei+1, 
			     Nph->GetBinContent(ti+1, ei+1)*dei*deltat); //[ph/(m�)]  
	}   
    }
  return Nph;
}

//////////////////////////////////////////////////
void GRBobsSim::SaveNv()
{
  
  m_Nv->SetXTitle("Time [s]");
  m_Nv->SetYTitle("Energy [keV]");
  m_Nv->SetZTitle("N_{v} [ph/m^2/s/keV]");
  m_Nv->GetXaxis()->SetTitleOffset(1.5);
  m_Nv->GetYaxis()->SetTitleOffset(1.5);
  m_Nv->GetZaxis()->SetTitleOffset(1.2);
  m_Nv->GetXaxis()->CenterTitle();
  m_Nv->GetYaxis()->CenterTitle();
  m_Nv->GetZaxis()->CenterTitle();
  
  char root_name[100];
  sprintf(root_name,"grbobs_%d.root",(int)m_params->GetGRBNumber());
  std::cout<<" Saving "<<root_name<<std::endl;
  TFile mod(root_name,"RECREATE");
  std::string name = m_Nv->GetName();
  m_Nv->SetName("Nv"); // I need a default name.
  m_Nv->Write();
  mod.Close();
  m_Nv->SetName(name.c_str());
  
};

//////////////////////////////////////////////////
double BandObsF(double *var, double *par)
{
  // GRB function from Band et al.(1993) ApJ.,413:281-292
  double a  = par[0];
  double b  = par[1];
  double E0 = pow(10.,par[2]);
  double NT = pow(10.,par[3]);
  
  double E   = var[0];
  double C   = pow((a-b)*E0/100.0,a-b)*exp(b-a);
  double H   = (a-b) * E0;
  if(E <= H) 
    return NT *  pow(E/100.0,a) * exp(-E/E0);
  return C* NT * pow(E/100.0,b); // ph cm^(-2) s^(-1) keV
}

void GRBobsSim::GetGBMFlux()
{
  double *e = new double[Ebin +1];
  for(int i = 0; i<=Ebin; i++)
    {
      e[i] = emin*pow(de,1.0*i); //keV
    }
  //////////////////////////////////////////////////
  // GBM Spectrum:
  TF1 band("grb_f",BandObsF,emin,1.0e+4,4); 
  band.SetParNames("a","b","Log10(E0)","Log10(Const)");
  band.SetParameters(-1.0,-2.25,2.5,-3.0);
  band.SetParLimits(0,-3.0,0.0);
  band.SetParLimits(1,-10.0,0.0);
  band.SetParLimits(2,log10(emin),4.0);
  band.SetParLimits(3,-10.0,3.0);
  double a,b,E0,Const;
  TH1D GBM("GBM","GBM",Ebin, e);
  GBM.SetMinimum(1e-5);
  GBM.SetMaximum(1e6);
  double t=0;
  double dt = m_Nv->GetXaxis()->GetBinWidth(1);
  double tbin = m_Nv->GetXaxis()->GetNbins();
  std::ofstream os("GBM_spectrum.txt",std::ios::out);
  os<<" t   Norm   alf   beta  E_p "<<std::endl;
  for(int ti = 0; ti<tbin; ti++)
    {
      t = ti*dt;
      for(int ei = 0; ei < Ebin; ei++)
	{
	  double nv = m_Nv->GetBinContent(ti+1, ei+1); // [ph/(m� s keV)]
	  GBM.SetBinContent(ei+1, /*e[ei] * e[ei] */ nv * 1.0e-4); // [(ph)/(cm� s keV)]
	}
      GBM.Fit("grb_f","rq");
      
      a=band.GetParameter(0);
      b=band.GetParameter(1);
      E0=pow(10.,band.GetParameter(2));
      Const=pow(10.,band.GetParameter(3));
      band.SetParameters(a,b,band.GetParameter(2),band.GetParameter(3));
      //Ep=(a+2)*E0; 
      os<<t<<" "<<Const<<" "<<a<<" "<<b<<" "<<E0<<" "<<std::endl;
      //      std::cout<<t<<" "<<Const<<" "<<a<<" "<<b<<" "<<E0<<" "<<std::endl;
      //band.Draw("same");
      gPad->SetLogx();
      gPad->SetLogy();
      gPad->Update();
      /*
	TString gbmFlux= "GBMFlux";
	gbmFlux+=ti;
	gbmFlux+=".gif";
	if(ti%10==0) gPad->Print(gbmFlux);
      */
    }   
  os.close();
  //////////////////////////////////////////////////
  delete[] e;
}
