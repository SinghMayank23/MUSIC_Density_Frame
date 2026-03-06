#ifndef SRC_DENSITY_FRAME_H_
#define SRC_DENSITY_FRAME_H_

#include <memory>
#include "data.h"
#include "cell.h"
#include "grid.h"
#include "reconst.h"
#include "pretty_ostream.h"
#include "transport_coeffs.h"
#include <DFHydro/Vischydro.hpp>

namespace DFHydro {
   class Vischydro;
   class EOS;
};

class Density_Frame {
 private:
    const InitData &DATA;
    std::unique_ptr<DFHydro::Vischydro> vischydro;
    std::unique_ptr<DFHydro::EOS> df_eos;
    pretty_ostream music_message;

    DFHydro::VischydroNode **asol = nullptr;
    DMDACoor2d **xy = nullptr;

    DFHydro::VischydroNode** get_asol() const { return asol; }



 public:
    Density_Frame(const ::EOS &eosIn, const InitData &DATA_in);
    ~Density_Frame();

    void Run(double tau, double tau_next, SCGrid &arena_current);

   
};
#endif  // SRC_DENSITY_FRAME_H_
