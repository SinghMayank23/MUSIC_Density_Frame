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
      : eos(eosIn), DATA(DATA_in), transport(eosIn, DATA_in) {}

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
    if (DATA.viscosity_flag == 0 || DATA.turn_on_shear == 0) {
        return(0.);
    } else {
        return (transport.get_eta_over_s(T, muB));
    }
  }
  double get_zeta_by_s(double T, double muB) const override {
    if (DATA.viscosity_flag == 0 || DATA.turn_on_bulk == 0) {
        return(0.);
    } else {
        return (transport.get_zeta_over_s(T));
    }
  }

private:
  const ::EOS &eos;
  const InitData &DATA;
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
// arena_future stores the DF frame values
void Density_Frame::Run(double tau, double tau_next, SCGrid &arena_current) {

  int local_ieta = 0;  

  PetscCallVoid(DMDAVecGetArray(vischydro->domain, vischydro->solution, &asol));
  PetscCallVoid(DMDAVecGetArray(vischydro->cdomain, vischydro->coordinates, &xy));

  // Loop over the grid and set the initial conditions
  PetscInt xs, ys, xm, ym;
  DMDAGetCorners(vischydro->domain, &xs, &ys, NULL, &xm, &ym, NULL);

  if (tau == DATA.tau0) {
    for (PetscInt iy = ys; iy < ys + ym; iy++) {
      for (PetscInt ix = xs; ix < xs + xm; ix++) {
      
        double x = xy[iy][ix].x;
        double y = xy[iy][ix].y;
        int local_ix = ((x + DATA.x_size / 2.) / DATA.delta_x + 0.5);
        int local_iy = ((y + DATA.y_size / 2.) / DATA.delta_y + 0.5);

        DFHydro::VischydroNode nLF;
        DFHydro::VischydroNode nDF;

        double eps = arena_current(local_ix, local_iy, local_ieta).epsilon;
        double ux = arena_current(local_ix, local_iy, local_ieta).u[1];
        double uy = arena_current(local_ix, local_iy, local_ieta).u[2];
        double piB = arena_current(local_ix, local_iy, local_ieta).pi_b;

        double utau = sqrt(1. + ux * ux + uy * uy);
        double rhob = 0.;
        
        double pressure, E_DF, Mx_DF, My_DF;

        //if (eps > DATA.DF_pedestal) {
        if (eps > DATA.DF_pedestal) {
          
          pressure = df_eos->get_pressure(eps, rhob);

          E_DF  = (eps + pressure + piB) * utau * utau - pressure - piB + arena_current(local_ix, local_iy, local_ieta).Wmunu[0];
          Mx_DF = (eps + pressure + piB) * utau * ux + arena_current(local_ix, local_iy, local_ieta).Wmunu[1];
          My_DF = (eps + pressure + piB) * utau * uy + arena_current(local_ix, local_iy, local_ieta).Wmunu[2];
        } else {
          pressure = df_eos->get_pressure(DATA.DF_pedestal, rhob);

          E_DF = DATA.DF_pedestal + pressure;
          Mx_DF = 0.;
          My_DF = 0.;
        }

        
        nDF.E    = E_DF;
        nDF.M[0] = Mx_DF;
        nDF.M[1] = My_DF;

        vhnode_findstate(nDF, nDF.E, nDF.M[0], nDF.M[1], *df_eos);
        asol[iy][ix] = nDF;
        vhnode_fill(asol[iy][ix], *vischydro->eos);
      } 
    }
  }

  // Restore the pointers to the Petsc Vec objects
  PetscCallVoid(DMDAVecRestoreArray(vischydro->domain, vischydro->solution, &asol));
  PetscCallVoid(DMDAVecRestoreArray(vischydro->cdomain, vischydro->coordinates, &xy));

  //double time_step = std::max(DATA.delta_tau, 0.1*DATA.delta_x);
  double time_step = DATA.delta_tau;
  vischydro->solve(tau, tau_next, time_step);

  PetscCallVoid(DMDAVecGetArray(vischydro->cdomain, vischydro->coordinates, &xy));
  PetscCallVoid(DMDAVecGetArray(vischydro->domain, vischydro->solution, &asol));
  DMDAGetCorners(vischydro->domain, &xs, &ys, NULL, &xm, &ym, NULL);
  for (PetscInt iy = ys; iy < ys + ym; iy++) {
    for (PetscInt ix = xs; ix < xs + xm; ix++) {
      double x = xy[iy][ix].x;
      double y = xy[iy][ix].y;
      int local_ix = ((x + DATA.x_size / 2.) / DATA.delta_x + 0.5);
      int local_iy = ((y + DATA.y_size / 2.) / DATA.delta_y + 0.5);

      double eps, ux, uy, pixxS, pixyS, piyyS, pinnS, piB;
      DFHydro::VischydroNode nDF;
      nDF = asol[iy][ix];
      DFHydro::VischydroNode nLF;

      bool convert_to_LF = vhnode_DFtoLF(*df_eos, nDF, nLF);
      if (convert_to_LF) {
        nLF.getstate_LF(*df_eos, eps, ux, uy, pixxS, pixyS, piyyS, pinnS, piB);

        double utau = sqrt(1. + ux * ux + uy * uy);

        arena_current(local_ix, local_iy, local_ieta).epsilon = eps;
        arena_current(local_ix, local_iy, local_ieta).u[1] = ux;
        arena_current(local_ix, local_iy, local_ieta).u[2] = uy;
        arena_current(local_ix, local_iy, local_ieta).u[3] = 0.;
        arena_current(local_ix, local_iy, local_ieta).u[0] = utau;

        double pietaetaS =  (2.*(  ux*uy*pixyS - (utau*utau - ux*ux)*pixxS
                       - (utau*utau - uy*uy)*piyyS)
                       /(utau*utau));
        double pitauxS = (1./utau*(  pixxS*ux + pixyS*uy));
        double pitauyS = (1./utau*(  pixyS*ux + piyyS*uy));
        double pitautauS = (1./utau* (pitauxS*ux + pitauyS*uy));

        arena_current(local_ix, local_iy, local_ieta).Wmunu[0] = pitautauS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[1] = pitauxS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[2] = pitauyS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[3] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[4] = pixxS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[5] = pixyS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[6] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[7] = piyyS;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[8] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[9] = pietaetaS;

        arena_current(local_ix, local_iy, local_ieta).pi_b = piB;
      } else {
        if (nDF.e > DATA.DF_pedestal) {
          double local_temp = df_eos->get_temperature(nDF.e, 0.);
          music_message << "DF energy exceeds pedestal at tau=" << tau_next << ", ix=" << local_ix << ", iy=" << local_iy << ", temperature=" << local_temp*hbarc << " GeV";
          music_message.flush("warning");
        }
        arena_current(local_ix, local_iy, local_ieta).epsilon = asol[iy][ix].e;
        arena_current(local_ix, local_iy, local_ieta).u[1] = asol[iy][ix].u[0];
        arena_current(local_ix, local_iy, local_ieta).u[2] = asol[iy][ix].u[1];
        arena_current(local_ix, local_iy, local_ieta).u[3] = 0.;
        arena_current(local_ix, local_iy, local_ieta).u[0] =
            sqrt(1. + asol[iy][ix].u[0] * asol[iy][ix].u[0] +
                 asol[iy][ix].u[1] * asol[iy][ix].u[1]);

        arena_current(local_ix, local_iy, local_ieta).Wmunu[0] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[1] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[2] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[3] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[4] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[5] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[6] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[7] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[8] = 0.;
        arena_current(local_ix, local_iy, local_ieta).Wmunu[9] = 0.;

        arena_current(local_ix, local_iy, local_ieta).pi_b = 0.;
    
      }
    }
  }
  PetscCallVoid(DMDAVecRestoreArray(vischydro->cdomain, vischydro->coordinates, &xy));
  PetscCallVoid(DMDAVecRestoreArray(vischydro->domain, vischydro->solution, &asol));
}
