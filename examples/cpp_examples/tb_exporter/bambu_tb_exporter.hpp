#ifndef __BAMBU_TB_EXPORTER_HPP
#define __BAMBU_TB_EXPORTER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace bambu
{
   class testbench_exporter
   {
      const std::string __xml_filename;
      std::vector<std::vector<std::pair<std::string, std::string>>> __function_tb;

      void __export_fini(const std::string& xml_filename)
      {
         std::ofstream tb_xml(xml_filename, std::ios::out | std::ios::trunc);
         tb_xml << "<?xml version=\"1.0\" ?>" << std::endl;
         tb_xml << "<function>" << std::endl;
         for(const auto& setup : __function_tb)
         {
            tb_xml << "  <testbench ";
            for(const auto& arg_init : setup)
            {
               tb_xml << std::endl << "    " << arg_init.first << ":init_file=\"" << arg_init.second << "\"";
            }
            tb_xml << " />" << std::endl;
         }
         tb_xml << "</function>" << std::endl;
         tb_xml.close();
         std::cout << "Exported XML testbench to '" << xml_filename << "'" << std::endl;
      }

    public:
      testbench_exporter(const std::string& tb_xml_filename) : __xml_filename(tb_xml_filename)
      {
      }

      ~testbench_exporter()
      {
         __export_fini(__xml_filename);
      }

      void export_init()
      {
         __function_tb.push_back(std::vector<std::pair<std::string, std::string>>());
      }

      template <typename T>
      void export_dat(const T& data, const std::string& argname)
      {
         const auto filename = argname + "." + std::to_string(__function_tb.size() - 1) + ".dat";
         std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
         outfile.write(reinterpret_cast<const char*>(&data), sizeof(T));
         outfile.close();
         __function_tb.back().push_back(std::make_pair(argname, filename));
         std::cout << "Exported " << sizeof(T) << " bytes to '" << filename << "'" << std::endl;
      }

#ifdef __AC_CHANNEL_H
      template <typename T>
      void export_channel(ac_channel<T>& channel, const std::string& argname)
      {
         const auto count = channel.size();
         T data[count] = {};
         for(auto i = 0U; i < count; ++i)
         {
            data[i] = channel[i];
         }
         const auto filename = argname + "." + std::to_string(__function_tb.size() - 1) + ".dat";
         std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
         outfile.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
         outfile.close();
         __function_tb.back().push_back(std::make_pair(argname, filename));
         std::cout << "Exported " << count << " channel items to '" << filename << "'" << std::endl;
      }
#endif
   };
} // namespace bambu

#endif // __BAMBU_TB_EXPORTER_HPP