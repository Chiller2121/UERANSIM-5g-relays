//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#pragma once

#include <rgnb/ueNas/usim/usim.hpp>
#include <lib/crypt/milenage.hpp>
#include <lib/nas/nas.hpp>
#include <rgnb/ueNas/mm/mm.hpp>
#include <rgnb/ueNas/sm/sm.hpp>
#include <rgnb/nts.hpp>
#include <rgnb/types.hpp>
#include <utils/nts.hpp>

namespace nr::rgnb
{

class NasTask : public NtsTask
{
  private:
    TaskBase *base;
    std::unique_ptr<Logger> logger;

    NasTimers timers;
    NasMm *mm;
    NasSm *sm;
    Usim *usim;

    friend class UeCmdHandler;

  public:
    explicit NasTask(TaskBase *base);
    ~NasTask() override = default;

  protected:
    void onStart() override;
    void onLoop() override;
    void onQuit() override;

  private:
    void performTick();
};

} // namespace nr::rgnb