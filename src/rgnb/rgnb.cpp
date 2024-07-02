//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#include "rgnb.hpp"

#include "gnbApp/task.hpp"
#include "gnbGtp/task.hpp"
#include "gnbNgap/task.hpp"
#include "gnbRls/task.hpp"
#include "gnbRrc/task.hpp"
#include "gnbSctp/task.hpp"

#include "ueRls/task.hpp"
#include "ueRrc/task.hpp"
#include "ueNas/task.hpp"
#include "ueApp/task.hpp"

#include <lib/app/cli_base.hpp>

namespace nr::rgnb
{

RGNodeB::RGNodeB(RGnbGnbConfig *gnbConfig, RGnbUeConfig *ueConfig, app::IUeController *ueController, app::INodeListener *nodeListener, NtsTask *cliCallbackTask)
{
    auto *base = new TaskBase();

    base->gnbConfig = gnbConfig;
    base->ueConfig = ueConfig;

    base->logBase = new LogBase("logs/" + config->name + ".log");
    base->nodeListener = nodeListener;
    base->cliCallbackTask = cliCallbackTask;

    // gNB Part Tasks
    base->gnbAppTask = new GnbAppTask(base);    // TODO: Can gNB and UE Part share an AppTask?
    base->gnbSctpTask = new SctpTask(base);
    base->gnbNgapTask = new NgapTask(base);
    base->gnbRrcTask = new GnbRrcTask(base);
    base->gnbGtpTask = new GtpTask(base);
    base->gnbRlsTask = new GnbRlsTask(base);

    // UE Part Tasks
    base->ueAppTask = new UeAppTask(base);
    base->ueRrcTask = new UeRrcTask(base);
    base->ueNasTask = new NasTask(base);
    base->ueRlsTask = new UeRlsTask(base);

    base->ueController = ueController; // TODO: what is this used for?

    taskBase = base;
}

RGNodeB::~RGNodeB()
{
    taskBase->gnbAppTask->quit();
    taskBase->gnbSctpTask->quit();
    taskBase->gnbNgapTask->quit();
    taskBase->gnbRrcTask->quit();
    taskBase->gnbGtpTask->quit();
    taskBase->gnbRlsTask->quit();

    taskBase->ueNasTask->quit();
    taskBase->ueRrcTask->quit();
    taskBase->ueRlsTask->quit();
    taskBase->ueAppTask->quit();

    delete taskBase->gnbAppTask;
    delete taskBase->gnbSctpTask;
    delete taskBase->gnbNgapTask;
    delete taskBase->gnbRrcTask;
    delete taskBase->gnbGtpTask;
    delete taskBase->gnbRlsTask;

    delete taskBase->ueNasTask;
    delete taskBase->ueRrcTask;
    delete taskBase->ueRlsTask;
    delete taskBase->ueAppTask;

    delete taskBase->logBase;

    delete taskBase;
}

void RGNodeB::start()
{
    taskBase->gnbAppTask->start();
    taskBase->gnbSctpTask->start();
    taskBase->gnbNgapTask->start();
    taskBase->gnbRrcTask->start();
    taskBase->gnbRlsTask->start();
    taskBase->gnbGtpTask->start();

    taskBase->ueNasTask->start();
    taskBase->ueRrcTask->start();
    taskBase->ueRlsTask->start();
    taskBase->ueAppTask->start();

}

void RGNodeB::pushCommand(std::unique_ptr<app::RGnbCliCommand> cmd, const InetAddress &address) // TODO: deal with commands for different parts of the RGNodeB
{
    taskBase->appTask->push(std::make_unique<NmGnbCliCommand>(std::move(cmd), address));
}

} // namespace nr::rgnb
