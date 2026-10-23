# Unbuilt and unreachable sub-packages

About two thirds of `celestialSources` (~21k of ~30k lines) is not compiled
in a default build, and one sub-package that is compiled cannot be reached.
This page records what each one is, why it is not available, and what
reviving it would take. Everything is from reading the code and CMake at
`a473bc5`; nothing here was built.

## Summary

| Sub-package | Lines | Needs ROOT | Has `CMakeLists.txt` | In top-level CMake | Status |
|-------------|------:|:----------:|:--------------------:|--------------------|--------|
| `microQuasar` | ~0.8k | no | yes | always `add_subdirectory` | **Built, unreachable** |
| `EarthPhenom` | ~1.1k | yes | yes | only if `FERMI_BUILD_ROOT` | Built with ROOT, never linked |
| `GRB` | ~7.8k | yes | **no** | commented out | Dead |
| `GRBobs` | ~5.0k | yes | **no** | commented out | Dead |
| `GRBtemplate` | ~1.9k | yes | **no** | commented out | Dead |
| `Pulsar` | ~4.1k | yes | **no** | commented out | Dead |
| `SpectObj` | ~0.9k | yes | **no** | commented out | Dead (support library for GRB*/Pulsar) |
| `pySources` | ~0.1k | no | **no** | commented out | Dead (old SWIG + numarray) |
| `DarkMatter` | ~0.8k | no | **no** | not mentioned | Dead (data + stand-alone halo code) |
| `genericSources` `TF1Spectrum`, `TF1Map` | ~0.3k | yes | — | not in source list | Dead |

Uncommenting an `add_subdirectory` line in `celestialSources/CMakeLists.txt`
is not enough for the dead ones: those directories have no `CMakeLists.txt`
at all (only the old SCons `SConscript`, `*Lib.py` and CMT `cmt/` files).

## microQuasar

A `Spectrum` subclass (R. Dubois) for X-ray binaries / microquasars:
emission modulated by orbital phase (two spectral-index regions) and by a
disk cycle with jet-on periods; 16 positional parameters documented in `microQuasar/xml/mq.xml`
(LS I +61 303 example).

- `microQuasar/CMakeLists.txt` builds `libmicroQuasar.a` and
  `test_microQuasar` and installs `xml/mq.xml` to
  `xml/fermi/microQuasar`.
- **Nothing links `microQuasar`**, and `load(microQuasarFactory())` is
  commented out in `SpectrumFactoryLoader.cxx`, so `gtobssim` cannot
  create one ("Unknown Spectrum: microQuasar").
- To enable it: link `microQuasar` into `celestialSources` and uncomment
  the declaration and the `load(...)` call. Check its `flux()` (returns 0)
  and `solidAngle()` (returns 0) against `flux`'s current rate logic first.

## EarthPhenom

Phenomenological Earth-limb and inner-Earth gamma-ray emission
(K. Bechtol): `EarthPhenomLimb` and `EarthPhenomInner`, ROOT `TGraph`-based.

- Built only with `FERMI_BUILD_ROOT=ON`, which is broken (see
  [README.md](README.md#invariants-and-gotchas)).
- Even then, `EarthPhenom` is not linked into `celestialSources` and its
  CMake has no `install` rules (headers, XML, library, test).

## GRB, GRBobs, GRBtemplate

Three generations of gamma-ray-burst simulators from the GLAST era
(N. Omodei et al.):

- `GRB`: a physical fireball/shock model (`GRBmanager`), with `GRBmaker`
  and `LatGRBAlert` helpers. Reads `getDataPath("GRB")/GRBParam.txt`.
- `GRBobs`: a phenomenological model (`GRBobsmanager`) built from Band
  spectra and pulse prescriptions from the BATSE literature. Reads `GRBOBS_SINGLEPULSE`,
  `GRBOBS_DISABLE_SPECEVOL`, `DISABLE_SAA`.
- `GRBtemplate`: replays a spectral-temporal template file
  (`GRBtemplateManager`, `$(GRBTEMPLATEDATAPATH)/…` in its XML). Reads
  NaI/BGO energy grids from `getDataPath("GRBtemplate")`.

All three depend on ROOT and `SpectObj`.
`GRBTEMPLATEDATAPATH` is not set by conda activation or by
`setupEnvironment()` (the sub-package is not in `FERMI_COMPONENTS`).

## Pulsar

`PulsarSpectrum` (N. Omodei, M. Razzano): physically motivated pulsar
light curves and spectra, with timing noise, binary orbits and barycentric
corrections, driven by `data/*DataList.txt` and time profiles. Reads many
environment variables (`PULSARDATA`, `SKYMODEL_DIR`, `PULSARROOT`,
`PULSAR_OUTPUT_LEVEL`, `PULSAR_NO_DB`, `PULSAROUTFILES`, `PULSAR_OUT_BIN`,
`PULSAR_OUT_BARY`, `PULSAR_OUT_TNOISE`, `PULSAR_EPH`). Depends on ROOT and
`SpectObj`. Its factory is commented out even in the ROOT branch of the
loader.

Do not confuse it with `genericSources`' `Pulsar` class, which is a simple
template-profile pulsar and **is** available.

## SpectObj

A ROOT-based spectral object (`TH2D` time × energy matrices) used by
the GRB packages and `Pulsar` to sample photons. Uses `eblAtten` and reads
`getDataPath("SpectObj")/EBLmodel.dat`.

## pySources

A SWIG module (`src/pySources.in`) exposing `flux`/`genericSources`/GRB
classes to Python, and two scripts using the long-obsolete `numarray`
module. No build rules.

## DarkMatter

A repository of dark-matter annihilation spectra (text files meant for
`FileSpectrum`) and a stand-alone halo line-of-sight integrator
(`makeMap.cxx`, `LineOfSight`, `Profile`). No build rules and no data
installed. For DM simulations today, use `FileSpectrum` / `FileSpectrumMap`
with your own spectrum file.

## TF1Spectrum, TF1Map (in genericSources)

Spectra defined by a ROOT `TF1` formula string (and the map variant).
Their sources are in `genericSources/src/` but not in
`genericSources/CMakeLists.txt`, and `BUILD_WITHOUT_ROOT` (inherited from
`tip`) compiles them out of the test.

## If you revive one

1. Write a `CMakeLists.txt` modelled on `genericSources/CMakeLists.txt`
   (static library, PUBLIC include dir, install headers/data/xml to
   `include`, `refdata/fermi/<name>` if it uses `getDataPath`, and
   `xml/fermi/<name>`).
2. Fix the ROOT branch of `celestialSources/CMakeLists.txt` (scope keyword
   on `target_sources`, link the sub-libraries into `celestialSources`).
3. Port XML-facing code to the current `flux` behaviour (see
   `src/flux/doc/architecture/source-xml.md`), in particular energies for
   `escale="GeV"`.
4. Add the factory call to `SpectrumFactoryLoader.cxx`.
5. Check data validity dates (ephemerides, pulsar databases), which in
   these packages date from 2005–2012.
