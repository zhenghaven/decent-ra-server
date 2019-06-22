#include "IasSimApp.h"

#include <DecentApi/Common/Net/ConnectionBase.h>

#include "IasRespSamples.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::IasSim;

IasSimApp::IasSimApp() :
	m_sigRlRpc(RpcWriter::CalcSizeStr(sizeof(gsk_sigRl) - 1), 1),
	m_reportRpc(RpcWriter::CalcSizeStr(sizeof(gsk_report) - 1) +
		RpcWriter::CalcSizeStr(sizeof(gsk_signature)) +
		RpcWriter::CalcSizeStr(sizeof(gsk_cert)), 3)
{
	auto sigRlStr = m_sigRlRpc.AddStringArg(sizeof(gsk_sigRl) - 1);

	sigRlStr.Fill(gsk_sigRl);

	auto reportStr = m_reportRpc.AddStringArg(sizeof(gsk_report) - 1);
	auto signStr = m_reportRpc.AddStringArg(sizeof(gsk_signature) - 1);
	auto certStr = m_reportRpc.AddStringArg(sizeof(gsk_cert) - 1);

	reportStr.Fill(gsk_report);
	signStr.Fill(gsk_signature);
	certStr.Fill(gsk_cert);
}

IasSimApp::~IasSimApp()
{
}

bool IasSimApp::ProcessSmartMessage(const std::string & category, Net::ConnectionBase & connection, Net::ConnectionBase *& freeHeldCnt)
{
	if (category == "SigRl")
	{
		std::string gidStr;
		connection.ReceivePack(gidStr);
		
		const auto& data = m_sigRlRpc.GetBinaryArray();

		if (m_sigRlRpc.HasSizeAtFront())
		{
			connection.SendRaw(data.data(), data.size());
		}
		else
		{
			connection.SendPack(data);
		}
	}
	else if (category == "Report")
	{
		std::string reqStr;
		connection.ReceivePack(reqStr);

		const auto& data = m_reportRpc.GetBinaryArray();

		if (m_reportRpc.HasSizeAtFront())
		{
			connection.SendRaw(data.data(), data.size());
		}
		else
		{
			connection.SendPack(data);
		}
	}
	return false;
}
