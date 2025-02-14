#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

namespace xmlParser
{
    enum struct TokenType
    {
        TAG_OPEN,
        TAG_CLOSE,
        TEXT
    };

    struct xmlToken
    {
        TokenType kind;
        std::string value;
    };

    struct xmlNode : public std::enable_shared_from_this<xmlNode>
    {

        TokenType tagType;
        std::string tagName;
        std::weak_ptr<xmlNode> prev;
        std::vector<std::shared_ptr<xmlNode>> nodes;

        xmlNode() = default;

        xmlNode(TokenType n_tagType, const std::string &n_tagName, std::shared_ptr<xmlNode> parent)
            : tagType(n_tagType) ,tagName(n_tagName), prev(std::move(parent)) {}

        void addChild(std::shared_ptr<xmlNode> child);

        void printTree(int level = 0) const;

        std::vector<std::shared_ptr<xmlNode>> findAllNodes(std::string_view target);

        void findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results);
    };

    std::vector<xmlToken> tokenizeString(std::ifstream &input);
    std::shared_ptr<xmlNode> readXML(const std::string &filePath);
}