#include "mgm/groupbalancer/BalancerEngine.hh"

namespace eos::mgm::group_balancer {

class MinMaxBalancerEngine: public BalancerEngine
{
public:
  void recalculate() override {};
  void updateGroup(const std::string& group_name) override;
  void configure(const engine_conf_t& conf) override;

  double get_min_threshold() const {
    return mMinThreshold;
  }

  double get_max_threshold() const {
    return mMaxThreshold;
  }

private:
  double mMinThreshold;
  double mMaxThreshold;
};


}
