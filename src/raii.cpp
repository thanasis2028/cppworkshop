#include <cassert> // assert()
#include <cstddef> // std::size_t
#include <new> // std::bad_alloc
#include <random> // std::random_device etc
#include <set> // std::set
#include <unordered_set> // std::unordered_set
#include <memory>

namespace net {
namespace util {
  template <typename T, T MIN, T MAX>
  static inline T random()
  {
    static std::random_device rd;
    static std::default_random_engine dre{rd()};
    static std::uniform_int_distribution<T> prob_dist{MIN, MAX};
    return prob_dist(dre);
  }
  static inline bool success() { return random<int, 0, 1>() ? true : false; }
} // namespace util

namespace stats {
static std::size_t bound{0}; ///< number of bound client ports
static std::size_t connected{0}; ///< number of connected server ports
static std::size_t closed{0}; ///< number of closed ports
static std::size_t msgs{0}; ///< number of existing Message instances
} // namespace stats

/// network message
struct Message
{
  Message() { ++stats::msgs; }
  ~Message() { --stats::msgs; }
};

/// get ephemeral client port
int client_port() { return util::random<int, 32768, 61000>(); }
/// bind client port
bool bind(int cport) { return util::success() && ++stats::bound; }
/// connect to server port
bool connect(int sport) { return util::success() && ++stats::connected; }
/// send network message
bool send(const Message* req) { return util::success(); }
/// receive network message
bool recv(std::unique_ptr<net::Message> const &resp) { return util::success(); }
/// close port
void close(int port) { ++stats::closed; }
} // namespace net

class Port {
  int number;
  bool active;
  public:
  Port(int number): number(number), active(false) {}
  bool bind() {
    return active = net::bind(number);
  }
  
  bool connect() {
    return active = net::connect(number);
  }
  
  ~Port() {
    if (active)
      net::close(number);
  }
};

/**
 * @brief Scan a number of network server ports.
 *
 * @param server_ports Network server ports to scan.
 *
 * @return Set of active (listening) server ports.
 */
std::unordered_set<int> port_scan(const std::set<int>& server_ports)
{
  std::unordered_set<int> active_ports; ///< active server ports
  for (auto sport : server_ports) {
    const int cport = net::client_port();
    Port my_sport(sport), my_cport(cport);
    if (my_cport.bind() == false) {
      continue;
    }
    if (my_sport.connect() == false) {
      continue;
    }
    net::Message req;
    if (net::send(&req) == false) {
      continue;
    }
    std::unique_ptr<net::Message> resp{nullptr};
    try {
      resp = std::make_unique<net::Message>();
    }
    catch (const std::bad_alloc&) {
    }
    if (net::recv(resp)) {
      // let's try again
      if (net::send(&req) == false || net::recv(resp) == false) {
        continue;
      }
      active_ports.insert(sport);
    }
  }
  return active_ports;
}


int main(int argc, char* argv[])
{
  std::set<int> server_ports{21, 22, 23, 80, 143, 161, 443};
  port_scan(server_ports);
  assert(net::stats::bound + net::stats::connected <= net::stats::closed);
  assert(net::stats::msgs == 0);
  return 0;
}
