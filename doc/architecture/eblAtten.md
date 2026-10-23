# eblAtten

`eblAtten` (`celestialSources/eblAtten/`) computes the optical depth τ(E, z)
to γγ pair production on the extragalactic background light for 20 EBL
models. It is a small static library (~0.8k lines) with two clients:

- `genericSources` (`SpectralTransient`, `FitsTransient`,
  `SourcePopulation`) attenuates simulated photons in `gtobssim`.
- `Likelihood` wraps it as the `EblAtten` spectral function
  (`dN/dE × exp(-tau_norm · τ(E, z))`, parameters `tau_norm`, `redshift`,
  `ebl_model`) and in the `gtebl` tool. Likelihood links `eblAtten`
  directly, not the `celestialSources` container library.

So a change here changes both simulations and fits.

Everything below is from reading the code at `a473bc5`.

## Interface

```cpp
#include "eblAtten/EblAtten.h"
IRB::EblAtten tau(IRB::Finke);        // throws std::runtime_error for an unknown model
float t = tau(energy_MeV, redshift);  // energy in MeV
```

- `IRB::EblAtten::operator()(float energy, float redshift)` takes energy in
  **MeV** and divides by 1000 before calling the model functions, which
  work in GeV. The header comment says "Photon energy (GeV)"; it is wrong.
- Everything is `float`.
- `IRB::EblModel` values are **integers in user files**: `eblModel=` in
  `gtobssim` XML and `ebl_model` in Likelihood XML models. Never reorder
  the enum; append.

## Models

| ID | Enum | Source of τ | Table energy range (GeV) | Lowest tabulated z |
|---:|------|-------------|--------------------------|--------------------|
| 0 | `Kneiske` | `opdep_KNEISKEetal2004_bestfit.dat` | to 1e8 | 0.1 |
| 1 | `Primack05` | Coefficients compiled into `Primack05.cxx` | — | — |
| 2 | `Kneiske_HighUV` | `opdep_KNEISKEetal2004_highUV.dat` | to 1e8 | 0.1 |
| 3 | `Stecker05` | Polynomial fit in `IRB_routines.cxx` | — | 0 |
| 4 | `Franceschini` | `opdep_Franceschini_2008.dat` | to 1.66e3 | 0.01 |
| 5 | `Finke` | `opdep_Finke_2009.dat` | 1 – 1e3 | **0.1** |
| 6 | `Gilmore` | Deprecated alias of 13 | | |
| 7 | `Stecker05_FE` | Polynomial fit | — | 0 |
| 8 | `SalamonStecker` | `opdep_SalamonStecker98.dat` | to 500 | **0.1** |
| 9 | `Generic` | Analytic (J. Finke); 0 below 10 GeV | — | — |
| 10 | `Dominguez11` | `opdep_DOMINGUEZetal_2011.dat` | to 3.1e4 | 0.0106 |
| 11 | `KneiskeDole10` | `opdep_KNEISKEandDOLE_2010_noCMB.dat` | to 1.1e8 | 0.0106 |
| 12 | `KneiskeDole10_CMB` | `opdep_KNEISKEandDOLE_2010.dat` | to 1.1e8 | 0.0106 |
| 13 | `Gilmore09` | `opdep_GILMOREetal_2009.dat` | to 1e3 | 0.01 |
| 14 | `Gilmore12_fiducial` | `opdep_fiducial_Gilmore2012.dat` (**MeV** column) | to 1e5 | 0.01 |
| 15 | `Gilmore12_fixed` | `opdep_fixed_Gilmore2012.dat` (**MeV** column) | to 1e5 | 0.01 |
| 16 | `Scully14_lowOp` | `opdep_SCULLYetal2014_lowOp.dat` | to 1.6e3 | 0.01 |
| 17 | `Scully14_highOp` | `opdep_SCULLYetal2014_highOp.dat` | to 1.6e3 | 0.01 |
| 18 | `Inoue13` | `opdep_INOUEetal_2013.dat` | to 1e3 | 0.01 |
| 19 | `HelgasonKashlinsky12` | `opdep_HELGASONandKASHLINSKY_2012.dat` | to 500 | 0.01 |

References for each model are in `data/README_EBLmodels.txt`.

## Table models (`AsciiTableModel`)

- **Location.** Each `calcX` builds a function-local `static
  AsciiTableModel` from `getDataPath("eblAtten")` =
  `$FERMI_INST_DIR/refdata/fermi/eblAtten`. `CMakeLists.txt` installs
  `data/` there. The file is read on the **first** call for that model and
  kept for the life of the process.
- **Format.** The first line is a header whose redshift list starts at
  character 16 (`"#E_gam (GeV), z="` is exactly 16 characters, so the
  parser does `substr(16)` and splits on commas). Each further line is
  `E τ(z1) τ(z2) …`. Lines with the wrong number of columns are **silently
  skipped**.
- **Interpolation.** Linear in z between the bracketing redshifts, then
  log-log in E between the bracketing energies. If either bracketing τ is 0
  the result is 0.

## Gotchas

- **τ = 0 at and below the first tabulated redshift.** `AsciiTableModel::value`
  returns 0 if `z ≤ z_first` (this became `≤` in `fade9a3`, PR #8, to stop
  an out-of-range index at exactly `z_first`). There is no interpolation
  toward z = 0. For `Finke`, `Kneiske`, `Kneiske_HighUV` and
  `SalamonStecker`, whose first column is z = 0.1, any source with
  z ≤ 0.1 gets **no attenuation**. For the others the cut is at about
  z = 0.01.
- **τ = 0 at and below the first tabulated energy.**
- **Above the table range it throws.** An energy above the last row, or a
  redshift above the last column, throws `std::runtime_error("Selected
  energy outside range of <file>")` (or redshift). The usable energy limit
  is as low as 500 GeV (`SalamonStecker`, `HelgasonKashlinsky12`) or 1 TeV
  (`Finke`, `Gilmore09`, `Inoue13`). A `Likelihood` fit or a `gtobssim` run
  whose energy range goes higher fails with that exception (from reading
  the code; not checked at runtime).
- **Unit mix-up risk.** The Gilmore 2012 tables are in MeV, so
  `calcGilmore12_*` multiply the GeV energy back by 1000. Any new table
  must say which unit it uses.
- **Missing data files** show up only on first use of that model, as an
  exception from `st_facilities::Util::readLines` (or an empty table and an
  index error), not when `EblAtten` is constructed.
- **Not thread-safe.** Lazy `static` initialisation is thread-safe in
  C++11, but `EblAtten::s_model_Ids` is a static `std::map` filled in the
  first constructor without a lock.

## Test

`test_eblAtten` (`src/test/test.cxx`, installed to `bin/`, not in CTest)
prints τ for all 20 models on 50 energies from 1 to 500 GeV and
z ∈ {0, 0.01, 0.1, 0.5, 1, 2, 3}. It has no assertions. It needs an
install and `FERMI_INST_DIR`, and fails only if something throws.

## Adding a model

1. Put the table in `data/opdep_<NAME>.dat` with the 16-character header
   convention, and a `readme_` file if the authors supplied one.
2. Append an enum value to `IRB::EblModel` (never insert in the middle).
3. Add `float calcX(float energy, float redshift)` to `IRB_routines.cxx`
   (copy an `AsciiTableModel` one; convert units if the table is not in
   GeV), declare it in `EblAtten.cxx`, add a `case` to
   `EblAtten::operator()` and a description to `s_model_Ids`.
4. Add the model to `test_eblAtten`, to `data/README_EBLmodels.txt`, and to
   the `ebl_model` documentation in `Likelihood` and `fermitools-fhelp`.
