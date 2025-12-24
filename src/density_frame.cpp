#ifdef _OPENMP
#include <omp.h>
#endif

#include <DFHydro/DFHydroEOS.hpp>
#include <DFHydro/Vischydro.hpp>
#include <cassert>
#include <cmath>
#include <memory>

#include "cell.h"
#include "data.h"
#include "density_frame.h"
#include "eos.h"
#include "evolve.h"
#include "grid.h"
#include "util.h"

using Util::hbarc;
using Util::map_1d_idx_to_2d;
using Util::map_2d_idx_to_1d;

class DFHydroMusicEOS : public DFHydro::EOS {
public:
  DFHydroMusicEOS(const ::EOS &eosIn, const InitData &DATA_in)
      : eos(eosIn), transport(eosIn, DATA_in) {}

  double get_cs2(double e, double rhob) const override {
    return (eos.get_cs2(e, rhob));
  }
  double get_temperature(double e, double rhob) const override {
    return (eos.get_temperature(e, rhob));
  }
  double get_pressure(double e, double rhob) const override {
    return (eos.get_pressure(e, rhob));
  }
  double get_entropy(double e, double rhob) const override {
    return (eos.get_entropy(e, rhob));
  }
  double get_eta_by_s(double T, double muB) const override {
    return (transport.get_eta_over_s(T, muB));
  }
  double get_zeta_by_s(double T, double muB) const override {
    return (transport.get_zeta_over_s(T));
  }

private:
  const ::EOS &eos;
  TransportCoeffs transport;
};
Density_Frame::Density_Frame(const ::EOS &eosIn, const InitData &DATAIn)
    : DATA(DATAIn) {
  df_eos = std::make_unique<DFHydroMusicEOS>(eosIn, DATA);
  int nx = DATA.nx;
  int ny = DATA.ny;
  double xmin = -1. * DATA.x_size / 2.;
  double xmax = DATA.x_size / 2.;
  double ymin = -1. * DATA.y_size / 2.;
  double ymax = DATA.y_size / 2.;
  vischydro = std::make_unique<DFHydro::Vischydro>(nx, xmin, xmax, ny, ymin,
                                                   ymax, df_eos.get());
}
Density_Frame::~Density_Frame() {}

// Initialize the vischydro petsc grid at every timestep
void Density_Frame::Run(double tau, SCGrid &arena_current) {

  int local_ieta = 0;

  // Domain is the Petsc grid descriptor
  DM domain = vischydro->domain;
  // Get the coordinate DM, the grid descriptor for the coordinates
  DM cdomain = vischydro->cdomain;

  // Convert the Petsc Vec objects to c-style 2D arrays for easy access
  DFHydro::VischydroNode **asol;
  PetscCallVoid(DMDAVecGetArray(domain, vischydro->solution, &asol));
  DMDACoor2d **xy;
  PetscCallVoid(DMDAVecGetArray(cdomain, vischydro->coordinates, &xy));

  // Loop over the grid and set the initial conditions
  PetscInt xs, ys, xm, ym;
  DMDAGetCorners(domain, &xs, &ys, NULL, &xm, &ym, NULL);

  for (PetscInt iy = ys; iy < ys + ym; iy++) {
    for (PetscInt ix = xs; ix < xs + xm; ix++) {
      double x = xy[iy][ix].x;
      double y = xy[iy][ix].y;
      int local_ix = ((x + DATA.x_size / 2.) / DATA.delta_x + 0.5);
      int local_iy = ((y + DATA.y_size / 2.) / DATA.delta_y + 0.5);

      asol[iy][ix].e = std::max(
          0.001, arena_current(local_ix, local_iy, local_ieta).epsilon);
      asol[iy][ix].u[0] = arena_current(local_ix, local_iy, local_ieta).u[1];
      asol[iy][ix].u[1] = arena_current(local_ix, local_iy, local_ieta).u[2];

      // Fill in the rest of the VischydroNode fields based on the EOS
      vhnode_fill(asol[iy][ix], *vischydro->eos);
    }
  }
  // Restore the pointers to the Petsc Vec objects
  PetscCallVoid(DMDAVecRestoreArray(domain, vischydro->solution, &asol));
  PetscCallVoid(DMDAVecRestoreArray(cdomain, vischydro->coordinates, &xy));

  vischydro->solve(tau, tau + DATA.delta_tau, DATA.delta_tau);

  PetscCallVoid(DMDAVecGetArray(cdomain, vischydro->coordinates, &xy));
  PetscCallVoid(DMDAVecGetArray(domain, vischydro->solution, &asol));
  DMDAGetCorners(domain, &xs, &ys, NULL, &xm, &ym, NULL);
  for (PetscInt iy = ys; iy < ys + ym; iy++) {
    for (PetscInt ix = xs; ix < xs + xm; ix++) {
      double x = xy[iy][ix].x;
      double y = xy[iy][ix].y;
      int local_ix = ((x + DATA.x_size / 2.) / DATA.delta_x + 0.5);
      int local_iy = ((y + DATA.y_size / 2.) / DATA.delta_y + 0.5);

      // This is the same as in simple_initialize, but could also use xy array
      arena_current(local_ix, local_iy, local_ieta).epsilon = asol[iy][ix].e;
      arena_current(local_ix, local_iy, local_ieta).u[1] = asol[iy][ix].u[0];
      arena_current(local_ix, local_iy, local_ieta).u[2] = asol[iy][ix].u[1];
      arena_current(local_ix, local_iy, local_ieta).u[3] = 0.;
      arena_current(local_ix, local_iy, local_ieta).u[0] =
          sqrt(1. + asol[iy][ix].u[0] * asol[iy][ix].u[0] +
               asol[iy][ix].u[1] * asol[iy][ix].u[1]);
    }
  }
  PetscCallVoid(DMDAVecRestoreArray(cdomain, vischydro->coordinates, &xy));
  PetscCallVoid(DMDAVecRestoreArray(domain, vischydro->solution, &asol));
}
