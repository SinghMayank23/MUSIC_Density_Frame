#ifndef SRC_DENSITY_FRAME_H_
#define SRC_DENSITY_FRAME_H_

#include <memory>
#include "data.h"
#include "cell.h"
#include "grid.h"
#include "reconst.h"
#include "pretty_ostream.h"
#include "transport_coeffs.h"

namespace DFHydro {
   class Vischydro;
   class EOS;
};

class Density_Frame {
 private:
    const InitData &DATA;
    std::unique_ptr<DFHydro::Vischydro> vischydro;
    std::unique_ptr<DFHydro::EOS> df_eos;

 public:
    Density_Frame(const ::EOS &eosIn, const InitData &DATA_in);
    ~Density_Frame();

    void Run(double tau, SCGrid &arena_current);
};

#endif  // SRC_DENSITY_FRAME_H_
