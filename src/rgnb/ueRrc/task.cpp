//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#include "task.hpp"
#include <asn/rrc/ASN_RRC_RRCSetupRequest-IEs.h>
#include <asn/rrc/ASN_RRC_RRCSetupRequest.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer-IEs.h>
#include <asn/rrc/ASN_RRC_ULInformationTransfer.h>
#include <lib/rrc/encode.hpp>
#include <rgnb/ueRls/task.hpp>
#include <utils/common.hpp>

static constexpr const int TIMER_ID_MACHINE_CYCLE = 1;
static constexpr const int TIMER_PERIOD_MACHINE_CYCLE = 2500;

namespace nr::rgnb
{

UeRrcTask::UeRrcTask(TaskBase *base) : m_base{base}, m_timers{}
{
    m_logger = base->logBase->makeUniqueLogger(base->ueConfig->getLoggerPrefix() + "ueRrc");

    m_startedTime = utils::CurrentTimeMillis();
    m_state = ERrcState::RRC_IDLE;
    m_establishmentCause = ASN_RRC_EstablishmentCause_mt_Access;
}

void UeRrcTask::onStart()
{
    triggerCycle();

    setTimer(TIMER_ID_MACHINE_CYCLE, TIMER_PERIOD_MACHINE_CYCLE);
}

void UeRrcTask::onQuit()
{
    // TODO
}

void UeRrcTask::onLoop()
{
    auto msg = take();
    if (!msg)
        return;

    switch (msg->msgType)
    {
//    case NtsMessageType::RGNB_NGAP_TO_RRC: {
//        handleNgapSapMessage(dynamic_cast<NmRgnbNgapToRrc &>(*msg));
//        break;
//    }
    case NtsMessageType::RGNB_RRC_TO_RRC: {
        handleRrcSapMessage(dynamic_cast<NmRgnbRrcToRrc &>(*msg));
        break;
    }
    case NtsMessageType::UE_NAS_TO_RRC: {
        handleNasSapMessage(dynamic_cast<NmUeNasToRrc &>(*msg));
        break;
    }
    case NtsMessageType::UE_RLS_TO_RRC: {
        handleRlsSapMessage(dynamic_cast<NmUeRlsToRrc &>(*msg));
        break;
    }
    case NtsMessageType::UE_RRC_TO_RRC: {
        auto &w = dynamic_cast<NmUeRrcToRrc &>(*msg);
        switch (w.present)
        {
        case NmUeRrcToRrc::TRIGGER_CYCLE:
            performCycle();
            break;
        }
        break;
    }
    case NtsMessageType::TIMER_EXPIRED: {
        auto &w = dynamic_cast<NmTimerExpired &>(*msg);
        if (w.timerId == TIMER_ID_MACHINE_CYCLE)
        {
            setTimer(TIMER_ID_MACHINE_CYCLE, TIMER_PERIOD_MACHINE_CYCLE);
            performCycle();
        }
        break;
    }
    default:
        m_logger->unhandledNts(*msg);
        break;
    }
}

} // namespace nr::rgnb