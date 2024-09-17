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

namespace nr::rgnb
{

void UeRrcTask::declareRadioLinkFailure(rls::ERlfCause cause)
{
    handleRadioLinkFailure(cause);
}

void UeRrcTask::handleRadioLinkFailure(rls::ERlfCause cause)
{
    m_state = ERrcState::RRC_IDLE;
//    m_base->ueNasTask->push(std::make_unique<NmUeRrcToNas>(NmUeRrcToNas::RADIO_LINK_FAILURE)); // TODO: if it is more than a notification, this needs to be handled
}

} // namespace nr::rgnb