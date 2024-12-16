#include "uuid.h"
#include <random>

// We use system generator.
#define UUID_SYSTEM_GENERATOR
#include <uuid.h>

namespace engine
{
	UUID buildUUID()
	{
		static std::random_device randomDevice;
		static std::mt19937 engine(randomDevice());
		static uuids::uuid_random_generator gen{engine};
		return uuids::to_string(gen());
	}

    // Simple random machine 64 bit uuid generator.
	UUID64u buildRuntimeUUID64u()
	{
		static std::random_device randomDevice;
		static std::mt19937_64 engine(randomDevice());
		static std::uniform_int_distribution<uint64_t> uniformDistribution;
		return uniformDistribution(engine);
	}
}