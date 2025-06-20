#include "sine_table_gen.hpp"

#include <fstream>
#include <iostream>
#include <vector>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main(int argc, char** argv)
{
  if(argc < 4) {
    std::cout << "Usage: " << argv[0] << " <count> <freq> <ampl> <rate> <out_file>\n";
    return -1;
  }

  size_t count;
  fptype_t frequency, amplitude, rate;
  std::string out_file;

  count = std::atol(argv[1]);
  frequency = std::atof(argv[2]);
  amplitude = std::atof(argv[3]);
  rate = std::atof(argv[4]);
  out_file = std::string(argv[5]);
  std::cout << "Sine table generation\n"
            << "Points   : " << count << "\n"
            << "Frequency: " << frequency << "\n"
            << "Amplitude: " << amplitude << "\n"
            << "Rate     : " << rate << "\n";

  std::vector<fptype_t> points, values;
  points.resize(count);
  values.resize(count);

  std::cout << "Table generation started...\n";

#ifdef __BAMBU_SIM__
  m_param_alloc(0, points.size() * sizeof(fptype_t));
  m_param_alloc(1, values.size() * sizeof(fptype_t));
#endif
  sine_table_gen(points.data(), values.data(), count, frequency, amplitude, rate);

  std::cout << "Table generation completed.\n";

  std::ofstream out(out_file);

  out << "# " << out_file.substr(0, out_file.find('.')) << "\n";
  for(size_t i = 0; i < count; ++i)
  {
    out << points[i] << " " << values[i] << "\n";
  }

  return 0;
}
