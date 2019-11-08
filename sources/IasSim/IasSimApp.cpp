#include "IasSimApp.h"

#include <chrono>
#include <thread>
#include <random>

#include <DecentApi/Common/Common.h>
#include <DecentApi/Common/Net/ConnectionBase.h>

#include "IasRespSamples.h"

using namespace Decent;
using namespace Decent::Net;
using namespace Decent::IasSim;

//#define TURN_OFF_DELAY_SIMULATION

namespace
{
	static constexpr double CalcSquare(double x)
	{
		return x * x;
	}

	/**
	 * \brief	Calculates the alpha for gamma distribution. Based on
	 * 			https://stats.stackexchange.com/questions/107679/how-to-get-only-positive-values-when-imputing-data
	 *
	 * \param	avg   	The average.
	 * \param	stdDev	The standard deviation.
	 *
	 * \return	The calculated alpha.
	 */
	static constexpr double CalcAlpha(double avg, double stdDev)
	{
		return CalcSquare(avg / stdDev);
	}

	/**
	 * \brief	Calculates the beta for gamma distribution. Based on
	 * 			https://stats.stackexchange.com/questions/107679/how-to-get-only-positive-values-when-imputing-data
	 *
	 * \param	avg   	The average.
	 * \param	stdDev	The standard deviation.
	 *
	 * \return	The calculated beta.
	 */
	static constexpr double CalcBeta(double avg, double stdDev)
	{
		return CalcSquare(stdDev) / avg;
	}

	static std::chrono::duration<double, std::milli> GetSigRlDelay()
	{
#ifndef TURN_OFF_DELAY_SIMULATION
		//Average value calculated based on the data collected from the IAS web portal
		static constexpr auto avg = 39.00; // ms;

		//Standard deviation calculated based on the data collected from the IAS web portal
		static constexpr auto stdDev = 23.79;

		static constexpr auto alpha = CalcAlpha(avg, stdDev);
		static constexpr auto beta = CalcBeta(avg, stdDev);

		static thread_local std::random_device rd;
		static thread_local std::mt19937 generator(rd());
		static thread_local std::gamma_distribution<double> dist(alpha, beta);

		double result = dist(generator);
		while (result > 10000.00)
		{
			PRINT_I("Discarded a too large sample - %f", result);
			result = dist(generator);
		}

		return std::chrono::duration<double, std::milli>(result);
#else
		using namespace std::chrono_literals;
		return 0ms;
#endif // !TURN_OFF_DELAY_SIMULATION

	}

	static std::chrono::duration<double, std::milli> GetReportDelay()
	{
#ifndef TURN_OFF_DELAY_SIMULATION
		//Average value calculated based on the data collected from the IAS web portal
		constexpr auto avg = 255.43; // ms;

		//Standard deviation calculated based on the data collected from the IAS web portal
		constexpr auto stdDev = 70.00;

		static constexpr auto alpha = CalcAlpha(avg, stdDev);
		static constexpr auto beta = CalcBeta(avg, stdDev);

		static thread_local std::random_device rd;
		static thread_local std::mt19937 generator(rd());
		static thread_local std::gamma_distribution<double> dist(alpha, beta);

		double result = dist(generator);
		while (result > 10000.00)
		{
			PRINT_I("Discarded a too large sample - %f", result);
			result = dist(generator);
		}

		return std::chrono::duration<double, std::milli>(result);
#else
		using namespace std::chrono_literals;
		return 0ms;
#endif // !TURN_OFF_DELAY_SIMULATION
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
		std::string gidStr = connection.RecvContainer<std::string>();

		std::this_thread::sleep_for(GetSigRlDelayNeeded());

		connection.SendRpc(m_sigRlRpc);
	}
	else if (category == "Report")
	{
		std::string reqStr = connection.RecvContainer<std::string>();

		std::this_thread::sleep_for(GetReportDelayNeeded());

		connection.SendRpc(m_reportRpc);
	}
	return false;
}
