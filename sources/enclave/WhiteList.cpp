#include <DecentApi/Common/Ra/WhiteList/HardCoded.h>

using namespace Decent::Ra::WhiteList;

HardCoded::HardCoded() :
	StaticTypeList(
		{
			std::make_pair<std::string, std::string>(HardCoded::sk_decentServerLabel, ""),
		}
		)
{
}
