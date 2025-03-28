#include "xmlparser.hpp"
#include <ranges>

namespace xmlParser
{

    // tagName
    // tagName siAdnorato="BLaLBalBla"
    // tagNames siAdnorato="BLaLBalBla"
    // Where, what
    static bool contains(std::string_view str1, std::string_view str2) noexcept
    {
        if (str1 == str2)
            return true;
        size_t len1 = str1.length(), len2 = str2.length();

        if (len1 <= len2)
            return false;

        size_t ind = 0;

        while (ind < len2 && str1[ind] == str2[ind])
        {
            ++ind;
        }

        return (ind == len1 && str1[ind + 1] == ' ');
    }

    void xmlNode::findAll(std::vector<std::shared_ptr<xmlNode>> &results) const noexcept
    {
        for (const auto &child : nodes)
        {
            results.push_back(child);
            child->findAll(results);
        }
    }

    void xmlNode::findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results) noexcept
    {
        if (contains(tagName, target))
        {
            results.push_back(shared_from_this());
            findAll(results);
        }

        for (const auto &child : nodes)
        {
            child->findNode(target, results);
        }
    }
    // books/book
    // book = field1, field2, field3

    bool xmlNode::checkRec(const nodeFilter &filter) const noexcept
    {
        for (const auto &child : nodes)
        {
            if (filter.field == child->tagName)
            {
                if (!child->nodes.empty() && child->nodes[0] != nullptr)
                {
                    if (filter.predicate(child->nodes[0]->tagName))
                    {
                        return true;
                    }
                }
            }

            if (child->checkRec(filter))
            {
                return true;
            }
        }

        return false;
    }

    void xmlNode::findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results, const nodeFilter &filter) noexcept
    {
        if (contains(tagName, target))
        {

            if (checkRec(filter))
            {
                results.push_back(shared_from_this());
                findAll(results);
            }
        }
        for (const auto &child : nodes)
        {
            child->findNode(target, results, filter);
        }
    }

    void xmlNode::findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results, const std::vector<nodeFilter> &filters) noexcept
    {
        if (contains(tagName, target))
        {
            for (const auto &filter : filters)
            {

                if (!checkRec(filter))
                {
                    return;
                }
            }
            results.push_back(shared_from_this());
            findAll(results);
        }

        for (const auto &child : nodes)
        {
            child->findNode(target, results, filters);
        }
    }

    std::vector<std::shared_ptr<xmlNode>> xmlNode::findAllNodes(std::string_view target) noexcept
    {
        std::vector<std::shared_ptr<xmlNode>> results;
        size_t len = target.size();

        if (target[0] == '<' && target[len - 1] == '>')
            findNode(target.substr(1, len - 2), results);
        else
            findNode(target, results);

        return results;
    }

    std::vector<std::shared_ptr<xmlNode>> xmlNode::findAllNodes(std::string_view target, const nodeFilter &filter) noexcept
    {
        std::vector<std::shared_ptr<xmlNode>> results;
        size_t len = target.size();

        if (target[0] == '<' && target[len - 1] == '>')
            findNode(target.substr(1, len - 2), results, filter);
        else
            findNode(target, results, filter);

        return results;
    }

    std::vector<std::shared_ptr<xmlNode>> xmlNode::findAllNodes(std::string_view target, const std::vector<nodeFilter> &filters) noexcept
    {
        std::vector<std::shared_ptr<xmlNode>> results;
        size_t len = target.size();

        if (target[0] == '<' && target[len - 1] == '>')
            findNode(target.substr(1, len - 2), results, filters);
        else
            findNode(target, results, filters);

        return results;
    }

    void xmlNode::printTree(int level) const noexcept
    {
        switch (tagType)
        {
        case TokenType::TAG_CLOSE:
            std::cout << std::string(level * 2, ' ') << "</" << this->tagName << ">\n";

            break;

        case TokenType::TAG_OPEN:
            std::cout << std::string(level * 2, ' ') << "<" << this->tagName << ">\n";

            break;

        case TokenType::META:
        case TokenType::TEXT:
            std::cout << std::string(level * 2, ' ') << this->tagName << '\n';

            break;
        default:
            std::cerr << "Unreachable!\n";
            break;
        }
        for (const auto &child : nodes)
        {
            child->printTree(level + 1);
        }
    }

    void xmlNode::addChild(std::shared_ptr<xmlNode> child) noexcept
    {
        child->prev = shared_from_this();
        nodes.push_back(std::move(child));
    }

    std::vector<xmlToken> tokenizeString(std::ifstream &input)
    {
        size_t openTokens = 0;
        size_t closeTokens = 0;
        size_t currsor = 0;

        std::string line;
        std::vector<xmlToken> tokens;

        std::getline(input, line);
        // XML version
        if (line.size() > 2 && line[1] == '?' && line.back() == '>')
        {
            tokens.push_back({TokenType::META, line});

            currsor = input.tellg();
        }
        else
        {
            input.seekg(0);
        }

        std::getline(input, line);
        // Doctype version
        if (line.find("DOCTYPE") != std::string::npos)
        {
            tokens.push_back({TokenType::META, line});
        }
        else
        {
            input.seekg(currsor);
        }

        while (std::getline(input, line))
        {
            size_t lineSize = line.size();
            for (size_t i = 0; i < lineSize; ++i)
            {
                if (line[i] == '<')
                {
                    size_t size = 0;
                    size_t start = i;

                    while (i < lineSize && line[i] != '>')
                    {
                        ++size;
                        ++i;
                    }
                    if (line[i] != '>')
                    {
                        throw std::runtime_error("Unclosed tag at " + line + "\nInvalid formated Document\n");
                    }
                    ++size;

                    TokenType currentType;

                    if (line[start + 1] == '/')
                    {
                        ++closeTokens;
                        currentType = TokenType::TAG_CLOSE;
                        // Dropping the </ and >
                        tokens.push_back({currentType, line.substr(start + 1, size - 2)});
                    }
                    else
                    {
                        ++openTokens;
                        currentType = TokenType::TAG_OPEN;
                        // Dropping the </ and >
                        tokens.push_back({currentType, line.substr(start + 1, size - 2)});
                    }
                }
                else
                {
                    while (i < lineSize && (line[i] == ' ' || line[i] == '\n' || line[i] == '\r'))
                    {
                        ++i;
                    }

                    size_t size = 0;
                    size_t start = i;

                    while (i < lineSize && line[i] != '<')
                    {
                        ++size;
                        ++i;
                    }
                    --i;

                    if (size != 0)
                    {
                        tokens.push_back({TokenType::TEXT, line.substr(start, size)});
                    }
                }
            }
        }

        if (closeTokens != openTokens)
        {
            std::cerr << "\033[1;31mWarning! Missing Opening Or Closing Tags\033[0m\n";
        }

        return tokens;
    }

    std::shared_ptr<xmlNode> readXML(const std::string &filePath)
    {
        std::ifstream input(filePath);

        if (!input.is_open())
        {
            throw std::runtime_error("Unable to open file");
        }

        std::vector<xmlToken> tokens = tokenizeString(input);

        if (tokens.size() == 0)
        {
            std::cerr << "File is empty!\n";
            return std::make_shared<xmlNode>();
        }

        // std::shared_ptr<xmlNode> root = std::make_shared<xmlNode>(tokens[0].kind, tokens[0].value, nullptr);
        std::shared_ptr<xmlNode> root = std::make_shared<xmlNode>();
        std::shared_ptr<xmlNode> current = root;

        // Ugly ahh syntax for skipping 1 elem :)
        for (const auto &[kind, value] : tokens /* | std::ranges::views::drop(1) */)
        {
            switch (kind)
            {
            case TokenType::TAG_OPEN:
            {

                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(kind, value, current);
                current->addChild(child);
                current = child;

                break;
            }

            case TokenType::TAG_CLOSE:
            {

                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(kind, value, root);
                current->addChild(child);
                if (auto parent = current->prev.lock())
                {
                    current = parent;
                }
                break;
            }

            case TokenType::META:
            case TokenType::TEXT:
            {
                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(kind, value, current);
                current->addChild(child);
                break;
            }

                // case TokenType::META:
                // {
                //     std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(kind, value, current);

                // }

            default:
                std::cerr << "Unreachable! How did we get here?\n";
                exit(EXIT_FAILURE);
            }
        }

        return root;
    }
}