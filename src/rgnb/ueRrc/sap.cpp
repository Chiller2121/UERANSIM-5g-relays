//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#include "task.hpp"

#include <lib/rrc/encode.hpp>
#include <rgnb/nts.hpp>
#include <rgnb/ueRls/task.hpp>

namespace nr::rgnb
{

void UeRrcTask::handleRlsSapMessage(NmUeRlsToRrc &msg)
{
    switch (msg.present)
    {
    case NmUeRlsToRrc::SIGNAL_CHANGED: {
        handleCellSignalChange(msg.cellId, msg.dbm);
        break;
    }
    case NmUeRlsToRrc::DOWNLINK_RRC_DELIVERY: {
        handleDownlinkRrc(msg.cellId, msg.channel, msg.pdu);
        break;
    }
    case NmUeRlsToRrc::RADIO_LINK_FAILURE: {
        handleRadioLinkFailure(msg.rlfCause);
        break;
    }
    }
}

void UeRrcTask::handleNasSapMessage(NmUeNasToRrc &msg)
{
    switch (msg.present)
    {
    case NmUeNasToRrc::UPLINK_NAS_DELIVERY: {
        deliverUplinkNas(msg.pduId, std::move(msg.nasPdu));
        break;
    }
    case NmUeNasToRrc::LOCAL_RELEASE_CONNECTION: {
        // TODO: handle treat barred
        (void)msg.treatBarred;

        switchState(ERrcState::RRC_IDLE);
        m_base->ueRlsTask->push(std::make_unique<NmUeRrcToRls>(NmUeRrcToRls::RESET_STI));
//        m_base->ueNasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RRC_CONNECTION_RELEASE)); // TODO: if it is more than a notification, this needs to be handled
        break;
    }
    case NmUeNasToRrc::RRC_NOTIFY: {
        triggerCycle();
        break;
    }
    case NmUeNasToRrc::PERFORM_UAC: {
        if (!msg.uacCtl->isExpiredForProducer())
            performUac(msg.uacCtl);
        break;
    }
    }
}

void UeRrcTask::handleRrcSapMessage(nr::rgnb::NmRgnbRrcToRrc &msg)
{
    switch (msg.present)
    {
    case NmRgnbRrcToRrc::INITIAL_NAS_DELIVERY: {
        ngapUeId = msg.ueId;
        deliverUplinkNas(0, std::move(msg.pdu)); // TODO: Not sure what the pduId is used for?
        break;
    }
    case NmRgnbRrcToRrc::UPLINK_NAS_DELIVERY: {
        deliverUplinkNas(0, std::move(msg.pdu));
        break;
    }
    case NmRgnbRrcToRrc::DOWNLINK_NAS_DELIVERY: {
        // TODO
        break;
    }
    }
}

//void UeRrcTask::handleNgapSapMessage(NmRgnbNgapToRrc &msg)
//{
//    switch (msg.present)
//    {
//    case NmRgnbNgapToRrc::UPLINK_NAS_DELIVERY: {
//        ngapUeId = msg.ueId;
//        deliverUplinkNas(msg.pduId, std::move(msg.nasPdu));
//        break;
//    }
//    }
//}

} // namespace nr::rgnb