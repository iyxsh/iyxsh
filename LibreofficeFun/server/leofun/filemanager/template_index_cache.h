
#ifndef TEMPLATE_INDEX_CACHE_H
#define TEMPLATE_INDEX_CACHE_H

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <ctime>
#include <vector>
#include <rtl/ustring.hxx>

namespace filemanager {
    // 语言分组结构体
    struct CharacterInfo
    {
        std::string character;
        std::string pos;
    };

    struct CharacterBody {
        std::string bodyname;
        std::vector<CharacterInfo> characters;
    };

    struct LanguageGroup {
        std::string languageType;
        std::vector<CharacterBody> bodies;
    };

    // 文本字符信息结构体
    struct TextCharInfo
    {
        std::string character;    // 字符本身
        std::string pos;          // 所在工作表的位置
        std::string languageType; // 语言类型
        std::string bodyname;     // 所属 bodyname
    };

    // 查询索引结构体，支持分类预选+字符查询
    struct CharacterIndex {
        // languageType -> bodyname -> character -> CharacterInfo
        std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<CharacterInfo>>>> data;

        // 构建索引
        void build(const std::vector<LanguageGroup>& groups) {
            data.clear();
            for (const auto& group : groups) {
                std::unordered_set<std::string> seenBodies;
                for (const auto& body : group.bodies) {
                    // 只保留第一个出现的 bodyname，后续重复的丢弃
                    if (seenBodies.find(body.bodyname) != seenBodies.end()) {
                        continue;
                    }
                    seenBodies.insert(body.bodyname);
                    std::unordered_set<std::string> seenChars;
                    for (const auto& info : body.characters) {
                        // 只保留第一个出现的 character，后续重复的丢弃
                        if (seenChars.find(info.character) == seenChars.end()) {
                            data[group.languageType][body.bodyname][info.character].push_back(info);
                            seenChars.insert(info.character);
                        }
                        // 否则跳过
                    }
                }
            }
        }

        // 按语言类型、bodyname、character 查询
        std::vector<TextCharInfo> query(const std::string& languageType, const std::string& bodyname, const std::string& character) const {
            std::vector<TextCharInfo> result;
            auto langIt = data.find(languageType);
            if (langIt == data.end()) return result;
            auto bodyIt = langIt->second.find(bodyname);
            if (bodyIt == langIt->second.end()) return result;
            auto charIt = bodyIt->second.find(character);
            if (charIt == bodyIt->second.end()) return result;
            for (const auto& info : charIt->second) {
                result.push_back(TextCharInfo{info.character, info.pos, languageType, bodyname});
            }
            return result;
        }

        // 按 languageType 和 bodyname 查询所有字符
        std::vector<TextCharInfo> queryByBody(const std::string& languageType, const std::string& bodyname) const {
            std::vector<TextCharInfo> result;
            auto langIt = data.find(languageType);
            if (langIt == data.end()) return result;
            auto bodyIt = langIt->second.find(bodyname);
            if (bodyIt == langIt->second.end()) return result;
            for (const auto& charPair : bodyIt->second) {
                for (const auto& info : charPair.second) {
                    result.push_back(TextCharInfo{info.character, info.pos, languageType, bodyname});
                }
            }
            return result;
        }

        // 仅按字符查询（所有类型、所有 body）
        std::vector<TextCharInfo> queryAll(const std::string& character) const {
            std::vector<TextCharInfo> result;
            for (const auto& langPair : data) {
                const std::string& languageType = langPair.first;
                for (const auto& bodyPair : langPair.second) {
                    const std::string& bodyname = bodyPair.first;
                    auto charIt = bodyPair.second.find(character);
                    if (charIt != bodyPair.second.end()) {
                        for (const auto& info : charIt->second) {
                            result.push_back(TextCharInfo{info.character, info.pos, languageType, bodyname});
                        }
                    }
                }
            }
            return result;
        }

        // 获取所有索引内容
        std::vector<TextCharInfo> getAll() const {
            std::vector<TextCharInfo> result;
            for (const auto& langPair : data) {
                const std::string& languageType = langPair.first;
                for (const auto& bodyPair : langPair.second) {
                    const std::string& bodyname = bodyPair.first;
                    for (const auto& charPair : bodyPair.second) {
                        for (const auto& info : charPair.second) {
                            result.push_back(TextCharInfo{info.character, info.pos, languageType, bodyname});
                        }
                    }
                }
            }
            return result;
        }
    };

    struct TemplateIndexEntry {
        std::shared_ptr<CharacterIndex> index;
        time_t timestamp;
        time_t lastAccess;
        TemplateIndexEntry(std::shared_ptr<CharacterIndex> idx, time_t t)
            : index(idx), timestamp(t), lastAccess(t) {}
    };

    class TemplateIndexCacheManager {
public:
    static TemplateIndexCacheManager& getInstance();
    std::shared_ptr<CharacterIndex> getTemplateIndex(const rtl::OUString& filePath, const rtl::OUString& sheetName);
    void reloadTemplateIndex(const rtl::OUString& filePath, const rtl::OUString& sheetName);
    void monitorTemplateFile(const std::string& filePath, const std::string& sheetName);
    std::vector<TextCharInfo> getCharacterInfos(const rtl::OUString& filePath, const rtl::OUString& sheetName);
    bool isLoading() const;
    private:
        TemplateIndexCacheManager() = default;
        ~TemplateIndexCacheManager() = default;
        TemplateIndexCacheManager(const TemplateIndexCacheManager&) = delete;
        TemplateIndexCacheManager& operator=(const TemplateIndexCacheManager&) = delete;
        std::string buildCacheKey(const rtl::OUString& filePath, const rtl::OUString& sheetName);
        bool isCacheEntryExpired(const std::string& cacheKey) const;
        void removeOldestCacheEntry();
        std::unordered_map<std::string, std::unique_ptr<TemplateIndexEntry>> indexCache;
        mutable std::mutex cacheMutex;
        std::atomic<bool> loading{false};
    };
}

#endif // TEMPLATE_INDEX_CACHE_H
