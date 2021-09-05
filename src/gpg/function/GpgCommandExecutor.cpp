/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */
#include "gpg/function/GpgCommandExecutor.h"

#include <boost/asio.hpp>

using boost::process::async_pipe;

void GpgFrontend::GpgCommandExecutor::Execute(
    StringArgsRef arguments,
    const std::function<void(async_pipe &in, async_pipe &out)> &interact_func) {

  using namespace boost::process;

  boost::asio::io_service ios;

  std::vector<char> buf;

  async_pipe in_pipe_stream(ios);
  async_pipe out_pipe_stream(ios);

  child child_process(ctx.GetInfo().appPath.c_str(), arguments,
                      std_out > in_pipe_stream, std_in < out_pipe_stream);

  boost::asio::async_read(
      in_pipe_stream, boost::asio::buffer(buf),
      [&](const boost::system::error_code &ec, std::size_t size) {
        interact_func(in_pipe_stream, out_pipe_stream);
      });

  ios.run();
  child_process.wait();
  int result = child_process.exit_code();
}
