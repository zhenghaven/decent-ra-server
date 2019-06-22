#pragma once

#include <DecentApi/Common/Net/ConnectionHandler.h>

#include <DecentApi/Common/Net/RpcWriter.h>

namespace Decent
{
	namespace IasSim
	{
		class IasSimApp : public Net::ConnectionHandler
		{
		public:
			IasSimApp();

			virtual ~IasSimApp();

			virtual bool ProcessSmartMessage(const std::string& category, Net::ConnectionBase& connection, Net::ConnectionBase*& freeHeldCnt) override;

		private:
			Net::RpcWriter m_sigRlRpc;
			Net::RpcWriter m_reportRpc;
		};
	}
}
