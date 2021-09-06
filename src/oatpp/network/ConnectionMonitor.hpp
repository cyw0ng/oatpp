/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#ifndef oatpp_network_ConnectionMonitor_hpp
#define oatpp_network_ConnectionMonitor_hpp

#include "./ConnectionProvider.hpp"
#include "oatpp/core/data/stream/Stream.hpp"

#include "unordered_map"

namespace oatpp { namespace network {

class ConnectionMonitor {
public:

  struct ConnectionStats {

    v_int64 timestampCreated = 0;

    v_io_size totalRead = 0;
    v_io_size totalWrite = 0;

    v_int64 timestampLastRead = 0;
    v_int64 timestampLastWrite = 0;

    v_io_size lastReadSize = 0;
    v_io_size lastWriteSize = 0;

    std::unordered_map<oatpp::String, void*> metricData;

  };

public:

  class StatCollector {
  public:
    virtual oatpp::String metricName() = 0;
    virtual void* createMetricData() = 0;
    virtual void deleteMetricData(void* metricData) = 0;
    virtual void onRead(void* metricData, v_io_size readResult, v_int64 timestamp) = 0;
    virtual void onWrite(void* metricData, v_io_size writeResult, v_int64 timestamp) = 0;
  };

private:

  class Monitor; // FWD

  class ConnectionProxy : public data::stream::IOStream {
    friend Monitor;
  private:
    std::shared_ptr<Monitor> m_monitor;
    /* provider which created this connection */
    std::shared_ptr<ConnectionProvider> m_connectionProvider;
    std::shared_ptr<data::stream::IOStream> m_connection;
    std::mutex m_statsMutex;
    ConnectionStats m_stats;
  public:

    ConnectionProxy(const std::shared_ptr<Monitor>& monitor,
                    const std::shared_ptr<ConnectionProvider>& connectionProvider,
                    const std::shared_ptr<data::stream::IOStream>& connection);

    ~ConnectionProxy() override;

    v_io_size read(void *buffer, v_buff_size count, async::Action& action) override;
    v_io_size write(const void *data, v_buff_size count, async::Action& action) override;

    void setInputStreamIOMode(data::stream::IOMode ioMode) override;
    data::stream::IOMode getInputStreamIOMode() override;
    data::stream::Context& getInputStreamContext() override;

    void setOutputStreamIOMode(data::stream::IOMode ioMode) override;
    data::stream::IOMode getOutputStreamIOMode() override;
    data::stream::Context& getOutputStreamContext() override;

    void invalidate();

  };

private:

  class Monitor {
  private:

    std::atomic<bool> m_running {true};

    std::mutex m_connectionsMutex;
    std::unordered_map<v_uint64, std::weak_ptr<ConnectionProxy>> m_connections;

    std::mutex m_statCollectorsMutex;
    std::unordered_map<oatpp::String, std::shared_ptr<StatCollector>> m_statCollectors;

  private:
    static void monitorTask(std::shared_ptr<Monitor> monitor);
  private:
    void* createOrGetMetricData(ConnectionStats& stats, const std::shared_ptr<StatCollector>& collector);
  public:

    static std::shared_ptr<Monitor> createShared();

    void addConnection(v_uint64 id, const std::weak_ptr<ConnectionProxy>& connection);
    void freeConnectionStats(ConnectionStats& stats);
    void removeConnection(v_uint64 id);

    void addStatCollector(const std::shared_ptr<StatCollector>& collector);
    void removeStatCollector(const oatpp::String& metricName);

    void onConnectionRead(ConnectionStats& stats, v_io_size readResult);
    void onConnectionWrite(ConnectionStats& stats, v_io_size writeResult);

    void stop();

  };

private:
  std::shared_ptr<Monitor> m_monitor;
  std::shared_ptr<ConnectionProvider> m_connectionProvider;
protected:

  ConnectionMonitor(const std::shared_ptr<ConnectionProvider>& connectionProvider);

  std::shared_ptr<data::stream::IOStream> get();

  void addStatCollector(const std::shared_ptr<StatCollector>& collector);

  void stop();

};

}}

#endif //oatpp_network_ConnectionMonitor_hpp