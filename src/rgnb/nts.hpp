//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#pragma once

#include "types.hpp"

#include <utility>

#include <lib/app/cli_base.hpp>
#include <lib/app/cli_cmd.hpp>
#include <lib/asn/utils.hpp>
#include <lib/rls/rls_base.hpp>
#include <lib/rrc/rrc.hpp>
#include <lib/sctp/sctp.hpp>

#include <utils/light_sync.hpp>
#include <utils/network.hpp>
#include <utils/nts.hpp>
#include <utils/octet_string.hpp>
#include <utils/unique_buffer.hpp>

extern "C"
{
    struct ASN_NGAP_FiveG_S_TMSI;
    struct ASN_NGAP_TAIListForPaging;
}

namespace nr::rgnb
{

// GNB Part Messages

struct NmGnbRlsToRrc : NtsMessage
{
    enum PR
    {
        SIGNAL_DETECTED,
        UPLINK_RRC,
    } present;

    // SIGNAL_DETECTED
    // UPLINK_RRC
    int ueId{};

    // UPLINK_RRC
    OctetString data;
    rrc::RrcChannel rrcChannel{};

    explicit NmGnbRlsToRrc(PR present) : NtsMessage(NtsMessageType::GNB_RLS_TO_RRC), present(present)
    {
    }
};

struct NmGnbRlsToGtp : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY,
    } present;

    // DATA_PDU_DELIVERY
    int ueId{};
    int psi{};
    OctetString pdu;

    explicit NmGnbRlsToGtp(PR present) : NtsMessage(NtsMessageType::GNB_RLS_TO_GTP), present(present)
    {
    }
};

struct NmGnbGtpToRls : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY,
    } present;

    // DATA_PDU_DELIVERY
    int ueId{};
    int psi{};
    OctetString pdu{};

    explicit NmGnbGtpToRls(PR present) : NtsMessage(NtsMessageType::GNB_GTP_TO_RLS), present(present)
    {
    }
};

struct NmGnbRlsToRls : NtsMessage
{
    enum PR
    {
        SIGNAL_DETECTED,
        SIGNAL_LOST,
        RECEIVE_RLS_MESSAGE,
        DOWNLINK_RRC,
        DOWNLINK_DATA,
        UPLINK_RRC,
        UPLINK_DATA,
        RADIO_LINK_FAILURE,
        TRANSMISSION_FAILURE,
    } present;

    // SIGNAL_DETECTED
    // SIGNAL_LOST
    // DOWNLINK_RRC
    // DOWNLINK_DATA
    // UPLINK_DATA
    // UPLINK_RRC
    int ueId{};

    // RECEIVE_RLS_MESSAGE
    std::unique_ptr<rls::RlsMessage> msg{};

    // DOWNLINK_DATA
    // UPLINK_DATA
    int psi{};  // PDU Session Identity

    // DOWNLINK_DATA
    // DOWNLINK_RRC
    // UPLINK_DATA
    // UPLINK_RRC
    OctetString data;

    // DOWNLINK_RRC
    uint32_t pduId{};

    // DOWNLINK_RRC
    // UPLINK_RRC
    rrc::RrcChannel rrcChannel{};

    // RADIO_LINK_FAILURE
    rls::ERlfCause rlfCause{};

    // TRANSMISSION_FAILURE
    std::vector<rls::PduInfo> pduList;

    explicit NmGnbRlsToRls(PR present) : NtsMessage(NtsMessageType::GNB_RLS_TO_RLS), present(present)
    {
    }
};

struct NmGnbRrcToRls : NtsMessage
{
    enum PR
    {
        RRC_PDU_DELIVERY,
    } present;

    // RRC_PDU_DELIVERY
    int ueId{};
    rrc::RrcChannel channel{};
    OctetString pdu{};

    explicit NmGnbRrcToRls(PR present) : NtsMessage(NtsMessageType::GNB_RRC_TO_RLS), present(present)
    {
    }
};

struct NmGnbNgapToRrc : NtsMessage
{
    enum PR
    {
        RADIO_POWER_ON,
        NAS_DELIVERY,
        AN_RELEASE,
        PAGING,
    } present;

    // NAS_DELIVERY
    // AN_RELEASE
    int ueId{};

    // NAS_DELIVERY
    OctetString pdu{};

    // PAGING
    asn::Unique<ASN_NGAP_FiveG_S_TMSI> uePagingTmsi{};
    asn::Unique<ASN_NGAP_TAIListForPaging> taiListForPaging{};

    explicit NmGnbNgapToRrc(PR present) : NtsMessage(NtsMessageType::GNB_NGAP_TO_RRC), present(present)
    {
    }
};

struct NmGnbRrcToNgap : NtsMessage
{
    enum PR
    {
        INITIAL_NAS_DELIVERY,
        UPLINK_NAS_DELIVERY,
        RADIO_LINK_FAILURE
    } present;

    // INITIAL_NAS_DELIVERY
    // UPLINK_NAS_DELIVERY
    // RADIO_LINK_FAILURE
    int ueId{};

    // INITIAL_NAS_DELIVERY
    // UPLINK_NAS_DELIVERY
    OctetString pdu{};

    // INITIAL_NAS_DELIVERY
    int64_t rrcEstablishmentCause{};
    std::optional<GutiMobileIdentity> sTmsi{};

    explicit NmGnbRrcToNgap(PR present) : NtsMessage(NtsMessageType::GNB_RRC_TO_NGAP), present(present)
    {
    }
};

struct NmGnbNgapToGtp : NtsMessage
{
    enum PR
    {
        UE_CONTEXT_UPDATE,
        UE_CONTEXT_RELEASE,
        SESSION_CREATE,
        SESSION_RELEASE,
    } present;

    // UE_CONTEXT_UPDATE
    std::unique_ptr<GtpUeContextUpdate> update{};

    // SESSION_CREATE
    PduSessionResource *resource{};

    // UE_CONTEXT_RELEASE
    // SESSION_RELEASE
    int ueId{};

    // SESSION_RELEASE
    int psi{};

    explicit NmGnbNgapToGtp(PR present) : NtsMessage(NtsMessageType::GNB_NGAP_TO_GTP), present(present)
    {
    }
};

struct NmGnbSctp : NtsMessage
{
    enum PR
    {
        CONNECTION_REQUEST,
        CONNECTION_CLOSE,
        ASSOCIATION_SETUP,
        ASSOCIATION_SHUTDOWN,
        RECEIVE_MESSAGE,
        SEND_MESSAGE,
        UNHANDLED_NOTIFICATION,
    } present;

    // CONNECTION_REQUEST
    // CONNECTION_CLOSE
    // ASSOCIATION_SETUP
    // ASSOCIATION_SHUTDOWN
    // RECEIVE_MESSAGE
    // SEND_MESSAGE
    // UNHANDLED_NOTIFICATION
    int clientId{};

    // CONNECTION_REQUEST
    std::string localAddress{};
    uint16_t localPort{};
    std::string remoteAddress{};
    uint16_t remotePort{};
    sctp::PayloadProtocolId ppid{};
    NtsTask *associatedTask{};

    // ASSOCIATION_SETUP
    int associationId{};
    int inStreams{};
    int outStreams{};

    // RECEIVE_MESSAGE
    // SEND_MESSAGE
    UniqueBuffer buffer{};
    uint16_t stream{};

    explicit NmGnbSctp(PR present) : NtsMessage(NtsMessageType::GNB_SCTP), present(present)
    {
    }
};

struct NmGnbStatusUpdate : NtsMessage
{
    static constexpr const int NGAP_IS_UP = 1;

    const int what;

    // NGAP_IS_UP
    bool isNgapUp{};

    explicit NmGnbStatusUpdate(const int what) : NtsMessage(NtsMessageType::GNB_STATUS_UPDATE), what(what)
    {
    }
};

struct NmGnbCliCommand : NtsMessage
{
    std::unique_ptr<app::GnbCliCommand> cmd;
    InetAddress address;

    NmGnbCliCommand(std::unique_ptr<app::GnbCliCommand> cmd, InetAddress address)
        : NtsMessage(NtsMessageType::GNB_CLI_COMMAND), cmd(std::move(cmd)), address(address)
    {
    }
};

// UE Part Messages
// TODO: so far only copied over, are modifications needed?

struct NmAppToTun : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY
    } present;

    // DATA_PDU_DELIVERY
    int psi{};
    OctetString data{};

    explicit NmAppToTun(PR present) : NtsMessage(NtsMessageType::UE_APP_TO_TUN), present(present)
    {
    }
};

struct NmUeTunToApp : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY,
        TUN_ERROR
    } present;

    // DATA_PDU_DELIVERY
    int psi{};
    OctetString data{};

    // TUN_ERROR
    std::string error{};

    explicit NmUeTunToApp(PR present) : NtsMessage(NtsMessageType::UE_TUN_TO_APP), present(present)
    {
    }
};

struct NmUeRrcToNas : NtsMessage
{
    enum PR
    {
        NAS_NOTIFY,
        NAS_DELIVERY,
        RRC_CONNECTION_SETUP,
        RRC_CONNECTION_RELEASE,
        RRC_ESTABLISHMENT_FAILURE,
        RADIO_LINK_FAILURE,
        PAGING,
        ACTIVE_CELL_CHANGED,
        RRC_FALLBACK_INDICATION,
    } present;

    // NAS_DELIVERY
    OctetString nasPdu;

    // PAGING
    std::vector<GutiMobileIdentity> pagingTmsi;

    // ACTIVE_CELL_CHANGED
    Tai previousTai;

    explicit NmUeRrcToNas(PR present) : NtsMessage(NtsMessageType::UE_RRC_TO_NAS), present(present)
    {
    }
};

struct NmUeNasToRrc : NtsMessage
{
    enum PR
    {
        LOCAL_RELEASE_CONNECTION,
        UPLINK_NAS_DELIVERY,
        RRC_NOTIFY,
        PERFORM_UAC,
    } present;

    // UPLINK_NAS_DELIVERY
    uint32_t pduId{};
    OctetString nasPdu;

    // LOCAL_RELEASE_CONNECTION
    bool treatBarred{};

    // PERFORM_UAC
    std::shared_ptr<LightSync<UacInput, UacOutput>> uacCtl{};

    explicit NmUeNasToRrc(PR present) : NtsMessage(NtsMessageType::UE_NAS_TO_RRC), present(present)
    {
    }
};

struct NmUeRrcToRls : NtsMessage
{
    enum PR
    {
        ASSIGN_CURRENT_CELL,
        RRC_PDU_DELIVERY,
        RESET_STI,
    } present;

    // ASSIGN_CURRENT_CELL
    int cellId{};

    // RRC_PDU_DELIVERY
    rrc::RrcChannel channel{};
    uint32_t pduId{};
    OctetString pdu{};

    explicit NmUeRrcToRls(PR present) : NtsMessage(NtsMessageType::UE_RRC_TO_RLS), present(present)
    {
    }
};

struct NmUeRrcToRrc : NtsMessage
{
    enum PR
    {
        TRIGGER_CYCLE,
    } present;

    explicit NmUeRrcToRrc(PR present) : NtsMessage(NtsMessageType::UE_RRC_TO_RRC), present(present)
    {
    }
};

struct NmUeRlsToRrc : NtsMessage
{
    enum PR
    {
        DOWNLINK_RRC_DELIVERY,
        SIGNAL_CHANGED,
        RADIO_LINK_FAILURE
    } present;

    // DOWNLINK_RRC_DELIVERY
    // SIGNAL_CHANGED
    int cellId{};

    // DOWNLINK_RRC_DELIVERY
    rrc::RrcChannel channel{};
    OctetString pdu;

    // SIGNAL_CHANGED
    int dbm{};

    // RADIO_LINK_FAILURE
    rls::ERlfCause rlfCause{};

    explicit NmUeRlsToRrc(PR present) : NtsMessage(NtsMessageType::UE_RLS_TO_RRC), present(present)
    {
    }
};

struct NmUeNasToNas : NtsMessage
{
    enum PR
    {
        PERFORM_MM_CYCLE,
        NAS_TIMER_EXPIRE,
    } present;

    // NAS_TIMER_EXPIRE
    UeTimer *timer{};

    explicit NmUeNasToNas(PR present) : NtsMessage(NtsMessageType::UE_NAS_TO_NAS), present(present)
    {
    }
};

struct NmUeNasToApp : NtsMessage
{
    enum PR
    {
        PERFORM_SWITCH_OFF,
        DOWNLINK_DATA_DELIVERY
    } present;

    // DOWNLINK_DATA_DELIVERY
    int psi{};
    OctetString data;

    explicit NmUeNasToApp(PR present) : NtsMessage(NtsMessageType::UE_NAS_TO_APP), present(present)
    {
    }
};

struct NmUeAppToNas : NtsMessage
{
    enum PR
    {
        UPLINK_DATA_DELIVERY,
    } present;

    // UPLINK_DATA_DELIVERY
    int psi{};
    OctetString data;

    explicit NmUeAppToNas(PR present) : NtsMessage(NtsMessageType::UE_APP_TO_NAS), present(present)
    {
    }
};

struct NmUeNasToRls : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY
    } present;

    // DATA_PDU_DELIVERY
    int psi{};
    OctetString pdu;

    explicit NmUeNasToRls(PR present) : NtsMessage(NtsMessageType::UE_NAS_TO_RLS), present(present)
    {
    }
};

struct NmUeRlsToNas : NtsMessage
{
    enum PR
    {
        DATA_PDU_DELIVERY
    } present;

    // DATA_PDU_DELIVERY
    int psi{};
    OctetString pdu{};

    explicit NmUeRlsToNas(PR present) : NtsMessage(NtsMessageType::UE_RLS_TO_NAS), present(present)
    {
    }
};

struct NmUeRlsToRls : NtsMessage
{
    enum PR
    {
        RECEIVE_RLS_MESSAGE,
        SIGNAL_CHANGED,
        UPLINK_DATA,
        UPLINK_RRC,
        DOWNLINK_DATA,
        DOWNLINK_RRC,
        RADIO_LINK_FAILURE,
        TRANSMISSION_FAILURE,
        ASSIGN_CURRENT_CELL,
    } present;

    // RECEIVE_RLS_MESSAGE
    // UPLINK_RRC
    // DOWNLINK_RRC
    // SIGNAL_CHANGED
    // ASSIGN_CURRENT_CELL
    int cellId{};

    // RECEIVE_RLS_MESSAGE
    std::unique_ptr<rls::RlsMessage> msg{};

    // SIGNAL_CHANGED
    int dbm{};

    // UPLINK_DATA
    // DOWNLINK_DATA
    int psi{};

    // UPLINK_DATA
    // DOWNLINK_DATA
    // UPLINK_RRC
    // DOWNLINK_RRC
    OctetString data;

    // UPLINK_RRC
    // DOWNLINK_RRC
    rrc::RrcChannel rrcChannel{};

    // UPLINK_RRC
    uint32_t pduId{};

    // RADIO_LINK_FAILURE
    rls::ERlfCause rlfCause{};

    // TRANSMISSION_FAILURE
    std::vector<rls::PduInfo> pduList;

    explicit NmUeRlsToRls(PR present) : NtsMessage(NtsMessageType::UE_RLS_TO_RLS), present(present)
    {
    }
};

struct NmUeStatusUpdate : NtsMessage
{
    static constexpr const int SESSION_ESTABLISHMENT = 1;
    static constexpr const int SESSION_RELEASE = 2;
    static constexpr const int CM_STATE = 3;

    const int what{};

    // SESSION_ESTABLISHMENT
    PduSession *pduSession{};

    // SESSION_RELEASE
    int psi{};

    // CM_STATE
    ECmState cmState{};

    explicit NmUeStatusUpdate(const int what) : NtsMessage(NtsMessageType::UE_STATUS_UPDATE), what(what)
    {
    }
};

struct NmUeCliCommand : NtsMessage
{
    std::unique_ptr<app::UeCliCommand> cmd;
    InetAddress address;

    NmUeCliCommand(std::unique_ptr<app::UeCliCommand> cmd, InetAddress address)
        : NtsMessage(NtsMessageType::UE_CLI_COMMAND), cmd(std::move(cmd)), address(address)
    {
    }
};


} // namespace nr::rgnb
