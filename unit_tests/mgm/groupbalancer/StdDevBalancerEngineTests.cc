#include "gtest/gtest.h"
#include "mgm/groupbalancer/BalancerEngine.hh"
#include "mgm/groupbalancer/StdDevBalancerEngine.hh"


using namespace eos::mgm::group_balancer;

TEST(StdDevBalancerEngine, configure)
{
  std::unique_ptr<BalancerEngine> engine = std::make_unique<StdDevBalancerEngine>();
  engine->configure({{"threshold","5"}});
  auto e = static_cast<StdDevBalancerEngine*>(engine.get());
  EXPECT_DOUBLE_EQ(e->get_threshold(), 0.05);
}

TEST(StdDevBalancerEngine, simple)
{
  std::unique_ptr<BalancerEngine> engine = std::make_unique<StdDevBalancerEngine>();
  engine->configure({{"threshold","5"}});
  engine->populateGroupsInfo({{"group1",{75,100}},
                              {"group2",{81,100}},
                              {"group3",{85,100}},
                              {"group4",{89,100}},
                              {"group5",{95,100}}});

  auto d = engine->get_data();
  EXPECT_NEAR(calculateAvg(d.mGroupSizes),0.85,0.0000001);
  EXPECT_EQ(d.mGroupSizes.size(),5);
  EXPECT_EQ(d.mGroupsOverThreshold.size(),1);
  EXPECT_EQ(d.mGroupsUnderThreshold.size(),1);
  auto [over,under] = engine->pickGroupsforTransfer();
  EXPECT_EQ(over,"group5");
  EXPECT_EQ(under,"group1");
}
