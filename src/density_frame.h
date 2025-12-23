#ifndef SRC_DENSITY_FRAME_H_
#define SRC_DENSITY_FRAME_H_

#include <memory>
#include "data.h"
#include "cell.h"
#include "grid.h"
#include "reconst.h"
#include "pretty_ostream.h"
#include "Vischydro.hpp"
#include "transport_coeffs.h"

using namespace DFHydro;

class Density_Frame {
 private:
    const InitData &DATA;
    const EOS &eos;

    TransportCoeffs transport_coeffs_;
    Vischydro vischydro;

 public:
    Density_Frame(const EOS &eosIn, const InitData &DATA_in);

    void Run(double tau, SCGrid &arena_current);
};

#endif  // SRC_DENSITY_FRAME_H_
