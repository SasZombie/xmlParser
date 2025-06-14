#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <ranges>
#include <algorithm>
#include <memory>
#include <set>

namespace xmlParser
{
    enum struct TokenType
    {
        TAG_OPEN,
        TAG_CLOSE,
        TEXT,
        ATTRIBUTE,
        META
    };

    struct xmlAttribute
    {
        std::string name;
        std::string value;

        bool operator<(const xmlAttribute &other) const
        {
            return name < other.name;
        }
    };

    struct xmlToken
    {
        TokenType kind;
        std::string value;
        std::set<xmlAttribute> attr;
    };

    struct nodeFilter
    {
        std::string field;
        std::function<bool(std::string_view)> predicate;
    };

    struct xmlNode : public std::enable_shared_from_this<xmlNode>
    {

        xmlToken token;
        std::weak_ptr<xmlNode> prev;
        std::vector<std::shared_ptr<xmlNode>> nodes;

        xmlNode() = default;

        xmlNode(const xmlToken &n_token, std::shared_ptr<xmlNode> parent) noexcept
            : token(n_token), prev(std::move(parent)) {}

        void addChild(std::shared_ptr<xmlNode> child) noexcept;

        void printTree(int level = 0) const noexcept;

        std::vector<std::shared_ptr<xmlNode>> findAllNodes(std::string_view target) noexcept;
        bool checkRec(const nodeFilter &filter) const noexcept;

        void findAll(std::vector<std::shared_ptr<xmlNode>> &results) const noexcept;
        void findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results) noexcept;
        void findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results, const nodeFilter &filter) noexcept;
        void findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results, const std::vector<nodeFilter> &filters) noexcept;
        std::vector<std::shared_ptr<xmlNode>> findAllNodes(std::string_view target, const nodeFilter &filter) noexcept;
        std::vector<std::shared_ptr<xmlNode>> findAllNodes(std::string_view target, const std::vector<nodeFilter> &filters) noexcept;
    };

    std::vector<xmlToken> tokenizeString(std::ifstream &input);
    std::shared_ptr<xmlNode> readXML(const std::string &filePath);
} // namespace xmlParser