//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <thread>

#include <unistd.h>

#include <rgnb/rgnb.hpp>
#include <lib/app/base_app.hpp>
#include <lib/app/cli_base.hpp>
#include <lib/app/cli_cmd.hpp>
#include <lib/app/proc_table.hpp>
#include <lib/app/ue_ctl.hpp>
#include <utils/io.hpp>
#include <utils/common.hpp>
#include <utils/concurrent_map.hpp>
#include <utils/constants.hpp>
#include <utils/options.hpp>
#include <utils/yaml_utils.hpp>
#include <yaml-cpp/yaml.h>

//static app::CliServer *g_cliServer = nullptr;
// use two config files, one for the GNB part, one for the UE part
static nr::rgnb::RGnbGnbConfig *gnb_refConfig = nullptr;
static nr::rgnb::RGnbUeConfig *ue_refConfig = nullptr;
static ConcurrentMap<std::string, nr::rgnb::RGNodeB *> g_rgnbMap{}; // TODO: The gnb uses an unordered map while the ue file uses a concurrent map
//static app::CliResponseTask *g_cliRespTask = nullptr;

static struct Options
{
    std::string ueConfigFile{};
    std::string gnbConfigFile{};
    bool noRoutingConfigs{}; // copied from ue.cpp
    bool disableCmd{};
    std::string imsi{}; // copied from ue.cpp
    int count{}; // copied from ue.cpp
    int tempo{}; // copied from ue.cpp
} g_options{};

struct NwUeControllerCmd : NtsMessage // copied from ue.cp
{
    enum PR
    {
        PERFORM_SWITCH_OFF,
    } present;

    // PERFORM_SWITCH_OFF
    nr::ue::UserEquipment *ue{};

    explicit NwUeControllerCmd(PR present) : NtsMessage(NtsMessageType::UE_CTL_COMMAND), present(present)
    {
    }
};

// performSwitchOff would be initiated by the app layer.
class UeControllerTask : public NtsTask // copied from ue.cp
{
  protected:
    void onStart() override
    {
    }

    void onLoop() override
    {
        // TODO: Perform Switch Off for UE if APP layer initiates it, how does my different data structure affect this?
        // Also, the rgnb should not just switch off when the UE part is unable to establish a connection
        auto msg = take();
        if (msg == nullptr)
            return;
//        if (msg->msgType == NtsMessageType::UE_CTL_COMMAND)
//        {
//            auto &w = dynamic_cast<NwUeControllerCmd &>(*msg);
//            switch (w.present)
//            {
//            case NwUeControllerCmd::PERFORM_SWITCH_OFF: { // TODO: Perform Switch Off
//                std::string key{};
//                g_ueMap.invokeForeach([&key, &w](auto &item) {
//                    if (item.second == w.ue)
//                        key = item.first;
//                });
//
//                if (key.empty())
//                    return;
//
//                if (g_ueMap.removeAndGetSize(key) == 0)
//                    exit(0);
//
//                delete w.ue;
//                break;
//            }
//            }
//        }
    }

    void onQuit() override
    {
    }
};

static UeControllerTask *g_controllerTask; // copied from ue.cp

static nr::rgnb::RGnbGnbConfig *ReadGnbConfigYaml()
{
    auto *result = new nr::rgnb::RGnbGnbConfig();
    auto config = YAML::LoadFile(g_options.gnbConfigFile);

    result->plmn.mcc = yaml::GetInt32(config, "mcc", 1, 999);
    yaml::GetString(config, "mcc", 3, 3);
    result->plmn.mnc = yaml::GetInt32(config, "mnc", 0, 999);
    result->plmn.isLongMnc = yaml::GetString(config, "mnc", 2, 3).size() != 2;

    result->nci = yaml::GetInt64(config, "nci", 0, 0xFFFFFFFFFll);
    result->gnbIdLength = yaml::GetInt32(config, "idLength", 22, 32);
    result->tac = yaml::GetInt32(config, "tac", 0, 0xFFFFFF);

    result->linkIp = yaml::GetIpAddress(config, "linkIp");
    result->ngapIp = yaml::GetIpAddress(config, "ngapIp");
    result->gtpIp = yaml::GetIpAddress(config, "gtpIp");

    if (yaml::HasField(config, "gtpAdvertiseIp"))
        result->gtpAdvertiseIp = yaml::GetIpAddress(config, "gtpAdvertiseIp");

    result->ignoreStreamIds = yaml::GetBool(config, "ignoreStreamIds");
    result->pagingDrx = EPagingDrx::V128;
    result->name = "UERANSIM-gnb-" + std::to_string(result->plmn.mcc) + "-" + std::to_string(result->plmn.mnc) + "-" +
                   std::to_string(result->getGnbId()); // NOTE: Avoid using "/" dir separator character.

    for (auto &amfConfig : yaml::GetSequence(config, "amfConfigs"))
    {
        nr::rgnb::GnbAmfConfig c{};
        c.address = yaml::GetIpAddress(amfConfig, "address");
        c.port = static_cast<uint16_t>(yaml::GetInt32(amfConfig, "port", 1024, 65535));
        result->amfConfigs.push_back(c);
    }

    for (auto &nssai : yaml::GetSequence(config, "slices"))
    {
        SingleSlice s{};
        s.sst = yaml::GetInt32(nssai, "sst", 0, 0xFF);
        if (yaml::HasField(nssai, "sd"))
            s.sd = octet3{yaml::GetInt32(nssai, "sd", 0, 0xFFFFFF)};
        result->nssai.slices.push_back(s);
    }

    return result;
}

static nr::rgnb::RGnbUeConfig *ReadUeConfigYaml()
{
    auto *result = new nr::rgnb::RGnbUeConfig();
    auto config = YAML::LoadFile(g_options.ueConfigFile);

    result->hplmn.mcc = yaml::GetInt32(config, "mcc", 1, 999);
    yaml::GetString(config, "mcc", 3, 3);
    result->hplmn.mnc = yaml::GetInt32(config, "mnc", 0, 999);
    result->hplmn.isLongMnc = yaml::GetString(config, "mnc", 2, 3).size() == 3;
    if (yaml::HasField(config, "routingIndicator"))
        result->routingIndicator = yaml::GetString(config, "routingIndicator", 1, 4);

    for (auto &gnbSearchItem : yaml::GetSequence(config, "gnbSearchList"))
        result->gnbSearchList.push_back(gnbSearchItem.as<std::string>());

    if (yaml::HasField(config, "default-nssai"))
    {
        for (auto &sNssai : yaml::GetSequence(config, "default-nssai"))
        {
            SingleSlice s{};
            s.sst = yaml::GetInt32(sNssai, "sst", 0, 0xFF);
            if (yaml::HasField(sNssai, "sd"))
                s.sd = octet3{yaml::GetInt32(sNssai, "sd", 0, 0xFFFFFF)};
            result->defaultConfiguredNssai.slices.push_back(s);
        }
    }

    if (yaml::HasField(config, "configured-nssai"))
    {
        for (auto &sNssai : yaml::GetSequence(config, "configured-nssai"))
        {
            SingleSlice s{};
            s.sst = yaml::GetInt32(sNssai, "sst", 0, 0xFF);
            if (yaml::HasField(sNssai, "sd"))
                s.sd = octet3{yaml::GetInt32(sNssai, "sd", 0, 0xFFFFFF)};
            result->configuredNssai.slices.push_back(s);
        }
    }

    result->key = OctetString::FromHex(yaml::GetString(config, "key", 32, 32));
    result->opC = OctetString::FromHex(yaml::GetString(config, "op", 32, 32));
    result->amf = OctetString::FromHex(yaml::GetString(config, "amf", 4, 4));

    result->configureRouting = !g_options.noRoutingConfigs;

    // If we have multiple UEs in the same process, then log names should be separated.
    result->prefixLogger = g_options.count > 1;

    if (yaml::HasField(config, "supi"))
        result->supi = Supi::Parse(yaml::GetString(config, "supi"));
    if (yaml::HasField(config, "protectionScheme"))
        result->protectionScheme = yaml::GetInt32(config, "protectionScheme", 0, 255);
    if (yaml::HasField(config, "homeNetworkPublicKeyId"))
        result->homeNetworkPublicKeyId = yaml::GetInt32(config, "homeNetworkPublicKeyId", 0, 255);
    if (yaml::HasField(config, "homeNetworkPublicKey"))
        result->homeNetworkPublicKey = OctetString::FromHex(yaml::GetString(config, "homeNetworkPublicKey", 64, 64));
    if (yaml::HasField(config, "imei"))
        result->imei = yaml::GetString(config, "imei", 15, 15);
    if (yaml::HasField(config, "imeiSv"))
        result->imeiSv = yaml::GetString(config, "imeiSv", 16, 16);
    if (yaml::HasField(config, "tunName"))
        result->tunName = yaml::GetString(config, "tunName", 1, 12);

    yaml::AssertHasField(config, "integrity");
    yaml::AssertHasField(config, "ciphering");

    result->supportedAlgs.nia1 = yaml::GetBool(config["integrity"], "IA1");
    result->supportedAlgs.nia2 = yaml::GetBool(config["integrity"], "IA2");
    result->supportedAlgs.nia3 = yaml::GetBool(config["integrity"], "IA3");
    result->supportedAlgs.nea1 = yaml::GetBool(config["ciphering"], "EA1");
    result->supportedAlgs.nea2 = yaml::GetBool(config["ciphering"], "EA2");
    result->supportedAlgs.nea3 = yaml::GetBool(config["ciphering"], "EA3");

    std::string opType = yaml::GetString(config, "opType");
    if (opType == "OP")
        result->opType = nr::rgnb::OpType::OP;
    else if (opType == "OPC")
        result->opType = nr::rgnb::OpType::OPC;
    else
        throw std::runtime_error("Invalid OP type: " + opType);

    if (yaml::HasField(config, "sessions"))
    {
        for (auto &sess : yaml::GetSequence(config, "sessions"))
        {
            nr::rgnb::SessionConfig s{};

            if (yaml::HasField(sess, "apn"))
                s.apn = yaml::GetString(sess, "apn");
            if (yaml::HasField(sess, "slice"))
            {
                auto slice = sess["slice"];
                s.sNssai = SingleSlice{};
                s.sNssai->sst = yaml::GetInt32(slice, "sst", 0, 0xFF);
                if (yaml::HasField(slice, "sd"))
                    s.sNssai->sd = octet3{yaml::GetInt32(slice, "sd", 0, 0xFFFFFF)};
            }

            std::string type = yaml::GetString(sess, "type");
            if (type == "IPv4")
                s.type = nas::EPduSessionType::IPV4;
            else if (type == "IPv6")
                s.type = nas::EPduSessionType::IPV6;
            else if (type == "IPv4v6")
                s.type = nas::EPduSessionType::IPV4V6;
            else if (type == "Ethernet")
                s.type = nas::EPduSessionType::ETHERNET;
            else if (type == "Unstructured")
                s.type = nas::EPduSessionType::UNSTRUCTURED;
            else
                throw std::runtime_error("Invalid PDU session type: " + type);

            s.isEmergency = false;

            result->defaultSessions.push_back(s);
        }
    }

    yaml::AssertHasField(config, "integrityMaxRate");
    {
        auto uplink = yaml::GetString(config["integrityMaxRate"], "uplink");
        auto downlink = yaml::GetString(config["integrityMaxRate"], "downlink");
        if (uplink != "full" && uplink != "64kbps")
            throw std::runtime_error("Invalid integrity protection maximum uplink data rate: " + uplink);
        if (downlink != "full" && downlink != "64kbps")
            throw std::runtime_error("Invalid integrity protection maximum downlink data rate: " + downlink);
        result->integrityMaxRate.uplinkFull = uplink == "full";
        result->integrityMaxRate.downlinkFull = downlink == "full";
    }

    yaml::AssertHasField(config, "uacAic");
    {
        result->uacAic.mps = yaml::GetBool(config["uacAic"], "mps");
        result->uacAic.mcs = yaml::GetBool(config["uacAic"], "mcs");
    }

    yaml::AssertHasField(config, "uacAcc");
    {
        result->uacAcc.normalCls = yaml::GetInt32(config["uacAcc"], "normalClass", 0, 9);
        result->uacAcc.cls11 = yaml::GetBool(config["uacAcc"], "class11");
        result->uacAcc.cls12 = yaml::GetBool(config["uacAcc"], "class12");
        result->uacAcc.cls13 = yaml::GetBool(config["uacAcc"], "class13");
        result->uacAcc.cls14 = yaml::GetBool(config["uacAcc"], "class14");
        result->uacAcc.cls15 = yaml::GetBool(config["uacAcc"], "class15");
    }

    return result;
}

static void ReadOptions(int argc, char **argv)
{
    opt::OptionsDescription desc{cons::Project,
                                 cons::Tag,
                                 "5G-SA relay gNB implementation",
                                 cons::Owner,
                                 "nr-rgnb",
                                 {"-gnb <gnb-config-file> -u <ue-config-file> [option...]"},
                                 {},
                                 true,
                                 false};

    opt::OptionItem itemGnbConfigFile = {'g', "config", "Use specified gNodeB configuration file for relay gNB", "gnb-config-file"};
    opt::OptionItem itemUeConfigFile = {'u', "config", "Use specified UE configuration file for relay gNB", "ue-config-file"};
//    opt::OptionItem itemDisableCmd = {'l', "disable-cmd", "Disable command line functionality for this instance", std::nullopt};

    desc.items.push_back(itemGnbConfigFile);
    desc.items.push_back(itemUeConfigFile);
//    desc.items.push_back(itemDisableCmd);

    opt::OptionsResult opt{argc, argv, desc, false, nullptr};

//    if (opt.hasFlag(itemDisableCmd))
//        g_options.disableCmd = true;
    g_options.gnbConfigFile = opt.getOption(itemGnbConfigFile);
    g_options.ueConfigFile = opt.getOption(itemUeConfigFile);

    try
    {
        gnb_refConfig = ReadGnbConfigYaml();
        ue_refConfig = ReadUeConfigYaml();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }
}

static std::string LargeSum(std::string a, std::string b)  // copied from ue.cpp
{
    if (a.length() > b.length())
        std::swap(a, b);

    std::string str;
    size_t n1 = a.length(), n2 = b.length();

    reverse(a.begin(), a.end());
    reverse(b.begin(), b.end());

    int carry = 0;
    for (size_t i = 0; i < n1; i++)
    {
        int sum = ((a[i] - '0') + (b[i] - '0') + carry);
        str.push_back(static_cast<char>((sum % 10) + '0'));
        carry = sum / 10;
    }
    for (size_t i = n1; i < n2; i++)
    {
        int sum = ((b[i] - '0') + carry);
        str.push_back(static_cast<char>((sum % 10) + '0'));
        carry = sum / 10;
    }
    if (carry)
        throw std::runtime_error("UE serial number overflow");
    reverse(str.begin(), str.end());
    return str;
}

static void IncrementNumber(std::string &s, int delta) // copied from ue.cpp
{
    s = LargeSum(s, std::to_string(delta));
}

//static nr::rgnb::RGnbUeConfig *GetConfigByUe(int ueIndex) // copied from ue.cpp
//{
//    auto *c = new nr::rgnb::RGnbUeConfig();
//    c->key = ue_refConfig->key.copy();
//    c->opC = ue_refConfig->opC.copy();
//    c->opType = ue_refConfig->opType;
//    c->amf = ue_refConfig->amf.copy();
//    c->imei = ue_refConfig->imei;
//    c->imeiSv = ue_refConfig->imeiSv;
//    c->supi = ue_refConfig->supi;
//    c->protectionScheme = ue_refConfig->protectionScheme;
//    c->homeNetworkPublicKey = ue_refConfig->homeNetworkPublicKey.copy();
//    c->homeNetworkPublicKeyId = ue_refConfig->homeNetworkPublicKeyId;
//    c->routingIndicator = ue_refConfig->routingIndicator;
//    c->tunName = ue_refConfig->tunName;
//    c->hplmn = ue_refConfig->hplmn;
//    c->configuredNssai = ue_refConfig->configuredNssai;
//    c->defaultConfiguredNssai = ue_refConfig->defaultConfiguredNssai;
//    c->supportedAlgs = ue_refConfig->supportedAlgs;
//    c->gnbSearchList = ue_refConfig->gnbSearchList;
//    c->defaultSessions = ue_refConfig->defaultSessions;
//    c->configureRouting = ue_refConfig->configureRouting;
//    c->prefixLogger = ue_refConfig->prefixLogger;
//    c->integrityMaxRate = ue_refConfig->integrityMaxRate;
//    c->uacAic = ue_refConfig->uacAic;
//    c->uacAcc = ue_refConfig->uacAcc;
//
//    if (c->supi.has_value())
//        IncrementNumber(c->supi->value, ueIndex);
//    if (c->imei.has_value())
//        IncrementNumber(*c->imei, ueIndex);
//    if (c->imeiSv.has_value())
//        IncrementNumber(*c->imeiSv, ueIndex);
//
//    return c;
//}

//static void ReceiveCommand(app::CliMessage &msg)
//{
//    if (msg.value.empty())
//    {
//        g_cliServer->sendMessage(app::CliMessage::Result(msg.clientAddr, ""));
//        return;
//    }
//
//    std::vector<std::string> tokens{};
//
//    auto exp = opt::PerformExpansion(msg.value, tokens);
//    if (exp != opt::ExpansionResult::SUCCESS)
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, "Invalid command: " + msg.value));
//        return;
//    }
//
//    if (tokens.empty())
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, "Empty command"));
//        return;
//    }
//
//    std::string error{}, output{};
//    auto cmd = app::ParseGnbCliCommand(std::move(tokens), error, output);
//    if (!error.empty())
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, error));
//        return;
//    }
//    if (!output.empty())
//    {
//        g_cliServer->sendMessage(app::CliMessage::Result(msg.clientAddr, output));
//        return;
//    }
//    if (cmd == nullptr)
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, ""));
//        return;
//    }
//
//    if (g_rgnbMap.count(msg.nodeName) == 0)
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, "Node not found: " + msg.nodeName));
//        return;
//    }
//
//    auto *rgnb = g_rgnbMap[msg.nodeName];
//    rgnb->pushCommand(std::move(cmd), msg.clientAddr);
//}

static void Loop()
{
    // removing all the client interaction stuff otherwise leads to the loop being empty
    ::pause();

//    if (!g_cliServer)
//    {
//        ::pause();
//        return;
//    }

//    auto msg = g_cliServer->receiveMessage();
//    if (msg.type == app::CliMessage::Type::ECHO)
//    {
//        g_cliServer->sendMessage(msg);
//        return;
//    }
//
//    if (msg.type != app::CliMessage::Type::COMMAND)
//        return;
//
//    if (msg.value.size() > 0xFFFF)
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, "Command is too large"));
//        return;
//    }
//
//    if (msg.nodeName.size() > 0xFFFF)
//    {
//        g_cliServer->sendMessage(app::CliMessage::Error(msg.clientAddr, "Node name is too large"));
//        return;
//    }
//
//    ReceiveCommand(msg);
}

static class UeController : public app::IUeController
{
  public:
    void performSwitchOff(nr::ue::UserEquipment *ue) override
    {
        auto w = std::make_unique<NwUeControllerCmd>(NwUeControllerCmd::PERFORM_SWITCH_OFF);
        w->ue = ue;
        g_controllerTask->push(std::move(w));
    }
} g_ueController;

int main(int argc, char **argv)
{
    app::Initialize();
    ReadOptions(argc, argv);

    std::cout << cons::Name << std::endl; // print UERANSIM and Version number

//    if (!g_options.disableCmd)
//    {
//        g_cliServer = new app::CliServer{};
//        g_cliRespTask = new app::CliResponseTask(g_cliServer);
//    }

    auto *rgnb = new nr::rgnb::RGNodeB(gnb_refConfig, ue_refConfig, &g_ueController, nullptr);
    g_rgnbMap.put(gnb_refConfig->name, rgnb);
//    g_rgnbMap[gnb_refConfig->name] = rgnb;

//    if (!g_options.disableCmd)
//    {
//        app::CreateProcTable(g_rgnbMap, g_cliServer->assignedAddress().getPort());
//        g_cliRespTask->start();
//    }

    rgnb->start();

    while (true)
        Loop();
}
