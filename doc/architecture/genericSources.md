# genericSources

`genericSources` (`celestialSources/genericSources/`) is the part of
`celestialSources` that `gtobssim` actually uses. Every class derives from
`flux`'s `Spectrum`, lives in the **global namespace** (required by
`flux`'s `typeid`-based registration), and is registered by
`SpectrumFactoryLoader`. The XML name is the class name. See
[README.md](README.md) for how the package fits together and
`src/flux/doc/architecture/source-xml.md` for the surrounding XML.

Everything here is from reading the code at `a473bc5`.
`genericSources/src/mainpage.h` has older per-class notes; they are mostly
right for the positional syntax, but some example paths
(`$(OBSERVATIONSIMDATAPATH)/CrabTemplate.dat`, `$(FLUXROOT)/sources/…`)
are stale.

## Conventions shared by all classes

- **Units.** Energies are MeV. `flux` parameters are **photons/m²/s**,
  integrated over energy *and* over the source's sky extent (not per sr,
  despite `mainpage.h` saying sr for `Isotropic`). Times are seconds.
- **Rates.** Each class (except `FileSpectrum`) implements
  `interval(t)` itself as `-ln(1-u) / (flux · EventSource::totalArea())`,
  or from a pre-drawn list of event times. The `<source flux>` attribute
  and `flux`'s solid-angle factor are not used.
- **Directions.** Classes that implement `dir()` return **(l, b) in
  degrees**, converting from equatorial where needed. Pair them with
  `<use_spectrum frame="galaxy"/>`. Point-like classes that do not
  implement `dir()` (transients, `Pulsar`, `PeriodicSource`,
  `FileSpectrum`) need a `<celestial_dir>`/`<galactic_dir>` element.
- **Time origin.** Time-dependent classes subtract `Spectrum::startTime()`
  (set by `gtobssim` from its `startdate` parameter) before using `tstart`,
  `tstop`, `t0`. With the default `startdate` (mission start) this is MET.
- **Two `params` syntaxes.** If the string contains `=`, it is parsed as
  `key=value` pairs with `celestialSources::ConstParMap` (missing required
  keys throw; a negative `flux` throws). Otherwise it is split on `", "` and
  read positionally with `atof` (typos become 0).
- **File names** in `params` go through `facilities::Util::expandEnvVar`.
  The shipped example library uses `$(GENERICSOURCESDATAPATH)`, which only
  conda activation sets.
- **`<spectrum flux="...">`** calls `Spectrum::setFlux`, which writes the
  base-class `m_flux`. Most classes declare their own `m_flux` and ignore
  it. `FileSpectrum` uses the base member, so for a `FileSpectrum` source
  the attribute **overrides** the `flux` parameter.
  (`IsotropicFileSpectrum` and `RadialSource` wrap a separate
  `FileSpectrum` object, so it does not reach them.)

## Source classes

| Class | Kind | `params` (positional order, or keys) | Notes |
|-------|------|----------------------------------------|-------|
| `MapSource` | Diffuse from a 2-D FITS image, power-law spectrum | `flux, gamma, fitsFile [, emin=30, emax=1e5 [, lonMin, lonMax, latMin, latMax]]` or keys `flux, fitsFile, gamma, emin, emax, lonMin…latMax` | Plate-carrée image. Equatorial if `CTYPE1` starts with `R`, otherwise Galactic. Optional sub-map bounds must be all four or none. `gamma=1` divides by zero. |
| `MapCube` | Diffuse from a 3-D FITS cube with an `ENERGIES` extension (column `Energy`) | `flux, fitsFile` or keys `flux, fitsFile, lonMin…latMax` | Per pixel, the spectrum between planes is a power law fitted to adjacent planes; energies are drawn from that. Galprop/`gll_iem`-style cubes work. |
| `FileSpectrumMap` | `MapSource` image × spectrum from `FileSpectrum` | keys as `MapSource` plus `specFile, emin, emax` | `flux` comes from `FileSpectrum` (file integral if `flux=0`). |
| `Isotropic` | Isotropic (optionally within a cone) power law | `flux, gamma, emin, emax [, ra, dec, radius]` or keys | `flux` is the **total** within the region (whole sky or cone), not per sr. |
| `IsotropicFileSpectrum` | Isotropic with a `FileSpectrum` spectrum | `FileSpectrum` keys + optional `ra, dec, radius` | e.g. `isotropic_iem_v02.txt`. |
| `GaussianSource` | Elliptical 2-D Gaussian, power law | `flux, gamma, ra, dec [, major=1, minor=1, posAngle=0 [, emin, emax]]` | Axes are 68% contour semi-axes in degrees; position angle from North. |
| `RadialSource` | Azimuthally symmetric profile from file, `FileSpectrum` spectrum | keys `flux, specFile, profileFile, ra, dec` | Profile file: angle (deg), relative intensity. |
| `GaussianSpectrum` | Point source with Gaussian energy distribution | keys `flux, mean, sigma, glon, glat` | Line-like test source; uses `dir()` → `use_spectrum`. |
| `FileSpectrum` | Point source, tabulated dN/dE | keys `specFile [, flux=file integral, emin=20, emax=2e5]` | Two columns: E (MeV), dN/dE. **No `interval()`**: relies on `flux`'s Poisson fallback. Needs a `<celestial_dir>`. |
| `SimpleTransient` | Point source on in `[tstart, tstop]`, power law | `flux, gamma, tstart, tstop [, emin=30, emax=1e5]` | Event times pre-drawn in the constructor. After the last event, `interval()` returns 3.15e8 s (≈10 yr) to "turn off". |
| `TransientTemplate` | As `SimpleTransient`, light-curve shape from file | `flux, gamma, tstart, tstop, templateFile [, emin, emax]` | Template: two columns, uniformly spaced; stretched to `[tstart, tstop]`, normalised to `flux`. |
| `SpectralTransient` | Flare with a light curve of broken power-law (or log-parabola) intervals; optional EBL | keys `flux, tstart, tstop, templateFile [, emin=20, emax=2e5, lc=0, z=0, eblModel, tauScale=1, useLogParabola=0, spectrumFile, particle=gamma]` | ASCII template: `tstart tstop flux gamma1 gamma2 ebreak` per line (6 columns exactly, relative times rescaled to `[tstart, tstop]`), or a FITS file (`TIMES`, `LIGHT_CURVES` extensions; `lc` selects a curve). EBL attenuation by rejection: each drawn photon is kept with probability `exp(-tauScale·τ(E, z))`, so `flux` is the **unattenuated** flux. `tauScale` is read with `atoi`, so non-integer values truncate (0.5 → 0, i.e. no attenuation). |
| `FitsTransient` | Flare whose spectral evolution is a FITS table | keys `flux, tstart, tstop, fitsFile [, z, eblModel]` | Reads `ENERGIES` (`Energy`), `TIMES` (`Time`) and `SPECTRA` with `tip`. |
| `PeriodicSource` | Point source with sinusoidal light curve | `flux, gamma, period [, amplitude=0.5, phi0=0, emin=30, emax=1e5]` | |
| `Pulsar` | Point source with a template pulse profile | `flux, gamma, period, pdot, t0, templateFile [, phi0=0, emin=30, emax=1e5]` | Template: phase, intensity, uniformly spaced. Simple period/ṗ timing only (no barycentring or binary orbit); the physical pulsar models are in the unbuilt `Pulsar/` sub-package. |
| `SourcePopulation` | Many steady point sources from a file | `sourceFile [, eblModel [, idOffset=100000]]` or keys `sourceFile, eblModel, idOffset` | One line per source: `name ra dec flux gamma gamma2 ebreak emin emax [z]` (space, comma or tab separated). Each event picks a source by flux weight; `identifier()` becomes `idOffset + index` (→ `MC_SRC_ID`). Uses `dir()`, so `use_spectrum frame="galaxy"`. |
| `TF1Spectrum`, `TF1Map` | ROOT `TF1` formula spectra | | **Not compiled** (not in `CMakeLists.txt`; need ROOT). |

The test library `genericSources/xml/source_library.xml` (installed to
`xml/fermi/genericSources/`) has a working example of each class, plus
`TF1Spectrum`, `TF1Map` and `EventList` entries that cannot be built in a
default build.

## Shared helpers

| File | Role |
|------|------|
| `src/FitsImage.{h,cxx}` | Small cfitsio reader (`genericSources::FitsImage`): image axes, pixel values, solid angles, `findHdu`, `readColumn`. Used by `MapSource`, `MapCube`, `SpectralTransient`. |
| `src/Util.{h,cxx}` | `genericSources::Util`: `file_ok`, `readLines`, `drawFromPowerLaw`, `logInterpolate`. |
| `src/GaussianQuadrature.h` | Header-only C++ translation of SLATEC `dgaus8` (templated on a functor). It does not use `st_facilities`' f2c `dgaus8`. `test_genericSources` checks it. |
| `src/dgaus8.h` | Declares the f2c `dgaus8_` with a `long* ierr` signature. **Included by nothing.** It also disagrees with the `int`-patched f2c types used elsewhere, so don't revive it. |

## Gotchas

- **Pre-drawn event times.** `SimpleTransient`, `TransientTemplate` and
  `FitsTransient` draw all their events in the constructor from
  `flux · totalArea() · duration`; `SpectralTransient` draws one
  light-curve interval at a time, on the first `interval()` call that needs
  it. Either way the area is read when the events are drawn, so set
  `EventSource::totalArea()` before creating sources (`gtobssim` does).
  Very bright, long flares use a lot of memory.
- **Static EBL model in `SourcePopulation`.** `PointSource::s_tau` is a
  class-wide static pointer. Two populations with different `eblModel`s
  share whichever was constructed last, and destroying that population
  leaves the other with a dangling pointer (from reading the code; not
  checked at runtime).
- **`MapSource::operator()(float) const`** does not override
  `Spectrum::operator()(float)` (which is non-const). It is harmless only
  because `MapSource` also overrides `energy()`. The same pattern is in
  `Isotropic`, `GaussianSource` and others; don't rely on `operator()`
  being called polymorphically.
- **`flux` version matters for `escale`.** `<spectrum escale="GeV">` used to
  make `flux` multiply energies by 1000; since `flux` `1d5fc58` it does not
  (see `src/flux/doc/architecture/`). The example `filespectrum_test` and
  `filespectrummap_test` entries use `escale="GeV"` with an MeV data file.
  Use `escale="MeV"` for every `genericSources` class.
- **`tauScale` is an integer** in `SpectralTransient` (`atoi`), see the
  table.
- **Missing shipped data.** `filespectrummap_test` (and `TF1Map`) point at
  `$(EXTFILESSYS)/galdiffuse/EGRET_diffuse_gal.fits`, which is not
  installed, and `eventlist_test` at a `.lis` file that does not exist.

## Adding a source class

1. Create `genericSources/genericSources/X.h` and `genericSources/src/X.cxx`.
   `class X : public Spectrum` in the global namespace, with
   `X(const std::string& params)`.
2. Implement `particleName()`, `title()`, `energy(t)` (MeV), `interval(t)`
   (use `EventSource::totalArea()`), `flux(t)`, and `dir(E)` returning
   (l, b) if the source supplies its own directions. Parse `params` with
   `celestialSources::ConstParMap`; accept the positional form only for
   backward compatibility.
3. Add the factory function in `X.cxx`:
   ```cpp
   ISpectrumFactory& XFactory() { static SpectrumFactory<X> f; return f; }
   ```
4. Add `src/X.cxx` to `genericSources/CMakeLists.txt`.
5. Declare `ISpectrumFactory& XFactory();` and call `load(XFactory());` in
   `celestialSources/src/SpectrumFactoryLoader.cxx`. Without this, `X` is
   silently missing from `gtobssim`.
6. Add an example to `genericSources/xml/source_library.xml`, any data to
   `genericSources/data/` (installed to `data/genericSources`, reached via
   `$(GENERICSOURCESDATAPATH)`), and the name to `setSources()` and
   `load_sources()` in `genericSources/src/test/test.cxx`.
7. Document the class in this file and in the user documentation for
   `gtobssim` (`fermitools-fhelp`).
