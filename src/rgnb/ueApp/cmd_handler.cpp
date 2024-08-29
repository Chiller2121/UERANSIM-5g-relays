//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#include "cmd_handler.hpp"

#include <rgnb/ueApp/task.hpp>
#include <rgnb/ueNas/task.hpp>
#include <rgnb/ueRls/task.hpp>
#include <rgnb/ueRrc/task.hpp>
#include <rgnb/ueTun/task.hpp>
#include <utils/common.hpp>
#include <utils/printer.hpp>

#define PAUSE_CONFIRM_TIMEOUT 3000
#define PAUSE_POLLING 10

// todo add coverage again to cli
static std::string SignalDescription(int dbm)
{
    if (dbm > -90)
        return "Excellent";
    if (dbm > -105)
        return "Good";
    if (dbm > -120)
        return "Fair";
    return "Poor";
}

namespace nr::rgnb
{

void UeCmdHandler::sendResult(const InetAddress &address, const std::string &output)
{
    m_base->cliCallbackTask->push(std::make_unique<app::NwCliSendResponse>(address, output, false));
}

void UeCmdHandler::sendError(const InetAddress &address, const std::string &output)
{
    m_base->cliCallbackTask->push(std::make_unique<app::NwCliSendResponse>(address, output, true));
}

void UeCmdHandler::pauseTasks()
{
    m_base->ueNasTask->requestPause();
    m_base->ueRrcTask->requestPause();
    m_base->ueRlsTask->requestPause();
}

void UeCmdHandler::unpauseTasks()
{
    m_base->ueNasTask->requestUnpause();
    m_base->ueRrcTask->requestUnpause();
    m_base->ueRlsTask->requestUnpause();
}

bool UeCmdHandler::isAllPaused()
{
    if (!m_base->ueNasTask->isPauseConfirmed())
        return false;
    if (!m_base->ueRrcTask->isPauseConfirmed())
        return false;
    if (!m_base->ueRlsTask->isPauseConfirmed())
        return false;
    return true;
}

void UeCmdHandler::handleCmd(NmUeCliCommand &msg)
{
    pauseTasks();

    uint64_t currentTime = utils::CurrentTimeMillis();
    uint64_t endTime = currentTime + PAUSE_CONFIRM_TIMEOUT;

    bool isPaused = false;
    while (currentTime < endTime)
    {
        currentTime = utils::CurrentTimeMillis();
        if (isAllPaused())
        {
            isPaused = true;
            break;
        }
        utils::Sleep(PAUSE_POLLING);
    }

    if (!isPaused)
    {
        sendError(msg.address, "UE is unable process command due to pausing timeout");
    }
    else
    {
        handleCmdImpl(msg);
    }

    unpauseTasks();
}

void UeCmdHandler::handleCmdImpl(NmUeCliCommand &msg)
{
    switch (msg.cmd->present)
    {
    case app::UeCliCommand::STATUS: {
        std::optional<int> currentCellId = std::nullopt;
        std::optional<Plmn> currentPlmn = std::nullopt;
        std::optional<int> currentTac = std::nullopt;

        auto currentCell = m_base->shCtx.currentCell.get();
        if (currentCell.hasValue())
        {
            currentCellId = currentCell.cellId;
            currentPlmn = currentCell.plmn;
            currentTac = currentCell.tac;
        }

        Json json = Json::Obj({
            {"cm-state", ToJson(m_base->ueNasTask->mm->m_cmState)},
            {"rm-state", ToJson(m_base->ueNasTask->mm->m_rmState)},
            {"mm-state", ToJson(m_base->ueNasTask->mm->m_mmSubState)},
            {"5u-state", ToJson(m_base->ueNasTask->mm->m_storage->uState->get())},
            {"sim-inserted", m_base->ueNasTask->mm->m_usim->isValid()},
            {"selected-plmn", ::ToJson(m_base->shCtx.selectedPlmn.get())},
            {"current-cell", ::ToJson(currentCellId)},
            {"current-plmn", ::ToJson(currentPlmn)},
            {"current-tac", ::ToJson(currentTac)},
            {"last-tai", ToJson(m_base->ueNasTask->mm->m_storage->lastVisitedRegisteredTai)},
            {"stored-suci", ToJson(m_base->ueNasTask->mm->m_storage->storedSuci->get())},
            {"stored-guti", ToJson(m_base->ueNasTask->mm->m_storage->storedGuti->get())},
            {"has-emergency", ::ToJson(m_base->ueNasTask->mm->hasEmergency())},
        });
        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::UeCliCommand::INFO: {
        auto json = Json::Obj({
            {"supi", ToJson(m_base->ueConfig->supi)},
            {"hplmn", ToJson(m_base->ueConfig->hplmn)},
            {"imei", ::ToJson(m_base->ueConfig->imei)},
            {"imeisv", ::ToJson(m_base->ueConfig->imeiSv)},
            {"ecall-only", ::ToJson(m_base->ueNasTask->usim->m_isECallOnly)},
            {"uac-aic", Json::Obj({
                            {"mps", m_base->ueConfig->uacAic.mps},
                            {"mcs", m_base->ueConfig->uacAic.mcs},
                        })},
            {"uac-acc", Json::Obj({
                            {"normal-class", m_base->ueConfig->uacAcc.normalCls},
                            {"class-11", m_base->ueConfig->uacAcc.cls11},
                            {"class-12", m_base->ueConfig->uacAcc.cls12},
                            {"class-13", m_base->ueConfig->uacAcc.cls13},
                            {"class-14", m_base->ueConfig->uacAcc.cls14},
                            {"class-15", m_base->ueConfig->uacAcc.cls15},
                        })},
            {"is-high-priority", m_base->ueNasTask->mm->isHighPriority()},
        });

        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::UeCliCommand::TIMERS: {
        sendResult(msg.address, ToJson(m_base->ueNasTask->timers).dumpYaml());
        break;
    }
    case app::UeCliCommand::DE_REGISTER: {
        m_base->ueNasTask->mm->deregistrationRequired(msg.cmd->deregCause);

        if (msg.cmd->deregCause != EDeregCause::SWITCH_OFF)
            sendResult(msg.address, "De-registration procedure triggered");
        else
            sendResult(msg.address, "De-registration procedure triggered. UE device will be switched off.");
        break;
    }
    case app::UeCliCommand::PS_RELEASE: {
        for (int i = 0; i < msg.cmd->psCount; i++)
            m_base->ueNasTask->sm->sendReleaseRequest(static_cast<int>(msg.cmd->psIds[i]) % 16);
        sendResult(msg.address, "PDU session release procedure(s) triggered");
        break;
    }
    case app::UeCliCommand::PS_RELEASE_ALL: {
        m_base->ueNasTask->sm->sendReleaseRequestForAll();
        sendResult(msg.address, "PDU session release procedure(s) triggered");
        break;
    }
    case app::UeCliCommand::PS_ESTABLISH: {
        SessionConfig ueConfig;
        ueConfig.type = nas::EPduSessionType::IPV4;
        ueConfig.isEmergency = msg.cmd->isEmergency;
        ueConfig.apn = msg.cmd->apn;
        ueConfig.sNssai = msg.cmd->sNssai;
        m_base->ueNasTask->sm->sendEstablishmentRequest(ueConfig);
        sendResult(msg.address, "PDU session establishment procedure triggered");
        break;
    }
    case app::UeCliCommand::PS_LIST: {
        Json json = Json::Obj({});
        for (auto *pduSession : m_base->ueNasTask->sm->m_pduSessions)
        {
            if (pduSession->psi == 0 || pduSession->psState == EPsState::INACTIVE)
                continue;

            auto obj = Json::Obj({
                {"state", ToJson(pduSession->psState)},
                {"session-type", ToJson(pduSession->sessionType)},
                {"apn", ::ToJson(pduSession->apn)},
                {"s-nssai", ToJson(pduSession->sNssai)},
                {"emergency", pduSession->isEmergency},
                {"address", ::ToJson(pduSession->pduAddress)},
                {"ambr", ::ToJson(pduSession->sessionAmbr)},
                {"data-pending", pduSession->uplinkPending},
            });

            json.put("PDU Session" + std::to_string(pduSession->psi), obj);
        }
        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::UeCliCommand::RLS_STATE: {
        Json json = Json::Obj({
            {"sti", OctetString::FromOctet8(m_base->ueRlsTask->m_shCtx->sti).toHexString()},
            {"gnb-search-space", ::ToJson(m_base->ueConfig->gnbSearchList)},
        });
        sendResult(msg.address, json.dumpYaml());
        break;
    }
    case app::UeCliCommand::COVERAGE: {
        Json json = Json::Obj({});

        const auto &cells = m_base->ueRrcTask->m_cellDesc;
        for (auto &item : cells)
        {
            auto &cell = item.second;

            auto mib = Json{};
            auto sib1 = Json{};

            if (cell.mib.hasMib)
            {
                mib = Json::Obj({
                    {"barred", cell.mib.isBarred},
                    {"intra-freq-reselection",
                     std::string{cell.mib.isIntraFreqReselectAllowed ? "allowed" : "not-allowed"}},
                });
            }
            if (cell.sib1.hasSib1)
            {
                sib1 = Json::Obj({
                    {"nr-cell-id", utils::IntToHex(cell.sib1.nci)},
                    {"plmn", ToJson(cell.sib1.plmn)},
                    {"tac", cell.sib1.tac},
                    {"operator-reserved", cell.sib1.isReserved},
                });
            }

            auto obj = Json::Obj({{"signal", std::to_string(cell.dbm) + " dBm (" + SignalDescription(cell.dbm) + ")"},
                                  {"mib", mib},
                                  {"sib1", sib1}});

            json.put("[" + std::to_string(item.first) + "]", obj);
        }

        if (cells.empty())
            json = "No cell available";

        sendResult(msg.address, json.dumpYaml());
        break;
    }
    }
}

} // namespace nr::rgnb
