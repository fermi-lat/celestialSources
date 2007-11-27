/**
 * @file main.cxx
 * @brief Test program to exercise genericSources
 * @author J. Chiang
 * $Header$
 */

#ifdef TRAP_FPE
#include <fenv.h>
#endif

#include <cstdlib>

#include <fstream>

#include "astro/GPS.h"
#include "astro/PointingTransform.h"
#include "astro/SkyDir.h"

#include "flux/CompositeSource.h"
#include "flux/FluxMgr.h"

#include "facilities/commonUtilities.h"

#include "TestUtil.h"

ISpectrumFactory & FitsTransientFactory();
ISpectrumFactory & GaussianSourceFactory();
ISpectrumFactory & GRBmanagerFactory();
ISpectrumFactory & IsotropicFactory();
ISpectrumFactory & MapCubeFactory();
ISpectrumFactory & MapSourceFactory();
ISpectrumFactory & PeriodicSourceFactory();
ISpectrumFactory & PulsarFactory();
ISpectrumFactory & SimpleTransientFactory();
ISpectrumFactory & SourcePopulationFactory();
ISpectrumFactory & TransientTemplateFactory();
ISpectrumFactory & TF1SpectrumFactory();
ISpectrumFactory & TF1MapFactory();
ISpectrumFactory & FileSpectrumFactory();
ISpectrumFactory & FileSpectrumMapFactory();
ISpectrumFactory & RadialSourceFactory();
//ISpectrumFactory & EventListFactory();

class TestApp {

public:

   TestApp() : m_fluxMgr(0), m_count(2000), m_compositeSource(0) {}

   ~TestApp() throw() {
      try {
         delete m_fluxMgr;
         delete m_compositeSource;
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
      } catch (...) {
         std::cerr << "~TestApp:: Unknown exception." << std::endl;
      }
   }

   void parseCommandLine(int iargc, char * argv[]);
   void setXmlFiles();
   void setSources();
   void createEvents(const std::string & filename);

   static void load_sources();
   static HepRotation instrumentToCelestial(double time);

private:

   FluxMgr * m_fluxMgr;
   unsigned long m_count;
   CompositeSource * m_compositeSource;

};

int main(int iargc, char * argv[]) {
#ifdef TRAP_FPE
   feenableexcept (FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
#endif

   facilities::commonUtilities::setupEnvironment();
   try {
      TestApp testApp;
      
      testApp.parseCommandLine(iargc, argv);
      testApp.load_sources();
      testApp.setXmlFiles();
      testApp.setSources();
      testApp.createEvents("test_data.dat");

   } catch (std::exception & eObj) {
      std::cout << eObj.what() << std::endl;
      return 1;
   }

   if (!Util_tests()) {
      return 1;
   }
}

void TestApp::setXmlFiles() {
   std::vector<std::string> fileList;

   std::string srcLibrary(facilities::commonUtilities::joinPath(facilities::commonUtilities::getXmlPath("genericSources"), "source_library.xml"));
   fileList.push_back(srcLibrary);

   m_fluxMgr = new FluxMgr(fileList);
   m_fluxMgr->setExpansion(1.);

// You'd think one could use a method of FluxMgr to set the total area
// without forcing the use of a temporary object...
   double totalArea(6.);
   EventSource * defaultSource = m_fluxMgr->source("default");
   defaultSource->totalArea(totalArea);
   delete defaultSource;
}

void TestApp::parseCommandLine(int iargc, char * argv[]) {
   if (iargc > 1) {
      m_count = static_cast<long>(std::atof(argv[1]));
   }
}


void TestApp::setSources() {
   char * srcNames[] = {
                        "Galactic_diffuse",
                        "Galactic_diffuse_0",
                        "simple_transient",
                        "transient_template",
                        "_3C279_June1991_flare",
                        "PKS1622m297_flare",
                        "periodic_source",
                        "Crab_Pulsar",
                        "Geminga_Pulsar",
                        "gaussian_source",
                        "Extragalactic_diffuse",
                        "map_cube_source",
                        "map_cube_source_0",
                        "fits_spectrum",
                        "source_population",
			"tf1spectrum_test",
 			"tf1map_test",
                        "filespectrummap_test",
 			"filespectrum_test",
                        "radial_source"
   };
   size_t nsrcNames(sizeof(srcNames)/sizeof(char*));
   std::vector<std::string> sourceNames(srcNames, srcNames + nsrcNames);

   m_compositeSource = new CompositeSource();
   unsigned long nsrcs(0);
   for (std::vector<std::string>::const_iterator name = sourceNames.begin();
        name != sourceNames.end(); ++name) {
      EventSource * source(0);
      if ((source = m_fluxMgr->source(*name))) {
         std::cerr << "adding source " << *name << std::endl;
         m_compositeSource->addSource(source);
         nsrcs++;
      } else {
         std::cerr << "Failed to find a source named \""
                   << *name << "\"" << std::endl;
      }
   }
   if (nsrcs == 0) {
      std::cerr << "No valid sources have been created. Exiting...." 
                << std::endl;
      exit(-1);
   }
}

void TestApp::createEvents(const std::string & filename) {
   EventSource * newEvent(0);
   double currentTime(0);
   std::ofstream outputFile(filename.c_str());
   for (unsigned int i = 0; i < m_count; i++) {
      newEvent = m_compositeSource->event(currentTime);
      double interval = m_compositeSource->interval(currentTime);
      currentTime += interval;
      Hep3Vector launchDir = newEvent->launchDir();
      
      HepRotation rotMatrix = instrumentToCelestial(currentTime);
      astro::SkyDir srcDir(rotMatrix(-launchDir), astro::SkyDir::EQUATORIAL);
      
      outputFile << m_compositeSource->findSource().c_str()<<"  "
		 << newEvent->particleName()<<"  "
		 << newEvent->time() << "  "
                 << newEvent->energy() << "  "
                 << srcDir.ra() << "  "
                 << srcDir.dec() << "\n";
   }
   outputFile.close();
}

void TestApp::load_sources() {
   FitsTransientFactory();
   GaussianSourceFactory();
   IsotropicFactory();
   MapCubeFactory();
   MapSourceFactory();
   PeriodicSourceFactory();
   PulsarFactory();
   SimpleTransientFactory();
   SourcePopulationFactory();
   TransientTemplateFactory();
   TF1SpectrumFactory();
   TF1MapFactory();
   FileSpectrumFactory();
   FileSpectrumMapFactory();
   RadialSourceFactory();
//   EventListFactory();
}

HepRotation TestApp::instrumentToCelestial(double time) {
   astro::GPS *gps = astro::GPS::instance();
   gps->time(time);

   astro::PointingTransform transform(gps->zAxisDir(), gps->xAxisDir());
   return transform.localToCelestial();
}
