#pragma once

#include "../data/EntityData.h"
#include "../data/EnemyStageData.h"
#include "../data/StageData.h"

#include <unordered_map>
#include <memory>

//Enum used for EntityData
enum AttackType
{
    SINGLE = 1,
    AREA,
};

/*z
* This class stores all units/enemies/stages' data.
* You can easily access this data by using the coresponding UID
*/

class DataLoader {
public:

    bool load_all();

    const std::shared_ptr<EntityData> get_unit_data(int uid) const;
    const std::shared_ptr<EntityData> get_enemy_data(int uid) const;
    const std::shared_ptr<StageData> get_stage_data(int uid) const;

private:

    bool load_units(const std::string& path);
    bool load_enemies(const std::string& path);
    bool load_stages(const std::string& path);

private:

    bool m_unitsLoaded = false;
    bool m_enemiesLoaded = false;
    bool m_stagesLoaded = false;

    std::unordered_map<int, std::shared_ptr<EntityData>> m_unitsDatabase;
    std::unordered_map<int, std::shared_ptr<EntityData>> m_enemiesDatabase;
    std::unordered_map<int, std::shared_ptr<StageData>> m_stagesDatabase;


};
