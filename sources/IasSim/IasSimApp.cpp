#include "IasSimApp.h"

#include <chrono>
#include <thread>

#include <DecentApi/Common/Net/ConnectionBase.h>

#include "IasRespSamples.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::IasSim;

namespace
{
	static std::chrono::duration<double, std::milli> GetSigRlDelay()
	{
		using namespace std::chrono_literals;
		return 30ms;
	}

	static std::chrono::duration<double, std::milli> GetReportDelay()
	{
		using namespace std::chrono_literals;
		return 227ms;
	}

	static std::chrono::duration<double, std::milli> GetSigRlDelayNeeded()
	{
		using namespace std::chrono_literals;

		auto start = std::chrono::high_resolution_clock::now();
		auto sampleDelay = GetSigRlDelay();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> elapsed = end - start;

		auto needed = sampleDelay - elapsed;
		return needed > 0ms ? needed : 0ms;
	}

	static std::chrono::duration<double, std::milli> GetReportDelayNeeded()
	{
		using namespace std::chrono_literals;

		auto start = std::chrono::high_resolution_clock::now();
		auto sampleDelay = GetReportDelay();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> elapsed = end - start;

		auto needed = sampleDelay - elapsed;
		return needed > 0ms ? needed : 0ms;
	}
}

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

		std::this_thread::sleep_for(GetSigRlDelayNeeded());

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

		std::this_thread::sleep_for(GetReportDelayNeeded());

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
