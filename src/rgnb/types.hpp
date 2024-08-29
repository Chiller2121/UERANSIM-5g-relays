//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#pragma once

#include "timer.hpp"

#include <array>
#include <atomic>
#include <deque>
#include <memory>
#include <queue>
#include <set>
#include <unordered_set>

#include <lib/app/monitor.hpp>
#include <lib/asn/utils.hpp>
#include <utils/common_types.hpp>
#include <utils/logger.hpp>
#include <utils/network.hpp>
#include <utils/nts.hpp>
#include <utils/octet_string.hpp>

#include <lib/app/ue_ctl.hpp>
#include <lib/nas/nas.hpp>
#include <utils/json.hpp>
#include <utils/locked.hpp>

#include <asn/ngap/ASN_NGAP_QosFlowSetupRequestList.h>
#include <asn/rrc/ASN_RRC_InitialUE-Identity.h>

// TODO: need to update with UE types

namespace nr::rgnb
{

// GNB classes
class GnbAppTask;
class GtpTask;
class NgapTask;
class GnbRrcTask;
class GnbRlsTask;
class SctpTask;

// UE classes
class UeAppTask;
class NasTask;
class UeRrcTask;
class UeRlsTask;
class RGNodeB;

// GNB types
enum class EAmfState
{
    NOT_CONNECTED = 0,
    WAITING_NG_SETUP,
    CONNECTED
};

struct SctpAssociation
{
    int associationId{};
    int inStreams{};
    int outStreams{};
};

struct Guami
{
    Plmn plmn{};
    int amfRegionId{}; // 8-bit
    int amfSetId{};    // 10-bit
    int amfPointer{};  // 6-bit
};

struct ServedGuami
{
    Guami guami{};
    std::string backupAmfName{};
};

// TODO: update cli and json for overload related types

enum class EOverloadAction
{
    UNSPECIFIED_OVERLOAD,
    REJECT_NON_EMERGENCY_MO_DATA,
    REJECT_SIGNALLING,
    ONLY_EMERGENCY_AND_MT,
    ONLY_HIGH_PRI_AND_MT,
};

enum class EOverloadStatus
{
    NOT_OVERLOADED,
    OVERLOADED
};

struct OverloadInfo
{
    struct Indication
    {
        // Reduce the signalling traffic by the indicated percentage
        int loadReductionPerc{};

        // If reduction percentage is not present, this action shall be used
        EOverloadAction action{};
    };

    EOverloadStatus status{};
    Indication indication{};
};

struct NgapAmfContext
{
    int ctxId{};
    SctpAssociation association{};
    int nextStream{}; // next available SCTP stream for uplink
    std::string address{};
    uint16_t port{};
    std::string amfName{};
    int64_t relativeCapacity{};
    EAmfState state{};
    OverloadInfo overloadInfo{};
    std::vector<ServedGuami *> servedGuamiList{};
    std::vector<PlmnSupport *> plmnSupportList{};
};

struct RlsUeContext
{
    const int ueId;
    uint64_t sti{};
    InetAddress addr{};
    int64_t lastSeen{};

    explicit RlsUeContext(int ueId) : ueId(ueId)
    {
    }
};

struct AggregateMaximumBitRate
{
    uint64_t dlAmbr{};
    uint64_t ulAmbr{};
};

struct NgapUeContext
{
    const int ctxId{};

    int64_t amfUeNgapId = -1; // -1 if not assigned
    int64_t ranUeNgapId{};
    int associatedAmfId{};
    int uplinkStream{};
    int downlinkStream{};
    AggregateMaximumBitRate ueAmbr{};
    std::set<int> pduSessions{};

    explicit NgapUeContext(int ctxId) : ctxId(ctxId)
    {
    }
};

struct RrcUeContext
{
    const int ueId{};

    int64_t initialId = -1; // 39-bit value, or -1
    bool isInitialIdSTmsi{}; // TMSI-part-1 or a random value
    int64_t establishmentCause{};
    std::optional<GutiMobileIdentity> sTmsi{};

    explicit RrcUeContext(const int ueId) : ueId(ueId)
    {
    }
};

struct NgapIdPair
{
    std::optional<int64_t> amfUeNgapId{};
    std::optional<int64_t> ranUeNgapId{};

    NgapIdPair() : amfUeNgapId{}, ranUeNgapId{}
    {
    }

    NgapIdPair(const std::optional<int64_t> &amfUeNgapId, const std::optional<int64_t> &ranUeNgapId)
        : amfUeNgapId(amfUeNgapId), ranUeNgapId(ranUeNgapId)
    {
    }
};

enum class NgapCause
{
    RadioNetwork_unspecified = 0,
    RadioNetwork_txnrelocoverall_expiry,
    RadioNetwork_successful_handover,
    RadioNetwork_release_due_to_ngran_generated_reason,
    RadioNetwork_release_due_to_5gc_generated_reason,
    RadioNetwork_handover_cancelled,
    RadioNetwork_partial_handover,
    RadioNetwork_ho_failure_in_target_5GC_ngran_node_or_target_system,
    RadioNetwork_ho_target_not_allowed,
    RadioNetwork_tngrelocoverall_expiry,
    RadioNetwork_tngrelocprep_expiry,
    RadioNetwork_cell_not_available,
    RadioNetwork_unknown_targetID,
    RadioNetwork_no_radio_resources_available_in_target_cell,
    RadioNetwork_unknown_local_UE_NGAP_ID,
    RadioNetwork_inconsistent_remote_UE_NGAP_ID,
    RadioNetwork_handover_desirable_for_radio_reason,
    RadioNetwork_time_critical_handover,
    RadioNetwork_resource_optimisation_handover,
    RadioNetwork_reduce_load_in_serving_cell,
    RadioNetwork_user_inactivity,
    RadioNetwork_radio_connection_with_ue_lost,
    RadioNetwork_radio_resources_not_available,
    RadioNetwork_invalid_qos_combination,
    RadioNetwork_failure_in_radio_interface_procedure,
    RadioNetwork_interaction_with_other_procedure,
    RadioNetwork_unknown_PDU_session_ID,
    RadioNetwork_unkown_qos_flow_ID,
    RadioNetwork_multiple_PDU_session_ID_instances,
    RadioNetwork_multiple_qos_flow_ID_instances,
    RadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported,
    RadioNetwork_ng_intra_system_handover_triggered,
    RadioNetwork_ng_inter_system_handover_triggered,
    RadioNetwork_xn_handover_triggered,
    RadioNetwork_not_supported_5QI_value,
    RadioNetwork_ue_context_transfer,
    RadioNetwork_ims_voice_eps_fallback_or_rat_fallback_triggered,
    RadioNetwork_up_integrity_protection_not_possible,
    RadioNetwork_up_confidentiality_protection_not_possible,
    RadioNetwork_slice_not_supported,
    RadioNetwork_ue_in_rrc_inactive_state_not_reachable,
    RadioNetwork_redirection,
    RadioNetwork_resources_not_available_for_the_slice,
    RadioNetwork_ue_max_integrity_protected_data_rate_reason,
    RadioNetwork_release_due_to_cn_detected_mobility,
    RadioNetwork_n26_interface_not_available,
    RadioNetwork_release_due_to_pre_emption,
    RadioNetwork_multiple_location_reporting_reference_ID_instances,

    Transport_transport_resource_unavailable = 100,
    Transport_unspecified,

    Nas_normal_release = 200,
    Nas_authentication_failure,
    Nas_deregister,
    Nas_unspecified,

    Protocol_transfer_syntax_error = 300,
    Protocol_abstract_syntax_error_reject,
    Protocol_abstract_syntax_error_ignore_and_notify,
    Protocol_message_not_compatible_with_receiver_state,
    Protocol_semantic_error,
    Protocol_abstract_syntax_error_falsely_constructed_message,
    Protocol_unspecified,

    Misc_control_processing_overload = 400,
    Misc_not_enough_user_plane_processing_resources,
    Misc_hardware_failure,
    Misc_om_intervention,
    Misc_unknown_PLMN,
};

struct GtpTunnel
{
    uint32_t teid{};
    OctetString address{};
};

struct PduSessionResource
{
    const int ueId;
    const int psi;

    AggregateMaximumBitRate sessionAmbr{};
    bool dataForwardingNotPossible{};
    PduSessionType sessionType = PduSessionType::UNSTRUCTURED;
    GtpTunnel upTunnel{};
    GtpTunnel downTunnel{};
    asn::Unique<ASN_NGAP_QosFlowSetupRequestList> qosFlows{};

    PduSessionResource(const int ueId, const int psi) : ueId(ueId), psi(psi)
    {
    }
};

struct RGnbStatusInfo
{
    bool isNgapUp{};
};

struct GtpUeContext
{
    const int ueId;
    AggregateMaximumBitRate ueAmbr{};

    explicit GtpUeContext(const int ueId) : ueId(ueId)
    {
    }
};

struct GtpUeContextUpdate
{
    bool isCreate{};
    int ueId{};
    AggregateMaximumBitRate ueAmbr{};

    GtpUeContextUpdate(bool isCreate, int ueId, const AggregateMaximumBitRate &ueAmbr)
        : isCreate(isCreate), ueId(ueId), ueAmbr(ueAmbr)
    {
    }
};

struct GnbAmfConfig
{
    std::string address{};
    uint16_t port{};
};

struct RGnbGnbConfig
{
    /* Read from config file */
    int64_t nci{};     // 36-bit
    int gnbIdLength{}; // 22..32 bit
    Plmn plmn{};
    int tac{};
    NetworkSlice nssai{};
    std::vector<GnbAmfConfig> amfConfigs{};
    std::string linkIp{};
    std::string ngapIp{};
    std::string gtpIp{};
    std::optional<std::string> gtpAdvertiseIp{};
    bool ignoreStreamIds{};

    /* Assigned by program */
    std::string name{};
    EPagingDrx pagingDrx{};
    Vector3 phyLocation{};

    [[nodiscard]] inline uint32_t getGnbId() const
    {
        return static_cast<uint32_t>((nci & 0xFFFFFFFFFLL) >> (36LL - static_cast<int64_t>(gnbIdLength)));
    }

    [[nodiscard]] inline int getCellId() const
    {
        return static_cast<int>(nci & static_cast<uint64_t>((1 << (36 - gnbIdLength)) - 1));
    }
}; // modified

// UE Types
struct UeCellDesc
{
    int dbm{};

    struct
    {
        bool hasMib = false;
        bool isBarred = true;
        bool isIntraFreqReselectAllowed = true;
    } mib{};

    struct
    {
        bool hasSib1 = false;
        bool isReserved = false;
        int64_t nci = 0;
        int tac = 0;
        Plmn plmn;
        UacAiBarringSet aiBarringSet;
    } sib1{};
};

struct SupportedAlgs
{
    bool nia1 = true;
    bool nia2 = true;
    bool nia3 = true;
    bool nea1 = true;
    bool nea2 = true;
    bool nea3 = true;
};

enum class OpType
{
    OP,
    OPC
};

struct SessionConfig
{
    nas::EPduSessionType type{};
    std::optional<SingleSlice> sNssai{};
    std::optional<std::string> apn{};
    bool isEmergency{};
};

struct IntegrityMaxDataRateConfig
{
    bool uplinkFull{};
    bool downlinkFull{};
};

struct CellSelectionReport
{
    int outOfPlmnCells{};
    int siMissingCells{};
    int reservedCells{};
    int barredCells{};
    int forbiddenTaiCells{};
};

struct ActiveCellInfo
{
    int cellId{};
    ECellCategory category{};
    Plmn plmn{};
    int tac{};

    [[nodiscard]] bool hasValue() const;
};

struct UeSharedContext
{
    Locked<std::unordered_set<Plmn>> availablePlmns;
    Locked<Plmn> selectedPlmn;
    Locked<ActiveCellInfo> currentCell;
    Locked<std::vector<Tai>> forbiddenTaiRoaming;
    Locked<std::vector<Tai>> forbiddenTaiRps;
    Locked<std::optional<GutiMobileIdentity>> providedGuti;
    Locked<std::optional<GutiMobileIdentity>> providedTmsi;

    Plmn getCurrentPlmn();
    Tai getCurrentTai();
    bool hasActiveCell();
};

struct RlsSharedContext
{
    std::atomic<uint64_t> sti{};
};

struct RrcTimers
{
    UeTimer t300;

    RrcTimers();
};

struct NasTimers
{
    UeTimer t3346; /* MM - ... */
    UeTimer t3396; /* SM - ... */

    UeTimer t3444; /* MM - ... */
    UeTimer t3445; /* MM - ... */

    UeTimer t3502; /* MM - Initiation of the registration procedure, if still required */
    UeTimer t3510; /* MM - Registration Request transmission timer */
    UeTimer t3511; /* MM - Retransmission of the REGISTRATION REQUEST, if still required */
    UeTimer t3512; /* MM - Periodic registration update timer */
    UeTimer t3516; /* MM - 5G AKA - RAND and RES* storing timer */
    UeTimer t3517; /* MM - Service Request transmission timer */
    UeTimer t3519; /* MM - Transmission with fresh SUCI timer */
    UeTimer t3520; /* MM - ... */
    UeTimer t3521; /* MM - De-registration transmission timer for not switch off */
    UeTimer t3525; /* MM - ... */
    UeTimer t3540; /* MM - ... */

    UeTimer t3584; /* SM - ... */
    UeTimer t3585; /* SM - ... */

    NasTimers();
};

enum class ERmState
{
    RM_DEREGISTERED,
    RM_REGISTERED
};

enum class ECmState
{
    CM_IDLE, // Exact same thing with 5GMM-IDLE in 24.501
    CM_CONNECTED
};

enum class E5UState
{
    U1_UPDATED = 0,
    U2_NOT_UPDATED,
    U3_ROAMING_NOT_ALLOWED
};

enum class ERrcState
{
    RRC_IDLE,
    RRC_CONNECTED,
    RRC_INACTIVE,
};

enum class ERrcLastSetupRequest
{
    SETUP_REQUEST,
    REESTABLISHMENT_REQUEST,
    RESUME_REQUEST,
    RESUME_REQUEST1,
};

enum class EMmState
{
    MM_NULL,
    MM_DEREGISTERED,
    MM_REGISTERED_INITIATED,
    MM_REGISTERED,
    MM_DEREGISTERED_INITIATED,
    MM_SERVICE_REQUEST_INITIATED,
};

enum class EMmSubState
{
    MM_NULL_PS,

    MM_DEREGISTERED_PS,
    MM_DEREGISTERED_NORMAL_SERVICE,
    MM_DEREGISTERED_LIMITED_SERVICE,
    MM_DEREGISTERED_ATTEMPTING_REGISTRATION,
    MM_DEREGISTERED_PLMN_SEARCH,
    MM_DEREGISTERED_NO_SUPI,
    MM_DEREGISTERED_NO_CELL_AVAILABLE,
    MM_DEREGISTERED_ECALL_INACTIVE,
    MM_DEREGISTERED_INITIAL_REGISTRATION_NEEDED,

    MM_REGISTERED_INITIATED_PS,

    MM_REGISTERED_PS,
    MM_REGISTERED_NORMAL_SERVICE,
    MM_REGISTERED_NON_ALLOWED_SERVICE,
    MM_REGISTERED_ATTEMPTING_REGISTRATION_UPDATE,
    MM_REGISTERED_LIMITED_SERVICE,
    MM_REGISTERED_PLMN_SEARCH,
    MM_REGISTERED_NO_CELL_AVAILABLE,
    MM_REGISTERED_UPDATE_NEEDED,

    MM_DEREGISTERED_INITIATED_PS,

    MM_SERVICE_REQUEST_INITIATED_PS
};

enum class EPsState
{
    INACTIVE,
    ACTIVE_PENDING,
    ACTIVE,
    INACTIVE_PENDING,
    MODIFICATION_PENDING
};

enum class EPtState
{
    INACTIVE,
    PENDING,
};

struct PduSession
{
    static constexpr const int MIN_ID = 1;
    static constexpr const int MAX_ID = 15;

    const int psi;

    EPsState psState{};
    bool uplinkPending{};

    nas::EPduSessionType sessionType{};
    std::optional<std::string> apn{};
    std::optional<SingleSlice> sNssai{};
    bool isEmergency{};

    std::optional<nas::IEQoSRules> authorizedQoSRules{};
    std::optional<nas::IESessionAmbr> sessionAmbr{};
    std::optional<nas::IEQoSFlowDescriptions> authorizedQoSFlowDescriptions{};
    std::optional<nas::IEPduAddress> pduAddress{};

    explicit PduSession(int psi) : psi(psi)
    {
    }
};

struct ProcedureTransaction
{
    static constexpr const int MIN_ID = 1;
    static constexpr const int MAX_ID = 254;

    EPtState state{};
    std::unique_ptr<UeTimer> timer{};
    std::unique_ptr<nas::SmMessage> message{};
    int psi{};
};

enum class EConnectionIdentifier
{
    THREE_3GPP_ACCESS = 0x01,
    NON_THREE_3GPP_ACCESS = 0x02,
};

struct NasCount
{
    octet2 overflow{};
    octet sqn{};

    [[nodiscard]] inline octet4 toOctet4() const
    {
        uint32_t value = 0;
        value |= (uint32_t)overflow;
        value <<= 8;
        value |= (uint32_t)sqn;
        return octet4{value};
    }
};

struct UeKeys
{
    OctetString abba{};

    OctetString kAusf{};
    OctetString kSeaf{};
    OctetString kAmf{};
    OctetString kNasInt{};
    OctetString kNasEnc{};

    [[nodiscard]] UeKeys deepCopy() const
    {
        UeKeys keys;
        keys.kAusf = kAusf.subCopy(0);
        keys.kSeaf = kSeaf.subCopy(0);
        keys.kAmf = kAmf.subCopy(0);
        keys.kNasInt = kNasInt.subCopy(0);
        keys.kNasEnc = kNasEnc.subCopy(0);
        return keys;
    }
};

struct NasSecurityContext
{
    nas::ETypeOfSecurityContext tsc{};
    int ngKsi{}; // 3-bit

    NasCount downlinkCount{};
    NasCount uplinkCount{};

    bool is3gppAccess = true;

    UeKeys keys{};
    nas::ETypeOfIntegrityProtectionAlgorithm integrity{};
    nas::ETypeOfCipheringAlgorithm ciphering{};

    std::deque<int> lastNasSequenceNums{};

    void updateDownlinkCount(const NasCount &validatedCount)
    {
        downlinkCount.overflow = validatedCount.overflow;
        downlinkCount.sqn = validatedCount.sqn;
    }

    [[nodiscard]] NasCount estimatedDownlinkCount(octet sequenceNumber) const
    {
        NasCount count;
        count.sqn = downlinkCount.sqn;
        count.overflow = downlinkCount.overflow;

        if (count.sqn > sequenceNumber)
            count.overflow = octet2(((int)count.overflow + 1) & 0xFFFF);
        count.sqn = sequenceNumber;
        return count;
    }

    void countOnEncrypt()
    {
        uplinkCount.sqn = static_cast<uint8_t>((((int)uplinkCount.sqn + 1) & 0xFF));
        if (uplinkCount.sqn == 0)
            uplinkCount.overflow = octet2(((int)uplinkCount.overflow + 1) & 0xFFFF);
    }

    void rollbackCountOnEncrypt()
    {
        if (uplinkCount.sqn == 0)
        {
            uplinkCount.sqn = 0xFF;

            if ((int)uplinkCount.overflow == 0)
                uplinkCount.overflow = octet2{0xFFFF};
            else
                uplinkCount.overflow = octet2{(int)uplinkCount.overflow - 1};
        }
        else
        {
            uplinkCount.sqn = static_cast<uint8_t>(((int)uplinkCount.sqn - 1) & 0xFF);
        }
    }

    [[nodiscard]] NasSecurityContext deepCopy() const
    {
        NasSecurityContext ctx;
        ctx.tsc = tsc;
        ctx.ngKsi = ngKsi;
        ctx.downlinkCount = downlinkCount;
        ctx.uplinkCount = uplinkCount;
        ctx.is3gppAccess = is3gppAccess;
        ctx.keys = keys.deepCopy();
        ctx.integrity = integrity;
        ctx.ciphering = ciphering;
        ctx.lastNasSequenceNums = lastNasSequenceNums;
        return ctx;
    }
};

enum class EAutnValidationRes
{
    OK,
    MAC_FAILURE,
    AMF_SEPARATION_BIT_FAILURE,
    SYNCHRONISATION_FAILURE,
};

enum class ERegUpdateCause
{
    // when the UE detects entering a tracking area that is not in the list of tracking areas that the UE previously
    // registered in the AMF
    ENTER_UNLISTED_TRACKING_AREA,
    // when the periodic registration updating timer T3512 expires
    T3512_EXPIRY,
    // when the UE receives a CONFIGURATION UPDATE COMMAND message indicating "registration requested" in the
    // Configuration update indication IE as specified in subclauses 5.4.4.3;
    CONFIGURATION_UPDATE,
    // when the UE in state 5GMM-REGISTERED.ATTEMPTING-REGISTRATION-UPDATE either receives a paging or the UE receives a
    // NOTIFICATION message with access type indicating 3GPP access over the non-3GPP access for PDU sessions associated
    // with 3GPP access
    PAGING_OR_NOTIFICATION,
    // upon inter-system change from S1 mode to N1 mode
    INTER_SYSTEM_CHANGE_S1_TO_N1,
    // when the UE receives an indication of "RRC Connection failure" from the lower layers and does not have signalling
    // pending (i.e. when the lower layer requests NAS signalling connection recovery) except for the case specified in
    // subclause 5.3.1.4;
    CONNECTION_RECOVERY,
    // when the UE receives a fallback indication from the lower layers and does not have signalling pending (i.e. when
    // the lower layer requests NAS signalling connection recovery, see subclauses 5.3.1.4 and 5.3.1.2);
    FALLBACK_INDICATION,
    // when the UE changes the 5GMM capability or the S1 UE network capability or both
    MM_OR_S1_CAPABILITY_CHANGE,
    // when the UE's usage setting changes
    USAGE_SETTING_CHANGE,
    // when the UE needs to change the slice(s) it is currently registered to
    SLICE_CHANGE,
    // when the UE changes the UE specific DRX parameters
    DRX_CHANGE,
    // when the UE in state 5GMM-REGISTERED.ATTEMPTING-REGISTRATION-UPDATE receives a request from the upper layers to
    // establish an emergency PDU session or perform emergency services fallback
    EMERGENCY_CASE,
    // when the UE needs to register for SMS over NAS, indicate a change in the requirements to use SMS over NAS, or
    // de-register from SMS over NAS;
    SMS_OVER_NAS_CHANGE,
    // when the UE needs to indicate PDU session status to the network after performing a local release of PDU
    // session(s) as specified in subclauses 6.4.1.5 and 6.4.3.5;
    PS_STATUS_INFORM,
    // when the UE in 5GMM-IDLE mode changes the radio capability for NG-RAN
    RADIO_CAP_CHANGE,
    // when the UE needs to request new LADN information
    NEW_LADN_NEEDED,
    // when the UE needs to request the use of MICO mode or needs to stop the use of MICO mode
    MICO_MODE_CHANGE,
    // when the UE in 5GMM-CONNECTED mode with RRC inactive indication enters a cell in the current registration area
    // belonging to an equivalent PLMN of the registered PLMN and not belonging to the registered PLMN;
    ENTER_EQUIVALENT_PLMN_CELL,
    // when the UE receives a SERVICE REJECT message with the 5GMM cause value set to #28 "Restricted service area".
    RESTRICTED_SERVICE_AREA,
    // ------ following are not specified by 24.501 ------
    TAI_CHANGE_IN_ATT_UPD,
    PLMN_CHANGE_IN_ATT_UPD,
    T3346_EXPIRY_IN_ATT_UPD,
    T3502_EXPIRY_IN_ATT_UPD,
    T3511_EXPIRY_IN_ATT_UPD,
};

enum class EServiceReqCause
{
    // a) the UE, in 5GMM-IDLE mode over 3GPP access, receives a paging request from the network
    IDLE_PAGING,
    // b) the UE, in 5GMM-CONNECTED mode over 3GPP access, receives a notification from the network with access type
    // indicating non-3GPP access
    CONNECTED_3GPP_NOTIFICATION_N3GPP,
    // c) the UE, in 5GMM-IDLE mode over 3GPP access, has uplink signalling pending
    IDLE_UPLINK_SIGNAL_PENDING,
    // d) the UE, in 5GMM-IDLE mode over 3GPP access, has uplink user data pending
    IDLE_UPLINK_DATA_PENDING,
    // e) the UE, in 5GMM-CONNECTED mode or in 5GMM-CONNECTED mode with RRC inactive indication, has user data pending
    // due to no user-plane resources established for PDU session(s) used for user data transport
    CONNECTED_UPLINK_DATA_PENDING,
    // f) the UE in 5GMM-IDLE mode over non-3GPP access, receives an indication from the lower layers of non-3GPP
    // access, that the access stratum connection is established between UE and network
    NON_3GPP_AS_ESTABLISHED,
    // g) the UE, in 5GMM-IDLE mode over 3GPP access, receives a notification from the network with access type
    // indicating 3GPP access when the UE is in 5GMM-CONNECTED mode over non-3GPP access
    IDLE_3GPP_NOTIFICATION_N3GPP,
    // h) the UE, in 5GMM-IDLE, 5GMM-CONNECTED mode over 3GPP access, or 5GMM-CONNECTED mode with RRC inactive
    // indication, receives a request for emergency services fallback from the upper layer and performs emergency
    // services fallback as specified in subclause 4.13.4.2 of 3GPP TS 23.502 [9]
    EMERGENCY_FALLBACK,
    // i) the UE, in 5GMM-CONNECTED mode over 3GPP access or in 5GMM-CONNECTED mode with RRC inactive indication,
    // receives a fallback indication from the lower layers (see subclauses 5.3.1.2 and 5.3.1.4) and or the UE has a
    // pending NAS procedure other than a registration, service request, or de-registration procedure
    FALLBACK_INDICATION
};

enum class EProcRc
{
    OK,
    CANCEL,
    STAY,
};

struct ProcControl
{
    std::optional<EInitialRegCause> initialRegistration{};
    std::optional<ERegUpdateCause> mobilityRegistration{};
    std::optional<EServiceReqCause> serviceRequest{};
    std::optional<EDeregCause> deregistration{};
};

struct UacInput
{
    std::bitset<16> identities;
    int category{};
    int establishmentCause{};
};

enum class EUacResult
{
    ALLOWED,
    BARRED,
    BARRING_APPLICABLE_EXCEPT_0_2,
};

struct UacOutput
{
    EUacResult res{};
};

enum class ENasTransportHint
{
    PDU_SESSION_ESTABLISHMENT_REQUEST,
    PDU_SESSION_ESTABLISHMENT_ACCEPT,
    PDU_SESSION_ESTABLISHMENT_REJECT,

    PDU_SESSION_AUTHENTICATION_COMMAND,
    PDU_SESSION_AUTHENTICATION_COMPLETE,
    PDU_SESSION_AUTHENTICATION_RESULT,

    PDU_SESSION_MODIFICATION_REQUEST,
    PDU_SESSION_MODIFICATION_REJECT,
    PDU_SESSION_MODIFICATION_COMMAND,
    PDU_SESSION_MODIFICATION_COMPLETE,
    PDU_SESSION_MODIFICATION_COMMAND_REJECT,

    PDU_SESSION_RELEASE_REQUEST,
    PDU_SESSION_RELEASE_REJECT,
    PDU_SESSION_RELEASE_COMMAND,
    PDU_SESSION_RELEASE_COMPLETE,

    FIVEG_SM_STATUS
};


struct RGnbUeConfig //copied from ue/types.hpp
{
    /* Read from config file */
    std::optional<Supi> supi{};
    int protectionScheme;
    int homeNetworkPublicKeyId;
    OctetString homeNetworkPublicKey{};
    std::optional<std::string> routingIndicator{};
    Plmn hplmn{};
    OctetString key{};
    OctetString opC{};
    OpType opType{};
    OctetString amf{};
    std::optional<std::string> imei{};
    std::optional<std::string> imeiSv{};
    SupportedAlgs supportedAlgs{};
    std::vector<std::string> gnbSearchList{};
    std::vector<SessionConfig> defaultSessions{};
    IntegrityMaxDataRateConfig integrityMaxRate{};
    NetworkSlice defaultConfiguredNssai{};
    NetworkSlice configuredNssai{};
    std::optional<std::string> tunName{};

    struct
    {
        bool mps{};
        bool mcs{};
    } uacAic;

    struct
    {
        int normalCls{}; // [0..9]
        bool cls11{};
        bool cls12{};
        bool cls13{};
        bool cls14{};
        bool cls15{};
    } uacAcc;

    /* Assigned by program */
    bool configureRouting{};
    bool prefixLogger{};

    [[nodiscard]] std::string getNodeName() const
    {
        if (supi.has_value())
            return ToJson(supi).str();
        if (imei.has_value())
            return "imei-" + *imei;
        if (imeiSv.has_value())
            return "imeisv-" + *imeiSv;
        return "unknown-ue";
    }

    [[nodiscard]] std::string getLoggerPrefix() const
    {
        if (!prefixLogger)
            return "";
        if (supi.has_value())
            return supi->value + "|";
        if (imei.has_value())
            return *imei + "|";
        if (imeiSv.has_value())
            return *imeiSv + "|";
        return "unknown-ue|";
    }
};

// Merged Types
struct TaskBase // modified to include both the UE and the gNB parts of the rGNB
{
    // Config
    RGnbGnbConfig *gnbConfig{};
    RGnbUeConfig *ueConfig{};

    // UE specific
    app::IUeController *ueController{};
    RGNodeB *rgnb{}; // TODO: The UE Task Base has the associated, might be needed for switchoff of UEs
    UeSharedContext shCtx{};

    LogBase *logBase{};
    app::INodeListener *nodeListener{};
    NtsTask *cliCallbackTask{};

    // gNB Part
    GnbAppTask *gnbAppTask{};
    GtpTask *gnbGtpTask{};
    NgapTask *gnbNgapTask{};
    GnbRrcTask *gnbRrcTask{};
    SctpTask *gnbSctpTask{};
    GnbRlsTask *gnbRlsTask{};

    // UE Part
    UeAppTask *ueAppTask{};
    UeRrcTask *ueRrcTask{};
    NasTask *ueNasTask{};
    UeRlsTask *ueRlsTask{};
};

// GNB functions
Json ToJson(const RGnbStatusInfo &v);
Json ToJson(const RGnbGnbConfig &v);
Json ToJson(const NgapAmfContext &v);
Json ToJson(const EAmfState &v);
Json ToJson(const EPagingDrx &v);
Json ToJson(const SctpAssociation &v);
Json ToJson(const ServedGuami &v);
Json ToJson(const Guami &v);

// UE functions
Json ToJson(const ECmState &state);
Json ToJson(const ERmState &state);
Json ToJson(const EMmState &state);
Json ToJson(const EMmSubState &state);
Json ToJson(const E5UState &state);
Json ToJson(const NasTimers &v);
Json ToJson(const ERegUpdateCause &v);
Json ToJson(const EPsState &v);
Json ToJson(const EServiceReqCause &v);
Json ToJson(const ERrcState &v);

} // namespace nr::rgnb