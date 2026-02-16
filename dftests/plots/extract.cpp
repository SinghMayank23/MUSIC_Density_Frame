
#include<string>
#include<iostream>
#include<fstream>
#include<iomanip>
#include<cstdlib>
#include<math.h>
#include<sstream>
#include<cmath>

using namespace std;

int main() {

  int nx = 256;
  int ny = 256;
  int neta = 1;
  double dx = 0.1328124;
  double dy = 0.1328124;
  double deta = 0.3125;
  double tau0 = 0.6;
  double dtau = 0.05;
  double tau_final = 9.3;
  int ieta = 0;
  double hbarc = 0.19733;

  float ideal[5], Wmunu[10], Pi[3];

  string filename = "evolution_xyeta.dat";
  fstream file_ideal;
  file_ideal.open(filename.c_str());

  string file_vis = "evolution_Wmunu_over_epsilon_plus_P_xyeta.dat";
  fstream visfile;
  visfile.open(file_vis.c_str());

  string file_bulk = "evolution_bulk_pressure_xyeta.dat";
  fstream bulkfile;
  bulkfile.open(file_bulk.c_str());

  int itau = 0;
  double tau = tau0;

  while (tau < tau_final)
  {

    FILE *filedd;
    string ddfile = "plotdata" + to_string(itau) + ".dat";
    filedd = fopen(ddfile.c_str(), "w");

    int linenumber = itau*neta*ny*nx + ieta*ny*nx;
    int long pos;
    float temp = 0.;
    pos = linenumber*5*sizeof(float);
    file_ideal.seekg(pos);
    pos = linenumber*10*sizeof(float);
    visfile.seekg(pos);
    pos = linenumber*3*sizeof(float);
    bulkfile.seekg(pos);
    for (int iy = 0; iy < ny; iy++)
    {
      float y = ((float)iy)*dy - ((float)ny)*dy/2.;
      for (int ix = 0; ix < nx; ix++)
      {
        float x = ((float)ix)*dx - ((float)nx)*dx/2.;
        for (int ii = 0; ii < 5; ii++)
        {
          file_ideal.read((char*)&temp, sizeof(float));
          ideal[ii] = temp;
        }
        for (int ii = 0; ii < 10; ii++)
        {
          visfile.read((char*)&temp, sizeof(float));
          Wmunu[ii] = temp;
        }
        for (int ii = 0; ii < 3; ii++)
        {
          bulkfile.read((char*)&temp, sizeof(float));
          Pi[ii] = temp;
        }

        fprintf(filedd, "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n", tau, x, y, ideal[0], ideal[1], ideal[2], ideal[3], Pi[0], Wmunu[0], Wmunu[1], Wmunu[2],
                            Wmunu[4], Wmunu[5], Wmunu[7], Wmunu[9]);
      }
      fprintf(filedd, "\n");
    }

    itau += 1;
    tau = tau0 + ((double)itau)*dtau;
    fclose(filedd);
  } 

  file_ideal.close();
  visfile.close();
  bulkfile.close();

}
