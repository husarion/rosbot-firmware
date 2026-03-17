#pragma once

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

class ClientInterface {
public:
  explicit ClientInterface(const char* service_name)
      : service_name_(service_name) {}

  /// Initialize client on the given node.
  virtual rcl_ret_t init(rcl_node_t& node, rcl_allocator_t& allocator) = 0;

  /// Send request. Returns true if rcl_send_request succeeded.
  virtual bool send() = 0;

  /// Cleanup. Called during destroyEntities().
  virtual void fini(rcl_node_t& node) = 0;

  /// Executor needs these to register the client.
  virtual rcl_client_t*          clientHandle()  = 0;
  virtual void*                  responseMsg()   = 0;
  virtual rclc_client_callback_t responseCallback() = 0;

  virtual const char* serviceName() const { return service_name_; }

protected:
  const char* service_name_;
};