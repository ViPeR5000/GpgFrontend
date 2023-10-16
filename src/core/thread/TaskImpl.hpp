/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>

#include "Task.h"

namespace GpgFrontend::Thread {

class Task::Impl : public QObject {
  Q_OBJECT

 public:
  Impl(Task *parent, std::string name)
      : parent_(parent), uuid_(generate_uuid()), name_(name) {
    SPDLOG_TRACE("task {} created", GetFullID());
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object, bool sequency)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(name),
        runnable_(std::move(runnable)),
        callback_(std::move([](int, const DataObjectPtr &) {})),
        callback_thread_(QThread::currentThread()),
        data_object_(data_object),
        sequency_(sequency) {
    SPDLOG_TRACE("task {} created with runnable, callback_thread_: {}",
                 GetFullID(), static_cast<void *>(callback_thread_));
    init();
  }

  Impl(Task *parent, TaskRunnable runnable, std::string name,
       DataObjectPtr data_object, TaskCallback callback, bool sequency)
      : parent_(parent),
        uuid_(generate_uuid()),
        name_(name),
        runnable_(std::move(runnable)),
        callback_(std::move(callback)),
        callback_thread_(QThread::currentThread()),
        data_object_(data_object),
        sequency_(sequency) {
    init();
    SPDLOG_TRACE(
        "task {} created with runnable and callback, callback_thread_: {}",
        GetFullID(), static_cast<void *>(callback_thread_));
  }

  ~Impl() { SPDLOG_TRACE("task {} destroyed", GetFullID()); }

  /**
   * @brief
   *
   * @return std::string
   */
  std::string GetFullID() const { return uuid_ + "/" + name_; }

  std::string GetUUID() const { return uuid_; }

  bool GetSequency() const { return sequency_; }

  void Run() {
    if (runnable_) {
      SetRTN(runnable_(data_object_));
    } else {
      SPDLOG_WARN("no runnable in task, do callback operation");
    }
  }

  /**
   * @brief Set the Finish After Run object
   *
   * @param finish_after_run
   */
  void HoldOnLifeCycle(bool hold_on) {
    this->run_callback_after_runnable_finished_ = !hold_on;
  }

  /**
   * @brief
   *
   * @param rtn
   */
  void SetRTN(int rtn) { this->rtn_ = rtn; }

  /**
   * @brief
   *
   */
  void RunnableInterfaceRun() {
    SPDLOG_TRACE("task {} starting", GetFullID());

    // build runnable package for running
    auto runnable_package = [=, id = GetFullID()]() {
      SPDLOG_DEBUG("task {} runnable start runing", id);
      // Run() will set rtn by itself
      Run();
      // raise signal to anounce after runnable returned
      if (run_callback_after_runnable_finished_)
        emit parent_->SignalTaskRunnableEnd(rtn_);
    };

    if (thread() != QThread::currentThread()) {
      SPDLOG_DEBUG("task running thread is not object living thread");
      // if running sequently
      if (sequency_) {
        // running in another thread, blocking until returned
        if (!QMetaObject::invokeMethod(thread(), runnable_package,
                                       Qt::BlockingQueuedConnection)) {
          SPDLOG_ERROR("qt invoke method failed");
        }
      } else {
        // running in another thread, non-blocking
        if (!QMetaObject::invokeMethod(thread(), runnable_package)) {
          SPDLOG_ERROR("qt invoke method failed");
        }
      }
    } else {
      if (!QMetaObject::invokeMethod(this, runnable_package)) {
        SPDLOG_ERROR("qt invoke method failed");
      }
    }
  }

 public slots:

  /**
   * @brief
   *
   */
  void SlotRun() { RunnableInterfaceRun(); }

 private:
  Task *parent_;
  const std::string uuid_;
  const std::string name_;
  const bool sequency_ = true;  ///< must run in the same thread
  TaskCallback callback_;       ///<
  TaskRunnable runnable_;       ///<
  bool run_callback_after_runnable_finished_ = true;  ///<
  int rtn_ = 0;                                       ///<
  QThread *callback_thread_ = nullptr;                ///<
  DataObjectPtr data_object_ = nullptr;               ///<

  void init() {
    // after runnable finished, running callback
    connect(parent_, &Task::SignalTaskRunnableEnd, this,
            &Impl::slot_task_run_callback);
  }

  /**
   * @brief
   *
   * @return std::string
   */
  std::string generate_uuid() {
    return boost::uuids::to_string(boost::uuids::random_generator()());
  }

 private slots:

  /**
   * @brief
   *
   */
  void slot_task_run_callback(int rtn) {
    SPDLOG_TRACE("task runnable {} finished, rtn: {}", GetFullID(), rtn);
    // set return value
    this->SetRTN(rtn);

    try {
      if (callback_) {
        if (callback_thread_ == QThread::currentThread()) {
          SPDLOG_DEBUG("callback thread is the same thread");
          if (!QMetaObject::invokeMethod(callback_thread_,
                                         [callback = callback_, rtn = rtn_,
                                          &data_object = data_object_, this]() {
                                           callback(rtn, data_object);
                                           // do cleaning work
                                           emit parent_->SignalTaskEnd();
                                         })) {
            SPDLOG_ERROR("failed to invoke callback");
          }
          // just finished, let callack thread to raise SignalTaskEnd
          return;
        } else {
          // waiting for callback to finish
          if (!QMetaObject::invokeMethod(
                  callback_thread_,
                  [callback = callback_, rtn = rtn_,
                   data_object = data_object_]() {
                    callback(rtn, data_object);
                  },
                  Qt::BlockingQueuedConnection)) {
            SPDLOG_ERROR("failed to invoke callback");
          }
        }
      }
    } catch (std::exception &e) {
      SPDLOG_ERROR("exception caught: {}", e.what());
    } catch (...) {
      SPDLOG_ERROR("unknown exception caught");
    }

    // raise signal, announcing this task come to an end
    SPDLOG_DEBUG("task {}, starting calling signal SignalTaskEnd", GetFullID());
    emit parent_->SignalTaskEnd();
  }
};
}  // namespace GpgFrontend::Thread