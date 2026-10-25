/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file record_output.cpp
 * @brief standalone tool that copies the output of a command to a log file
 * and, live, to the console.
 *
 * The tool is installed next to bambu and it is used by the generated scripts
 * and by the simulation flow in place of the system tee. tee is not part of
 * the tool flow: when it dies mid-stream, as the tee of uutils coreutils
 * 0.10.0 does by asserting inside its internal splice, the pipe closes and
 * the logged command, the simulator or the system testbench, is killed by
 * SIGPIPE and a run that was actually correct is reported as a failure.
 *
 * This copier never endangers the logged command:
 * - it belongs to the distribution, so the run does not depend on external
 *   tools: neither the system tee nor python3 are needed;
 * - SIGPIPE is ignored: if the console disappears, a broken standard output,
 *   the live console copy is disabled, the log file keeps being written and
 *   the logged command is not killed by a closed pipe;
 * - if the log file cannot be written, the error is reported, the input keeps
 *   being drained so that the logged command is not killed, and the exit
 *   status is not zero, so that the flow reports the problem.
 *
 * The console copy is live: every chunk of data read is written to the log
 * file and flushed, then written to the console and flushed. The logs are
 * therefore visible while the command runs, whenever the flow pipes the
 * output through this tool, which is done when the verbosity is high enough
 * (see PandaSystem and the simulation wrappers).
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string_view>

#include <fcntl.h>
#include <unistd.h>

namespace
{
   void usage(const char* exe_name, FILE* out)
   {
      std::fprintf(out, "Usage: %s [-q|--quiet] <log-file>\n", exe_name);
      std::fprintf(out, "Copy the standard input to <log-file> and, live, to the standard output.\n");
      std::fprintf(out, "  -q, --quiet  write only the log file, without the live console copy\n");
      std::fprintf(out, "  -h, --help   print this help and exit\n");
   }

   bool write_all(int fd, const char* data, size_t size)
   {
      while(size > 0)
      {
         const auto written = ::write(fd, data, size);
         if(written < 0)
         {
            if(errno == EINTR)
            {
               continue;
            }
            return false;
         }
         data += written;
         size -= static_cast<size_t>(written);
      }
      return true;
   }
} // namespace

int main(int argc, char** argv)
{
   bool console_copy = true;
   const char* log_file = nullptr;
   for(int arg = 1; arg < argc; arg++)
   {
      const std::string_view current(argv[arg]);
      if(current == "-q" || current == "--quiet")
      {
         console_copy = false;
      }
      else if(current == "-h" || current == "--help")
      {
         usage(argv[0], stdout);
         return 0;
      }
      else if(log_file)
      {
         usage(argv[0], stderr);
         return 2;
      }
      else
      {
         log_file = argv[arg];
      }
   }
   if(!log_file)
   {
      usage(argv[0], stderr);
      return 2;
   }

   // Keep draining stdin when the console disappears.
   std::signal(SIGPIPE, SIG_IGN);

   // Keep the log separate from any closed standard descriptors.
   int log_fd = ::open(log_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if(log_fd >= 0 && log_fd <= STDERR_FILENO)
   {
      const int separate_fd = ::fcntl(log_fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
      const int saved_errno = errno;
      ::close(log_fd);
      log_fd = separate_fd;
      errno = saved_errno;
   }
   // An open failure must not close the producer's pipe.
   bool log_ok = log_fd >= 0;
   if(!log_ok)
   {
      std::fprintf(stderr, "record_output: cannot open log file \"%s\": %s\n", log_file, std::strerror(errno));
   }
   bool live_copy = console_copy;
   char buffer[65536];
   while(true)
   {
      const auto read_size = ::read(STDIN_FILENO, buffer, sizeof(buffer));
      if(read_size == 0)
      {
         break;
      }
      if(read_size < 0)
      {
         if(errno == EINTR)
         {
            continue;
         }
         std::fprintf(stderr, "record_output: error reading the input: %s\n", std::strerror(errno));
         log_ok = false;
         break;
      }
      const auto size = static_cast<size_t>(read_size);
      if(log_ok && !write_all(log_fd, buffer, size))
      {
         std::fprintf(stderr,
                      "record_output: error writing the log file \"%s\": %s. The log is not updated anymore, "
                      "but the logged command is not interrupted.\n",
                      log_file, std::strerror(errno));
         log_ok = false;
      }
      if(live_copy && !write_all(STDOUT_FILENO, buffer, size))
      {
         std::fprintf(stderr,
                      "record_output: the console copy failed (%s). The live console copy is disabled, "
                      "the log file goes on.\n",
                      std::strerror(errno));
         live_copy = false;
      }
   }

   // close can report a deferred write error of the buffered writes.
   if(log_fd >= 0 && ::close(log_fd) != 0)
   {
      std::fprintf(stderr, "record_output: error closing the log file \"%s\": %s\n", log_file, std::strerror(errno));
      log_ok = false;
   }
   return log_ok ? 0 : 1;
}
