/** 
* @file SpectrumFactoryLoader.cxx
* @brief Load the external spectrum factory objects
*
*  $Header$
*/

#include <iostream>
#include <vector>

#include "celestialSources/SpectrumFactoryLoader.h"
#ifndef BUILD_WITHOUT_ROOT
#include "celestialSources/TRandom4.h"
#endif

#include "flux/ISpectrumFactory.h"

// declare the external factories.
auto FitsTransientFactory() -> ISpectrumFactory &;
auto GaussianSourceFactory() -> ISpectrumFactory &;
auto GaussianSpectrumFactory() -> ISpectrumFactory &;
auto GRBmanagerFactory() -> ISpectrumFactory &;
auto GRBobsmanagerFactory() -> ISpectrumFactory &;
auto GRBtemplateManagerFactory() -> ISpectrumFactory &;
auto IsotropicFactory() -> ISpectrumFactory &;
auto IsotropicFileSpectrumFactory() -> ISpectrumFactory &;
auto MapSourceFactory() -> ISpectrumFactory &;
auto MapCubeFactory() -> ISpectrumFactory &;
auto PeriodicSourceFactory() -> ISpectrumFactory &;
auto PulsarFactory() -> ISpectrumFactory &;
//ISpectrumFactory & PulsarSpectrumFactory();
auto SourcePopulationFactory() -> ISpectrumFactory &;
auto SimpleTransientFactory() -> ISpectrumFactory &;
auto SpectralTransientFactory() -> ISpectrumFactory &;
auto TransientTemplateFactory() -> ISpectrumFactory &;
auto TF1SpectrumFactory() -> ISpectrumFactory &;
auto TF1MapFactory() -> ISpectrumFactory &;
auto FileSpectrumFactory() -> ISpectrumFactory &;
auto FileSpectrumMapFactory() -> ISpectrumFactory &;
//ISpectrumFactory & microQuasarFactory();
auto RadialSourceFactory() -> ISpectrumFactory &;
auto EarthPhenomLimbFactory() -> ISpectrumFactory &;
auto EarthPhenomInnerFactory() -> ISpectrumFactory &;

SpectrumFactoryLoader::SpectrumFactoryLoader() {
// Replace ROOT's global TRandom instance with our local version that
// uses the CLHEP engines underneath.
#ifndef BUILD_WITHOUT_ROOT
   gRandom = new TRandom4();
#endif

   load(FitsTransientFactory());
#ifndef BUILD_WITHOUT_ROOT
   load(GRBmanagerFactory());
   load(GRBobsmanagerFactory());
   load(GRBtemplateManagerFactory());
#endif
   load(GaussianSourceFactory());
   load(GaussianSpectrumFactory());
   load(IsotropicFactory());
   load(IsotropicFileSpectrumFactory());
   load(MapSourceFactory());
   load(MapCubeFactory());
   load(PeriodicSourceFactory());
   load(PulsarFactory());
   load(SimpleTransientFactory());
   load(SourcePopulationFactory());
   load(SpectralTransientFactory());
   load(TransientTemplateFactory());
#ifndef BUILD_WITHOUT_ROOT
   //load(PulsarSpectrumFactory());
   load(TF1SpectrumFactory());
   load(TF1MapFactory());
   load(EarthPhenomLimbFactory());
   load(EarthPhenomInnerFactory());
#endif
   load(FileSpectrumFactory());
   load(FileSpectrumMapFactory());
   // load(microQuasarFactory());
   load(RadialSourceFactory());
}

void SpectrumFactoryLoader::load(ISpectrumFactory& factory) {
   m_names.push_back( factory.name() );
}
