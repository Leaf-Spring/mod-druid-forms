#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "SpellAuras.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

struct FormOption {
    uint32 displayId;
    std::string name;
};

struct FormCategory {
    std::vector<FormOption> both;
    std::vector<FormOption> alliance;
    std::vector<FormOption> horde;
};

// Almacén global de configuraciones leídas del .conf
static std::unordered_map<std::string, FormCategory> sFormConfigs;
static bool sIgnoreFaction = false;

// Almacén en RAM para las elecciones de la sesión actual: Map<GUID, Map<TipoForma, DisplayId>>
static std::unordered_map<uint32, std::unordered_map<std::string, uint32>> sActivePlayerForms;

// Mapeo unificado de hechizos a tipos de forma
const std::unordered_map<uint32, std::string> SPELL_TO_FORM = {
    {768,   "cat"},
    {5487,  "bear"},      
    {9634,  "bear"},      
    {783,   "travel"},
    {1066,  "aquatic"},
    {33891, "tree"},
    {24858, "moonkin"},
    {33943, "flight"},    
    {40120, "flight"}     
};

// Función para procesar los strings del archivo de configuración
void ParseConfigString(std::string const& str, std::vector<FormOption>& list) {
    list.clear();
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        if (token.empty()) continue;

        size_t colon = token.find(':');
        if (colon != std::string::npos) {
            uint32 id = std::stoul(token.substr(0, colon));
            std::string name = token.substr(colon + 1);
            list.push_back({id, name});
        }
    }
}

class DruidFormsWorldScript : public WorldScript {
public:
    DruidFormsWorldScript() : WorldScript("DruidFormsWorldScript") {}

    void OnAfterConfigLoad(bool /*reload*/) override {
        sIgnoreFaction = sConfigMgr->GetOption<bool>("DruidForms.IgnoreFaction", false);
        std::vector<std::string> formTypes = {"cat", "bear", "travel", "tree", "aquatic", "moonkin", "flight"};
        
        for (std::string const& type : formTypes) {
            FormCategory category;
            ParseConfigString(sConfigMgr->GetOption<std::string>("DruidForms." + type + ".Both", ""), category.both);
            ParseConfigString(sConfigMgr->GetOption<std::string>("DruidForms." + type + ".Alliance", ""), category.alliance);
            ParseConfigString(sConfigMgr->GetOption<std::string>("DruidForms." + type + ".Horde", ""), category.horde);
            sFormConfigs[type] = category;
        }
    }
};

// Función compartida para reaplicar la forma
void ReapplyForm(Player* player) {
    if (!player) return;
    uint32 guid = player->GetGUID().GetCounter();
    
    if (sActivePlayerForms.find(guid) == sActivePlayerForms.end()) return;

    for (auto const& pair : SPELL_TO_FORM) {
        if (player->HasAura(pair.first)) {
            std::string const& formName = pair.second;
            auto it = sActivePlayerForms[guid].find(formName);
            
            if (it != sActivePlayerForms[guid].end()) {
                uint32 displayId = it->second;
                if (displayId == 0) {
                    player->SetDisplayId(player->GetNativeDisplayId()); 
                } else {
                    player->SetDisplayId(displayId);
                }
                break;
            }
        }
    }
}

class DruidFormsPlayerScript : public PlayerScript {
public:
    DruidFormsPlayerScript() : PlayerScript("DruidFormsPlayerScript") {}

    void OnPlayerLogin(Player* player) override {
        uint32 guid = player->GetGUID().GetCounter();
        
        QueryResult result = CharacterDatabase.Query("SELECT form_type, display_id FROM character_druid_forms_selections WHERE guid = {}", guid);
        
        if (result) {
            do {
                Field* fields = result->Fetch();
                std::string formType = fields[0].Get<std::string>();
                uint32 displayId = fields[1].Get<uint32>();
                
                sActivePlayerForms[guid][formType] = displayId;
            } while (result->NextRow());
        }

        ReapplyForm(player);
    }

    void OnPlayerLogout(Player* player) override {
        sActivePlayerForms.erase(player->GetGUID().GetCounter());
    }

    void OnPlayerMapChanged(Player* player) override {
        ReapplyForm(player);
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override {
        ReapplyForm(player);
    }
};

class DruidFormsUnitScript : public UnitScript {
public:
    DruidFormsUnitScript() : UnitScript("DruidFormsUnitScript") {}

    void OnAuraApply(Unit* unit, Aura* aura) override {
        Player* player = unit->ToPlayer();
        if (!player) return;

        uint32 spellId = aura->GetId();
        auto formIt = SPELL_TO_FORM.find(spellId);
        if (formIt == SPELL_TO_FORM.end()) return;

        uint32 guid = player->GetGUID().GetCounter();
        auto playerConf = sActivePlayerForms.find(guid);
        if (playerConf == sActivePlayerForms.end()) return;

        auto selectionIt = playerConf->second.find(formIt->second);
        if (selectionIt == playerConf->second.end()) return;

        uint32 displayId = selectionIt->second;
        if (displayId == 0) {
            player->SetDisplayId(player->GetNativeDisplayId());
        } else {
            player->SetDisplayId(displayId);
        }
    }
};

using namespace Acore::ChatCommands;

class DruidFormsCommandScript : public CommandScript {
public:
    DruidFormsCommandScript() : CommandScript("DruidFormsCommandScript") {}

    ChatCommandTable GetCommands() const override {
        static ChatCommandTable commandTable = {
            { "cat_form",     HandleCatForm,     SEC_PLAYER, Console::No },
            { "bear_form",    HandleBearForm,    SEC_PLAYER, Console::No },
            { "travel_form",  HandleTravelForm,  SEC_PLAYER, Console::No },
            { "tree_form",    HandleTreeForm,    SEC_PLAYER, Console::No },
            { "moonkin_form", HandleMoonkinForm, SEC_PLAYER, Console::No },
            { "aquatic_form", HandleAquaticForm, SEC_PLAYER, Console::No },
            { "flight_form",  HandleFlightForm,  SEC_PLAYER, Console::No }
        };
        return commandTable;
    }

    static bool HandleGenericForm(ChatHandler* handler, char const* args, std::string const& formType) {
        Player* player = handler->GetSession()->GetPlayer();
        if (player->getClass() != CLASS_DRUID) {
            handler->SendSysMessage("Only Druids can use this command.");
            return true;
        }

        FormCategory const& category = sFormConfigs[formType];
        std::vector<FormOption> availableOptions = category.both;

        if (sIgnoreFaction) {
            availableOptions.insert(availableOptions.end(), category.alliance.begin(), category.alliance.end());
            availableOptions.insert(availableOptions.end(), category.horde.begin(), category.horde.end());
        } else {
            if (player->GetTeamId() == TEAM_ALLIANCE)
                availableOptions.insert(availableOptions.end(), category.alliance.begin(), category.alliance.end());
            else
                availableOptions.insert(availableOptions.end(), category.horde.begin(), category.horde.end());
        }

        if (availableOptions.empty()) {
            handler->SendSysMessage("No available options for this form.");
            return true;
        }

        if (!args || !*args) {
            handler->PSendSysMessage("Available {} options:", formType);
            for (size_t i = 0; i < availableOptions.size(); ++i) {
                handler->PSendSysMessage("[{}] - {}", i + 1, availableOptions[i].name);
            }
            handler->PSendSysMessage("Usage: .{}_form [number]", formType);
            return true;
        }

        int index = std::atoi(args) - 1;
        if (index < 0 || index >= static_cast<int>(availableOptions.size())) {
            handler->SendSysMessage("Invalid option number.");
            return true;
        }

        uint32 chosenId = availableOptions[index].displayId;
        uint32 guid = player->GetGUID().GetCounter();
        
        // Guardar la elección en RAM
        sActivePlayerForms[guid][formType] = chosenId;
        
        // Guardar en la base de datos de manera permanente
        CharacterDatabase.Execute("REPLACE INTO character_druid_forms_selections (guid, form_type, display_id) VALUES ({}, '{}', {})", guid, formType, chosenId);
        
        handler->PSendSysMessage("Form selected: {}", availableOptions[index].name);
        ReapplyForm(player); 
        return true;
    }

    static bool HandleCatForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "cat"); }
    static bool HandleBearForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "bear"); }
    static bool HandleTravelForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "travel"); }
    static bool HandleTreeForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "tree"); }
    static bool HandleMoonkinForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "moonkin"); }
    static bool HandleAquaticForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "aquatic"); }
    static bool HandleFlightForm(ChatHandler* handler, char const* args) { return HandleGenericForm(handler, args, "flight"); }
};

void AddSC_mod_druid_forms() {
    new DruidFormsWorldScript();
    new DruidFormsPlayerScript();
    new DruidFormsUnitScript();
    new DruidFormsCommandScript();
}
