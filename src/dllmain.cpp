#include <string>

#include <Mod/CppUserModBase.hpp>

#include "_structs.hpp"
#include "updator_handler.cpp"
#include "Config/config_reader.hpp"

class PopulationControl : public RC::CppUserModBase {
private:
  int last_tick_attempt = 0;
  int current_tick = 0;
  const int tick_delay = 1800;

  PopulationConfig::PopulationLimiterConfig LimiterConfig;

public:
  PopulationControl() : CppUserModBase()
  {
    ModName = STR("PopulationControl");
    ModVersion = STR("1.0");
    ModDescription = STR("Hehe");
    ModAuthors = STR("Shiza");
  }

  auto on_unreal_init() -> void override {
    if (ModConfigReader::LoadModConfig(&LimiterConfig)) return;

    PopulationHandler::Initialize();
  }

  auto on_update() -> void override {
    if (++current_tick < last_tick_attempt + tick_delay) return;
    last_tick_attempt = current_tick;
    PopulationHandler::Fire(LimiterConfig);
  }
};

#define KISMET_DEBUGGER_MOD_API __declspec(dllexport)
extern "C"
{
  KISMET_DEBUGGER_MOD_API RC::CppUserModBase* start_mod()
  {
    return new PopulationControl();
  }

  KISMET_DEBUGGER_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
  {
    delete mod;
  }
}