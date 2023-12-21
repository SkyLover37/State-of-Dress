
#include "version.h"
#include <stddef.h>
#include <cmath>
#include <RenderManager.h>
using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;
/*
    I originally made this for like 2 papyrus functions, then kept adding more. Now it's a horrendous monster and I am too lazy to split the code up.
*/
namespace {
    const std::string PapyrusClassName = "State of Dress";
    constexpr std::uint32_t serializationDataVersion = 1;
    const SKSE::SerializationInterface* g_serialization;
    std::vector<RE::BGSKeyword*> kwCov;
    std::vector<RE::BGSKeyword*> kwLoc;
    std::vector<RE::BGSKeyword*> kwBlock;
    RE::TESForm* tempvar;
    clock_t morphTimer{};
    SKSE::RegistrationSet<const RE::TESObjectREFR*> evt{"morphCatch"sv};
    clock_t transformTimer{};
    SKSE::RegistrationSet<const RE::TESObjectREFR*> evTrans{"transformCatch"sv};
    void Serialization_Revert(SKSE::SerializationInterface* serializationInterface) {
        // logger::info("Serialization_revert begin");
        evt.Revert(serializationInterface);
        evTrans.Revert(serializationInterface);
    }
    void Serialization_Save(SKSE::SerializationInterface* serializationInterface) {
        // logger::info("Serialization_Save begin");
        evt.Save(serializationInterface, 'MRPH', 1);
        evt.Save(serializationInterface, 'TFRM', 1);
        if (tempvar) {
            if (!serializationInterface->OpenRecord('SODT', 1)) 
                logger::info("Failed to open SODT record");
                
            else 
                if (!serializationInterface->WriteRecordData(tempvar->formID))
                    logger::info("failed to write formID");
        }
        // logger::info("Serialization_Save end");
    }
    void Serialization_Load(SKSE::SerializationInterface* serializationInterface) {
        // logger::info("Serialization_Load begin");
        std::uint32_t type;
        std::uint32_t version;
        std::uint32_t length;
        while (serializationInterface->GetNextRecordInfo(type, version, length)) {
            switch (type) {
                case 'MRPH': {
                    evt.Load(serializationInterface);
                    break;
                }
                case 'TFRM': {
                    evTrans.Load(serializationInterface);
                    break;
                }
                case 'SODT': {
                    RE::FormID oldID;
                    serializationInterface->ReadRecordData(oldID);
                    logger::info("{:x}",  oldID);
                    break;
                }
            }
        }

        // logger::info("Serialization_Load end");
    }
    void Serialization_FormDelete(RE::VMHandle a_handle) {
        // logger::info("Serialization_delete begin");
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
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::level_enum::debug);
        log->flush_on(spdlog::level::level_enum::trace);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    InterfaceExchangeMessage _nioInterface = InterfaceExchangeMessage{};
    IBodyMorphInterface* g_bodyMorphInterface;
    INiTransformInterface* g_transformInterface; 
    void updateModelWeight(RE::StaticFunctionTag* func, RE::TESObjectREFR* ref) {
        if (!ref) return;
        g_bodyMorphInterface->UpdateModelWeight(ref);
        RE::BSScript::IVirtualMachine* registry = RE::SkyrimVM::GetSingleton()->impl.get();
        //logger::debug("modelWeight updated for {}", ref->GetBaseObject()->GetFormEditorID());
        // skyVM->impl->DispatchMethodCall2();
        // RE::TESForm* aQuest = RE::TESDataHandler::GetSingleton()->LookupForm(0x0D70, "State of Dress.esp");
        if (morphTimer < clock() / CLOCKS_PER_SEC) {
            morphTimer = clock();
            evt.SendEvent(ref);
        }

    }
    void UpdateNodeTransform(RE::StaticFunctionTag*, RE::TESObjectREFR* aRef, bool firstPerson, bool isFemale,
                             RE::BSFixedString nodeName) { 
        if (!aRef) return;
        g_transformInterface->UpdateNodeTransforms(aRef, firstPerson, isFemale, nodeName.c_str());
        RE::BSScript::IVirtualMachine* registry = RE::SkyrimVM::GetSingleton()->impl.get();
        // skyVM->impl->DispatchMethodCall2();
        // RE::TESForm* aQuest = RE::TESDataHandler::GetSingleton()->LookupForm(0x0D70, "State of Dress.esp");
        if (transformTimer < clock() / CLOCKS_PER_SEC) {
            transformTimer = clock();
            evTrans.SendEvent(aRef);
        }
    }
    void setBodyMorph(RE::StaticFunctionTag* func, RE::TESObjectREFR* ref, RE::BSFixedString morphName,
                      RE::BSFixedString keyName, float avalue) {
        if (!ref) return;
        g_bodyMorphInterface->SetMorph(ref, morphName.c_str(), keyName.c_str(), avalue);
    }
    void RegisterForMorphEvent(RE::StaticFunctionTag*, RE::TESForm* aForm) {
        if (!aForm) return;
        evt.Register(aForm);
    }

    void RegisterForMorphEventAME(RE::StaticFunctionTag*, RE::ActiveEffect* aForm) {
        if (!aForm) return;
        evt.Register(aForm);
    }
    void RegisterForTransformEvent(RE::StaticFunctionTag*, RE::TESForm* aForm) { evTrans.Register(aForm); }

    void RegisterForTransformEventAME(RE::StaticFunctionTag*, RE::ActiveEffect* aForm) { evTrans.Register(aForm); }
    float GetFinalMorphValue(RE::StaticFunctionTag*, RE::TESObjectREFR* aRef, RE::BSFixedString morphName) {
        if (!aRef) return 0.0f;
        // logger::info("Checking for {} and retrieved {}", morphName.c_str(),
        //              g_bodyMorphInterface->GetBodyMorphs(aRef, morphName.c_str()));
        return g_bodyMorphInterface->GetBodyMorphs(aRef, morphName.c_str());
    }
    int getWornKeywordCount(RE::StaticFunctionTag*, RE::Actor* akActor, RE::BGSKeyword* akKeyword) {
        if (!akActor || !akKeyword) {
            logger::warn("Recieved an empty argument for GetWornKeywordCount");
            return -1;
        }
        int a_result = 0;
        auto inv = akActor->GetInventory();
        for (auto& elem : inv) {
            if (elem.first->Is(RE::FormType::LeveledItem)) {
                continue;
            }
            const auto& [count, entry] = elem.second;
            if (count > 0 && entry->IsWorn()) {
                if (elem.first->As<RE::TESForm>()->GetFormType() == RE::FormType::Armor &&
                    elem.first->As<RE::BGSKeywordForm>()->HasKeyword(akKeyword)) {
                    a_result++;
                }
            }
        }

        // RE::SCRIPT_FUNCTION::

        return a_result;
    };
    RE::BGSListForm* Blocking = nullptr;
    RE::BGSListForm* CoverageList = nullptr;
    RE::BGSListForm* LocationList = nullptr;
    RE::BGSListForm* Presets = nullptr;
    RE::BGSListForm* Bondage = nullptr;
    RE::BGSListForm* Modesty = nullptr;
    std::vector<RE::BGSKeyword*> getPresetsKeywords(RE::StaticFunctionTag*, std::uint16_t index) {
        std::vector<RE::BGSKeyword*> kwList{};
        if (!Presets || !Presets->ContainsOnlyType(RE::BGSListForm::FORMTYPE) || index < 0 || index >= Presets->forms.size() ||
            !Presets->forms[index]->As<RE::BGSListForm>()->ContainsOnlyType(RE::BGSKeyword::FORMTYPE))
            return kwList;

        Presets->forms[index]->As<RE::BGSListForm>()->ForEachForm([&kwList](RE::TESForm& entry) {
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return kwList;
    }
    std::vector<RE::BGSKeyword*> getBondageKeywords(RE::StaticFunctionTag*) {
        std::vector<RE::BGSKeyword*> kwList{};
        if (!Bondage || !Bondage->ContainsOnlyType(RE::BGSKeyword::FORMTYPE)) return kwList;

        Bondage->ForEachForm([&kwList](RE::TESForm& entry) {
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return kwList;
    }
    std::vector<RE::BGSKeyword*> getCoverageKeywords(RE::StaticFunctionTag*) { 
        std::vector<RE::BGSKeyword*> kwList{};
        if (!CoverageList || !CoverageList->ContainsOnlyType(RE::BGSKeyword::FORMTYPE))
            return kwList;
        
        CoverageList->ForEachForm([&kwList](RE::TESForm& entry) { 
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue; });
        return kwList;
    }
    std::vector<RE::BGSKeyword*> getLocationKeywords(RE::StaticFunctionTag*) {
        std::vector<RE::BGSKeyword*> kwList{};
        if (!LocationList || !LocationList->ContainsOnlyType(RE::BGSKeyword::FORMTYPE)) return kwList;

        LocationList->ForEachForm([&kwList](RE::TESForm& entry) {
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return kwList;
    }
    std::vector<RE::BGSKeyword*> getModestyKeywords(RE::StaticFunctionTag*, std::uint16_t index) {
        std::vector<RE::BGSKeyword*> kwList{};
        if (!Modesty || !Presets->ContainsOnlyType(RE::BGSListForm::FORMTYPE) || index < 0 ||
            index >= Modesty->forms.size() ||
            !Modesty->forms[index]->As<RE::BGSListForm>()->ContainsOnlyType(RE::BGSKeyword::FORMTYPE))
            return kwList;

        Modesty->forms[index]->As<RE::BGSListForm>()->ForEachForm([&kwList](RE::TESForm& entry) {
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return kwList;
    }
    std::vector<RE::BGSKeyword*> getBlockingKeywords(RE::StaticFunctionTag*, std::uint32_t index) {
        std::vector<RE::BGSKeyword*> kwList{};
        if (!Blocking || !Blocking->ContainsOnlyType(RE::BGSListForm::FORMTYPE) || index < 0 ||
            index >= Modesty->forms.size() ||
            !Modesty->forms[index]->As<RE::BGSListForm>()->ContainsOnlyType(RE::BGSKeyword::FORMTYPE))
            return kwList;

        Blocking->ForEachForm([&kwList](RE::TESForm& entry) {
            kwList.push_back(entry.As<RE::BGSKeyword>());
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return kwList;
    }
    
    std::vector<RE::TESObjectARMO*> getArmorWithKeywords(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> aKeywords, bool matchAll, bool strict, bool allowEnchanted) {
        
        std::vector<RE::TESObjectARMO*> armor{};
        if (aKeywords.empty()) return armor;
        auto forms = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectARMO>();
        //Copy keyword vector
        std::vector<RE::BGSKeyword*> exclude = kwCov;
        for (auto kw : aKeywords) {
            //Remove wanted keywords
            if (!kw) continue;
            exclude.erase(std::find(exclude.begin(), exclude.end(), kw));
        }
        for (auto form : forms) {
            if (!form) continue;
            //logger::debug("Checking {}", form->GetFormEditorID());
            
            
            if (!form->GetPlayable()) {
                logger::debug("Is not playable, skipping");
                continue;
            }
            if (!allowEnchanted && form->As<RE::TESEnchantableForm>()->formEnchanting) {
                //logger::debug("{} has {} enchantments ignoring", form->GetFormEditorID(),
                              //form->As<RE::TESEnchantableForm>()->amountofEnchantment);
                continue;
            }
            
            if ((!strict || !form->HasKeywordInArray(exclude, false)) && form->HasKeywordInArray(aKeywords, matchAll)) {
                logger::debug("Adding {} from {} to list", form->GetName(), form->GetFile()->GetFilename());
                armor.push_back(form->As<RE::TESObjectARMO>());
            }
               
            
        }
        return armor;
    }
    std::vector<RE::TESObjectARMO*> getArmorWithoutKeywords(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> aKeywords,
                                                         bool matchAll, bool strict, bool allowEnchanted) {
        std::vector<RE::TESObjectARMO*> armor{};
        if (aKeywords.empty()) return armor;
        auto forms = RE::TESDataHandler::GetSingleton()->GetFormArray(RE::FormType::Armor);
        // Copy keyword vector
        std::vector<RE::BGSKeyword*> exclude = kwCov;
        for (auto kw : aKeywords) {
            // Remove wanted keywords
            if (!kw) continue;
            exclude.erase(std::find(exclude.begin(), exclude.end(), kw));
        }
       
        for (auto form : forms) {
            if (!form) continue;
            if (!form->GetPlayable()) {
                logger::debug("Is not playable, skipping");
                continue;
            }
            if (!allowEnchanted && form->As<RE::TESEnchantableForm>()->amountofEnchantment > 0)
                logger::debug("{} has {} enchantments ignoring", form->GetFormEditorID(), form->As<RE::TESEnchantableForm>()->amountofEnchantment);
            if ((!strict || form->HasKeywordInArray(exclude, false)) && !form->HasKeywordInArray(aKeywords, matchAll)) armor.push_back(form->As<RE::TESObjectARMO>());
            
        }
        return armor;
    }
    std::vector<RE::TESObjectARMO*> filterArmorArray(RE::StaticFunctionTag*, std::vector<RE::TESObjectARMO*> aArmor,
        std::vector<RE::BGSKeyword*> aKeywords, bool matchAll) {
        if (aArmor.empty() || aKeywords.empty()) return aArmor;
        
        for (auto iter = aArmor.begin(); iter != aArmor.end();) {
            auto form = *iter;
            if (!form) {
                ++iter;
                continue;
            }
            
            if (!form->HasKeywordInArray(aKeywords, matchAll)) {
                //logger::debug("Removing {}", form->GetFormEditorID());
                iter = aArmor.erase(iter);
            }
            else
                ++iter;
        }
        return aArmor;
    }
    std::vector<RE::TESObjectARMO*> filterArmorArrayByPrice(RE::StaticFunctionTag*, std::vector<RE::TESObjectARMO*> aArmor,
                                                     int min, int max) {
        if (aArmor.empty()) return aArmor;
        
        for (auto iter = aArmor.begin(); iter != aArmor.end();) {
            auto form = *iter;
            if (!form) {
                ++iter;
                continue;
            }

            if (form->GetGoldValue() < min || form->GetGoldValue() > max) {
                //logger::debug("Removing {}", form->GetFormEditorID());
                iter = aArmor.erase(iter);
            } else
                ++iter;
        }
        return aArmor;
    }
    std::vector<RE::TESObjectARMO*> filterArmorArrayEx(RE::StaticFunctionTag*, std::vector<RE::TESObjectARMO*> aArmor,
                                                     std::vector<RE::BGSKeyword*> aKeywords, bool matchAll) {
        if (aArmor.empty() || aKeywords.empty()) return aArmor;
        
        for (auto iter = aArmor.begin(); iter != aArmor.end();) {
            auto form = *iter;
            if (!form) {
                ++iter;
                continue;
            }
            
            if (form->HasKeywordInArray(aKeywords, matchAll)) {
                //logger::debug("Removing {}", form->GetFormEditorID());
                iter = aArmor.erase(iter);
            } else
                ++iter;
        }
        return aArmor;
    }
    // float GetFinalTransformValue(RE::StaticFunctionTag*, RE::TESObjectREFR* aRef, RE::BSFixedString nodeName) {
    //     if (!aRef) return 0.0f;
    //     return g_transformInterface->Get
    // }
    void testFunc(RE::StaticFunctionTag*, RE::TESForm* am){ 
        logger::info("Setting tempvar");
        tempvar = am;
    }
    bool RegisterFuncs(RE::BSScript::IVirtualMachine* registry) {
        //registry->RegisterFunction("SetBodyMorph", "NiOverride", setBodyMorph, true);
        registry->RegisterFunction("RegisterForTransformEvent", "SoD_SKSE", RegisterForTransformEvent);
        registry->RegisterFunction("RegisterForTransformEventAME", "SoD_SKSE", RegisterForTransformEventAME);
        //registry->RegisterFunction("GetFinalTransformValue", "SoD_SKSE", GetFinalMorphValue, true);
        registry->RegisterFunction("getWornKeywordCount", "SoD_SKSE", getWornKeywordCount);
        registry->RegisterFunction("RegisterForMorphEvent", "SoD_SKSE", RegisterForMorphEvent);
        registry->RegisterFunction("RegisterForMorphEventAME", "SoD_SKSE", RegisterForMorphEventAME);
        registry->RegisterFunction("GetFinalMorphValue", "SoD_SKSE", GetFinalMorphValue, true);
        registry->RegisterFunction("getArmorWithKeywords", "SoD_SKSE", getArmorWithKeywords, true);
        registry->RegisterFunction("getArmorWithoutKeywords", "SoD_SKSE", getArmorWithoutKeywords);
        registry->RegisterFunction("filterArmorArray", "SoD_SKSE", filterArmorArray);
        registry->RegisterFunction("filterArmorArrayEx", "SoD_SKSE", filterArmorArrayEx);

        registry->RegisterFunction("getPresetsKeywords", "SoD_SKSE", getPresetsKeywords);
        registry->RegisterFunction("getBlockingKeywords", "SoD_SKSE", getBlockingKeywords);
        registry->RegisterFunction("getModestyKeywords", "SoD_SKSE", getModestyKeywords);
        registry->RegisterFunction("getLocationKeywords", "SoD_SKSE", getLocationKeywords);
        registry->RegisterFunction("getCoverageKeywords", "SoD_SKSE", getCoverageKeywords);
        registry->RegisterFunction("getBondageKeywords", "SoD_SKSE", getBondageKeywords);
        registry->RegisterFunction("filterArmorArrayByPrice", "SoD_SKSE", filterArmorArrayByPrice);

        registry->RegisterFunction("testFunc", "SoD_SKSE", testFunc);
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
            g_transformInterface =
                dynamic_cast<INiTransformInterface*>(_nioInterface.interfaceMap->QueryInterface("NiTransform"));
            if (GetPapyrusInterface()->Register([](RE::BSScript::IVirtualMachine* registry) {
                    if (g_bodyMorphInterface)
                        registry->RegisterFunction("UpdateModelWeight", "NiOverride", updateModelWeight, true);
                    else
                        logger::error("Failed to retrieve Morph interface. SoD will continue working, but may not update body sizes reliably");

                    if (g_transformInterface)
                        registry->RegisterFunction("UpdateNodeTransform", "NiOverride", UpdateNodeTransform, true);
                    else
                        logger::error("Failed to retrieve Transform interface. SoD will continue working, but may not update body sizes reliably");
                    return true;
                })) {
                log::debug("Finished overwriting functions");
            } else {
                stl::report_and_fail("Failure to overwrite.");
            }
            
        } else
            logger::debug("failed to retrieve interface");
    }
    bool newGame = false;
    //inline constexpr REL::ID Vtbl(static_cast<std::uint64_t>(208040));
    class menuRet : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
        using notify = RE::BSEventNotifyControl;

    public:
        static menuRet* getSingleton() { 
            static menuRet* object;
            if (!object)
                object = new menuRet();
            return object;
        }
        menuRet(){ RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);
        }
        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) {
            
            if (a_event) 
            {
                if (a_event->menuName == "Book Menu" && a_event->opening) {
                    logger::debug("{} Opening {}", a_event->menuName.c_str(), a_event->opening);

                    auto task = []() {
                        auto movie = RE::UI::GetSingleton()->GetMovieView("Book Menu");

                        RE::GFxValue gfStr;
                        RE::GFxValue gfBoolean;
                        gfStr.SetString("Hello world");
                        gfBoolean.SetBoolean(false);
                        RE::GFxValue bookContent[]{gfStr, gfBoolean};
                        // movie->CreateArray(&bookContent);
                        logger::debug("created gfx object");

                        // movie->CreateString(&gfStr, "Hello World");
                        // bookContent.PushBack(&gfStr);
                        // bookContent.PushBack(RE::GFxValue(false));
                        RE::GFxValue bookReturn;

                        movie->InvokeNoReturn("SetBookText", bookContent, 2);
                    };
                    SKSE::GetTaskInterface()->AddUITask(task);
                }
            }
            
            return notify::kContinue;
        }
    };
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

                    menuRet::getSingleton();
                    
                    break;
                case MessagingInterface::kDataLoaded: {  // All ESM/ESL/ESP plugins have loaded, main menu is now
                                                         // active.
                    auto data = RE::TESDataHandler::GetSingleton();
                    auto sod = data->LookupModByName("State of Dress.esp");
                    auto keywords = data->GetFormArray<RE::BGSKeyword>();
                    if (Blocking = data->LookupForm<RE::BGSListForm>(RE::FormID(0x86F), "State of Dress.esp")) logger::debug("Retrieved blocking form list with {} items", Blocking->forms.size());
                    if (Bondage = data->LookupForm<RE::BGSListForm>(RE::FormID(0x872), "State of Dress.esp")) logger::debug("Retrieved Bondage list with {} items", Bondage->forms.size());
                    if (Modesty = data->LookupForm<RE::BGSListForm>(RE::FormID(0x873), "State of Dress.esp")) logger::debug("Retrieved Modesty list with {} items", Modesty->forms.size());
                    if (Presets = data->LookupForm<RE::BGSListForm>(RE::FormID(0x865), "State of Dress.esp")) logger::debug("Retrieved Presets list with {} items", Presets->forms.size());
                    if (CoverageList = data->LookupForm<RE::BGSListForm>(RE::FormID(0x867), "State of Dress.esp")) logger::debug("Retrieved CoverageList with {} items", CoverageList->forms.size());
                    if (LocationList = data->LookupForm<RE::BGSListForm>(RE::FormID(0x868), "State of Dress.esp")) logger::debug("Retrieved LocationList with {} items", LocationList->forms.size());
                    //
                    for (auto entry : keywords) {
                        if (!entry || !sod->IsFormInMod(entry->GetFormID())) continue;

                        std::string editorID = entry->GetFormEditorID();
                        logger::debug("Checking {}", editorID);
                        if (size_t pos = editorID.find("Cov"); pos > 4 && pos < 9) {
                            logger::debug("{}(pos {}) is a coverage kw", editorID, pos);
                            kwCov.push_back(entry);
                            continue;
                        }
                        if (size_t pos = editorID.find("Loc"); pos > 4 && pos < 9) {
                            logger::debug("{}(pos {}) is a location kw", editorID, pos);
                            kwLoc.push_back(entry);
                            continue;
                        }
                        if (size_t pos = editorID.find("Block"); pos > 4 && pos < 9) {
                            logger::debug("{}(pos {}) is a block kw", editorID, pos);
                            kwBlock.push_back(entry);
                            continue;
                        }
                    }
                    logger::debug("Cov: {} | Loc: {} | Block: {}", kwCov.size(), kwLoc.size(), kwBlock.size());
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

    //RenderManager::Install();
    return true;
}