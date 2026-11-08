#include "ProjectData.h"

#include <algorithm>
#include <fstream>

namespace
{
    size_t index_of(DataFile file) { return static_cast<size_t>(file); }
}

// ==================
// Load / Save
// ==================
ProjectData::ProjectData(std::filesystem::path projectFolder) :
    m_projectFolder(std::move(projectFolder))
{
}

bool ProjectData::load()
{
    m_error.clear();

    for (size_t i = 0; i < FILE_COUNT; ++i)
    {
        const std::filesystem::path path = get_data_folder() / file_name(static_cast<DataFile>(i));
        std::ifstream file(path);
        if (!file)
        {
            m_error = "Can't open " + path.string();
            return false;
        }

        try
        {
            m_documents[i] = ordered_json::parse(file);
        }
        catch (const std::exception& e)
        {
            m_error = path.string() + ": " + e.what();
            return false;
        }

        m_committed[i] = m_documents[i];
        m_isUnsaved[i] = false;
    }

    m_undoStack.clear();
    m_redoStack.clear();
    return true;
}

bool ProjectData::save()
{
    m_error.clear();

    for (size_t i = 0; i < FILE_COUNT; ++i)
    {
        if (!m_isUnsaved[i]) continue;

        // Same as the game's save: write a temporary file, then replace the real one
        const std::filesystem::path path = get_data_folder() / file_name(static_cast<DataFile>(i));
        const std::filesystem::path tempPath = path.string() + ".tmp";
        {
            std::ofstream file(tempPath, std::ios::trunc);
            if (!file)
            {
                m_error = "Can't write " + tempPath.string();
                return false;
            }
            file << m_documents[i].dump(4) << "\n";
        }

        std::error_code error;
        std::filesystem::rename(tempPath, path, error);
        if (error)
        {
            m_error = "Can't replace " + path.string() + ": " + error.message();
            return false;
        }

        m_isUnsaved[i] = false;
    }
    return true;
}

ordered_json& ProjectData::get(DataFile file)
{
    return m_documents[index_of(file)];
}

const ordered_json& ProjectData::get(DataFile file) const
{
    return m_documents[index_of(file)];
}

// ==================
// Undo / Redo
// ==================
void ProjectData::commit_edit(DataFile file)
{
    const size_t i = index_of(file);
    if (m_documents[i] == m_committed[i]) return; // Nothing actually changed

    m_undoStack.push_back({ file, m_committed[i], m_documents[i] });
    if (m_undoStack.size() > MAX_UNDO_STEPS) m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();

    m_committed[i] = m_documents[i];
    m_isUnsaved[i] = true;
}

void ProjectData::commit_all()
{
    for (size_t i = 0; i < FILE_COUNT; ++i) commit_edit(static_cast<DataFile>(i));
}

bool ProjectData::undo()
{
    if (m_undoStack.empty()) return false;

    Edit edit = m_undoStack.back();
    m_undoStack.pop_back();

    const size_t i = index_of(edit.file);
    m_documents[i] = edit.before;
    m_committed[i] = edit.before;
    m_isUnsaved[i] = true;

    m_redoStack.push_back(std::move(edit));
    return true;
}

bool ProjectData::redo()
{
    if (m_redoStack.empty()) return false;

    Edit edit = m_redoStack.back();
    m_redoStack.pop_back();

    const size_t i = index_of(edit.file);
    m_documents[i] = edit.after;
    m_committed[i] = edit.after;
    m_isUnsaved[i] = true;

    m_undoStack.push_back(std::move(edit));
    return true;
}

bool ProjectData::can_undo() const { return !m_undoStack.empty(); }
bool ProjectData::can_redo() const { return !m_redoStack.empty(); }

// ==================
// State
// ==================
bool ProjectData::has_unsaved_changes() const
{
    return std::any_of(m_isUnsaved.begin(), m_isUnsaved.end(), [](bool isUnsaved) { return isUnsaved; });
}

bool ProjectData::is_unsaved(DataFile file) const
{
    return m_isUnsaved[index_of(file)];
}

const std::string& ProjectData::get_error() const
{
    return m_error;
}

// ==================
// Paths
// ==================
const std::filesystem::path& ProjectData::get_project_folder() const
{
    return m_projectFolder;
}

std::filesystem::path ProjectData::get_data_folder() const
{
    return m_projectFolder / "game_data";
}

std::filesystem::path ProjectData::resolve(const std::string& relativePath) const
{
    return m_projectFolder / relativePath;
}

bool ProjectData::exists(const std::string& relativePath) const
{
    std::error_code error;
    return !relativePath.empty() && std::filesystem::is_regular_file(resolve(relativePath), error);
}

const char* ProjectData::file_name(DataFile file)
{
    switch (file)
    {
    case DataFile::UNITS:    return "units.json";
    case DataFile::ENEMIES:  return "enemies.json";
    case DataFile::STAGES:   return "stages.json";
    case DataFile::CHAPTERS: return "chapters.json";
    case DataFile::STATUSES: return "statuses.json";
    default:                 return "";
    }
}

// ==================
// Entries by UID
// ==================
std::string ProjectData::uid_key(int uid)
{
    return "UID_" + std::to_string(uid);
}

std::string ProjectData::find_key(DataFile file, int uid) const
{
    for (const auto& [key, value] : get(file).items())
    {
        if (value.is_object() && value.contains("UID") && value["UID"].is_number_integer() && value["UID"].get<int>() == uid)
            return key;
    }
    return "";
}

ordered_json* ProjectData::find_uid(DataFile file, int uid)
{
    for (auto& [key, value] : get(file).items())
    {
        if (value.is_object() && value.contains("UID") && value["UID"].is_number_integer() && value["UID"].get<int>() == uid)
            return &value;
    }
    return nullptr;
}

const ordered_json* ProjectData::find_uid(DataFile file, int uid) const
{
    return const_cast<ProjectData*>(this)->find_uid(file, uid);
}

std::vector<int> ProjectData::sorted_uids(DataFile file) const
{
    std::vector<int> uids;
    for (const auto& [key, value] : get(file).items())
    {
        if (value.is_object() && value.contains("UID") && value["UID"].is_number_integer())
            uids.push_back(value["UID"].get<int>());
    }
    std::sort(uids.begin(), uids.end());
    return uids;
}

int ProjectData::next_free_uid(DataFile file) const
{
    const std::vector<int> uids = sorted_uids(file);
    int uid = 1;
    while (std::find(uids.begin(), uids.end(), uid) != uids.end()) ++uid;
    return uid;
}
