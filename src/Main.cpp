
#include "version.h"
#include <stddef.h>
#include <cmath>

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    const std::string PapyrusClassName = "State of Dress";
    constexpr std::uint32_t serializationDataVersion = 1;
    const SKSE::SerializationInterface* g_serialization;

    SKSE::RegistrationSet<const RE::TESObjectREFR*> evt{"morphCatch"sv};
    void Serialization_Revert(SKSE::SerializationInterface* serializationInterface) {
        logger::info("Serialization_revert begin");
        evt.Revert(serializationInterface);
    }
    void Serialization_Save(SKSE::SerializationInterface* serializationInterface) {
        logger::info("Serialization_Save begin");
        evt.Save(serializationInterface, 'MRPH', 1);

        logger::info("Serialization_Save end");
    }
    void Serialization_Load(SKSE::SerializationInterface* serializationInterface) {
        
        logger::info("Serialization_Load begin");
        std::uint32_t type;
        std::uint32_t version;
        std::uint32_t length;
        while (serializationInterface->GetNextRecordInfo(type, version, length)) {
            switch (type) {
                case 'MRPH' : {
                evt.Load(serializationInterface);
                break;}
            }
        }


        //logger::info("Serialization_Load end");
    }
    void Serialization_FormDelete(RE::VMHandle a_handle) { 
        logger::info("Serialization_delete begin");
        evt.Unregister(a_handle);
    }
    /**
     * Setup logging.
     *
     * <p>
     * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
     * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
     * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
     * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
     * (or <code>Skyrim VR</code> if you are using VR).
     * </p>
     */
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::level_enum::debug);
        log->flush_on(spdlog::level::level_enum::trace);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }
    
    
   std::vector<RE::TESForm*> eventSink{};
    std::vector<RE::ActiveEffect*> eventAMESink{};
    InterfaceExchangeMessage _nioInterface = InterfaceExchangeMessage{};
    IBodyMorphInterface* g_bodyMorphInterface;
    void updateModelWeight(RE::StaticFunctionTag* func, RE::TESObjectREFR* ref) {
        RE::BSScript::IVirtualMachine* registry = RE::SkyrimVM::GetSingleton()->impl.get();
        // skyVM->impl->DispatchMethodCall2();
        //RE::TESForm* aQuest = RE::TESDataHandler::GetSingleton()->LookupForm(0x0D70, "State of Dress.esp");
        evt.SendEvent(ref);
        
        g_bodyMorphInterface->UpdateModelWeight(ref);
        
    }
    void setBodyMorph(RE::StaticFunctionTag* func, RE::TESObjectREFR* ref, RE::BSFixedString morphName,
        RE::BSFixedString keyName, float avalue) {
        g_bodyMorphInterface->SetMorph(ref, morphName.c_str(), keyName.c_str(), avalue);
        
    }
    void RegisterForMorphEvent(RE::StaticFunctionTag*, RE::TESForm* aForm) { evt.Register(aForm); }

    void RegisterForMorphEventAME(RE::StaticFunctionTag*, RE::ActiveEffect* aForm) {
        evt.Register(aForm);
    }

    bool RegisterFuncs(RE::BSScript::IVirtualMachine* registry) {
        //registry->RegisterFunction("SetBodyMorph", "NiOverride", setBodyMorph, true);
        registry->RegisterFunction("UpdateModelWeight", "NiOverride", updateModelWeight, true);
        registry->RegisterFunction("RegisterForMorphEvent", "SoD_SKSE", RegisterForMorphEvent);
        registry->RegisterFunction("RegisterForMorphEventAME", "SoD_SKSE", RegisterForMorphEventAME);
        return true;
    }

    void InitializePapyrus() {

        log::trace("Initializing Papyrus binding...");
        if (GetPapyrusInterface()->Register(RegisterFuncs)) {
            log::debug("Papyrus functions bound.");
        } else {
            stl::report_and_fail("Failure to register Papyrus bindings.");
        }
    }
    void overwriteModelWeight() {
        if (_nioInterface.interfaceMap) {
            
            g_bodyMorphInterface =
                dynamic_cast<IBodyMorphInterface*>(_nioInterface.interfaceMap->QueryInterface("BodyMorph"));
            log::trace("Overwriting UpdateModelWeight()");
            if (GetPapyrusInterface()->Register([](RE::BSScript::IVirtualMachine* registry) {
                    registry->RegisterFunction("UpdateModelWeight", "NiOverride", updateModelWeight, true);
                    return true;
                })) {
                log::debug("UpdateModelWeight() overwritten");
            } else {
                stl::report_and_fail("Failure to overwrite.");
            }
            
        } else
            logger::debug("failed to retrieve interface");
    }
    bool newGame = false;
    inline constexpr REL::ID Vtbl(static_cast<std::uint64_t>(208040));
    void InitializeMessaging() {
        if (!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
            switch (message->type) {
                // Skyrim lifecycle events.
                case MessagingInterface::kPostLoad: // Called after all plugins have finished running SKSEPlugin_Load.
                    // It is now safe to do multithreaded operations, or operations against other plugins.
                case MessagingInterface::kPostPostLoad: {  // Called after all kPostLoad message handlers have run.
                    GetMessagingInterface()->Dispatch(InterfaceExchangeMessage::kMessage_ExchangeInterface,
                                                      &_nioInterface, sizeof(_nioInterface), nullptr);
                    
                    break;
                }
                case MessagingInterface::kInputLoaded: // Called when all game data has been found.
                    break;
                case MessagingInterface::kDataLoaded: {  // All ESM/ESL/ESP plugins have loaded, main menu is now
                                                         // active.
                    
                    break;
                }
                // Skyrim game events.
                case MessagingInterface::kNewGame: {  // Player starts a new game from main menu.
                    if (!g_bodyMorphInterface)
                    overwriteModelWeight();
                    
                    break;
                }
                case MessagingInterface::kPreLoadGame: // Player selected a game to load, but it hasn't loaded yet.
                    break;
                    // Data will be the name of the loaded save.
                case MessagingInterface::kPostLoadGame: {  // Player's selected save game has finished loading.
                    if (!g_bodyMorphInterface) overwriteModelWeight();
                    break;
                }
                    // Data will be a boolean indicating whether the load was successful.
                case MessagingInterface::kSaveGame: {  // The player has saved a game.
                    // Data will be the save name.
                    break;
                }
                case MessagingInterface::kDeleteGame: // The player deleted a saved game from within the load menu.
                    break;
            }
        })) {
            stl::report_and_fail("Unable to register message listener.");
        }
    }
}  // namespace

/**
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);



    Init(skse);
    InitializeMessaging();
    InitializePapyrus();
    // configuration
    //auto skyVM = RE::SkyrimVM::GetSingleton();
    //auto classVM = skyVM->impl->GetObjectHandlePolicy();
    
    //classVM->GetHandleForObject();
    g_serialization = GetSerializationInterface();
    g_serialization->SetUniqueID(_byteswap_ulong('SODS'));
    g_serialization->SetFormDeleteCallback(Serialization_FormDelete);
    g_serialization->SetSaveCallback(Serialization_Save);
    g_serialization->SetRevertCallback(Serialization_Revert);
    g_serialization->SetLoadCallback(Serialization_Load);
    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
