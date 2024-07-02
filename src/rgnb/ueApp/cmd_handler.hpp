//
// This file is a part of UERANSIM project.
// Copyright (c) 2023 ALİ GÜNGÖR.
//
// https://github.com/aligungr/UERANSIM/
// See README, LICENSE, and CONTRIBUTING files for licensing details.
//

#pragma once

#include <rgnb/nts.hpp>
#include <rgnb/types.hpp>

namespace nr::rgnb
{

class UeCmdHandler
{
  private:
    TaskBase *m_base;

  public:
    explicit UeCmdHandler(TaskBase *base) : m_base(base)
    {
    }

    void handleCmd(NmUeCliCommand &msg);

  private:
    void pauseTasks();
    void unpauseTasks();
    bool isAllPaused();

  private:
    void handleCmdImpl(NmUeCliCommand &msg);

  private:
    void sendResult(const InetAddress &address, const std::string &output);
    void sendError(const InetAddress &address, const std::string &output);
};

} // namespace nr::gnb
