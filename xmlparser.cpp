#include "xmlparser.hpp"
#include <ranges>
#include <stack>

namespace xmlParser
{

    enum class expects
    {
        LESS_THEN,
        GREATER_THEN,
        ATTRIBUTE_NAME,
        CLOSING_SLASH,
        SELF_CLOSING_SLASH,
        NODE_NAME,
        NODE_CLOSING_NAME,
        NODE_VALUE,
        ATTRIBUTE_VALUE,
        EQUALS,
        OPEN_QUATE,
        CLOSE_QUATE,
        NOT_EXPECTED
    };

    static bool isExpected(expects expected, char c)
    {
        using enum expects;
        switch (expected)
        {
        case LESS_THEN:
            return c == '<';
        case GREATER_THEN:
            return c == '>';

        case NODE_NAME:
        case NODE_CLOSING_NAME:
        case NODE_VALUE:
        case ATTRIBUTE_VALUE:
        case ATTRIBUTE_NAME:
            return c != '<' && c != '>' && c != '"' && c != '/' && c != '=';

        case EQUALS:
            return c == '=';

        case CLOSE_QUATE:
        case OPEN_QUATE:
            return c == '"';
        case CLOSING_SLASH:
        case SELF_CLOSING_SLASH:
            return c == '/';
        case NOT_EXPECTED:
            return false;
        }

        return false;
    }

    static std::vector<std::string> tokenize(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuote = false;
        bool inNode = false;

        for (size_t i = 0; i < line.size(); ++i)
        {
            char c = line[i];

            if (inQuote)
            {
                if (c == '"')
                {
                    tokens.push_back(current);
                    current.clear();
                    inQuote = false;
                }

                current += c;
            }
            else if (inNode)
            {
                if (c == '<')
                {
                    tokens.push_back(current);
                    tokens.push_back("<");
                    current.clear();
                    inNode = false;
                }
                else if (c == '\0')
                {
                    tokens.push_back(current);
                    current.clear();
                    inNode = false;
                }
                else
                {
                    current += c;
                }
            }
            else
            {
                if (std::isspace(c))
                {
                    if (!current.empty())
                    {
                        tokens.push_back(current);
                        current.clear();
                    }
                }
                else if (c == '<' || c == '>' || c == '=' || c == '/')
                {
                    if (!current.empty())
                    {
                        tokens.push_back(current);
                        current.clear();
                    }
                    tokens.push_back(std::string(1, c));

                    if (c == '>')
                    {
                        inNode = true;
                    }
                }
                else if (c == '"')
                {
                    if (!current.empty())
                    {
                        tokens.push_back(current);
                        current.clear();
                    }
                    tokens.push_back("\"");
                    inQuote = true;
                }
                else
                {
                    current += c;
                }
            }
        }

        if (!current.empty())
        {
            tokens.push_back(current);
        }

        return tokens;
    }

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
        if (contains(token.value, target))
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
            if (filter.field == child->token.value)
            {
                if (!child->nodes.empty() && child->nodes[0] != nullptr)
                {
                    if (filter.predicate(child->nodes[0]->token.value))
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
        if (contains(token.value, target))
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
        if (contains(token.value, target))
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
        switch (token.kind)
        {
        case TokenType::TAG_CLOSE:
            std::cout << std::string(level * 2, ' ') << "<" << this->token.value << ">\n";

            break;

        case TokenType::TAG_OPEN:
            std::cout << std::string(level * 2, ' ') << "<" << this->token.value << ">\n";

            break;

        case TokenType::META:
        case TokenType::TEXT:
            std::cout << std::string(level * 2, ' ') << this->token.value << '\n';

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
            tokens.push_back({TokenType::META, line, {}});

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
            tokens.push_back({TokenType::META, line, {}});
        }
        else
        {
            input.seekg(currsor);
        }

        expects expected[3] = {expects::LESS_THEN, expects::NOT_EXPECTED, expects::NOT_EXPECTED};
        size_t expectedSize = sizeof(expected) / sizeof(expected[0]);

        std::set<xmlAttribute> attributes;
        std::stack<std::string> nodeNames;
        xmlAttribute attr;
        std::string nodeValue;
        // MARK: TODO!!!
        while (std::getline(input, line))
        {
            const auto &words = tokenize(line);
            for (size_t i = 0; i < words.size(); ++i)
            {
                bool found = false;

                for (size_t exp = 0; exp < expectedSize; ++exp)
                {
                    if (isExpected(expected[exp], words[i][0]))
                    {
                        found = true;
                        expected[0] = expected[exp];
                        expected[1] = expects::NOT_EXPECTED;
                        expected[2] = expects::NOT_EXPECTED;
                        break;
                    }
                }
                if (!found)
                {
                    // std::cout << (int)expected << ' ' << (int)expected2 << '\n';
                    // for (const auto &elem : tokens)
                    // {
                    //     std::cout << elem << '\n';
                    // }
                    throw std::runtime_error("Invalid Formated Doccument :3, at Word:" + words[i]);
                }

                switch (expected[0])
                {
                case expects::LESS_THEN:
                    if (!nodeValue.empty())
                    {
                        tokens.push_back({TokenType::TEXT, nodeValue, {}});
                        nodeValue.clear();
                    }
                    expected[0] = expects::NODE_NAME;
                    expected[1] = expects::CLOSING_SLASH;
                    break;

                case expects::NODE_NAME:
                    tokens.push_back({TokenType::TAG_OPEN, words[i], {}});
                    expected[0] = expects::GREATER_THEN;
                    expected[1] = expects::ATTRIBUTE_NAME;
                    expected[2] = expects::SELF_CLOSING_SLASH;
                    ++openTokens;

                    break;

                case expects::NODE_VALUE:
                    nodeValue = nodeValue + words[i];
                    expected[0] = expects::LESS_THEN;
                    expected[1] = expects::NODE_VALUE;
                    break;

                case expects::GREATER_THEN:
                    if (!attr.value.empty() && !attr.name.empty())
                    {
                        const auto result = attributes.insert(attr);
                        if (!result.second)
                        {
                            throw std::runtime_error("Attribute " + words[i] + " is redifined!\n");
                        }
                        attr.name.clear();
                        attr.value.clear();
                    }

                    if (!attributes.empty())
                    {

                        tokens.back().attr = attributes;
                        attributes.clear();
                    }
                    expected[0] = expects::NODE_VALUE;
                    expected[1] = expects::LESS_THEN;
                    break;
                case expects::ATTRIBUTE_NAME:
                    if (!attr.value.empty() && !attr.name.empty())
                    {
                        const auto result = attributes.insert(attr);
                        if (!result.second)
                        {
                            throw std::runtime_error("Attribute " + words[i] + " is redifined!\n");
                        }
                        attr.name.clear();
                        attr.value.clear();
                    }
                    attr.name = words[i];
                    expected[0] = expects::EQUALS;
                    break;
                case expects::EQUALS:
                    expected[0] = expects::OPEN_QUATE;
                    break;
                case expects::OPEN_QUATE:
                    expected[0] = expects::ATTRIBUTE_VALUE;
                    break;
                case expects::ATTRIBUTE_VALUE:
                    attr.value = words[i];
                    expected[0] = expects::CLOSE_QUATE;
                    break;
                case expects::CLOSE_QUATE:
                    expected[0] = expects::GREATER_THEN;
                    expected[1] = expects::ATTRIBUTE_NAME;
                    expected[2] = expects::SELF_CLOSING_SLASH;
                    break;
                case expects::CLOSING_SLASH:
                    expected[0] = expects::NODE_CLOSING_NAME;
                    break;
                case expects::SELF_CLOSING_SLASH:
                    ++closeTokens;
                    tokens.push_back({TokenType::TAG_CLOSE, "/", {}});

                    expected[1] = expects::GREATER_THEN;                
                    break;
                case expects::NODE_CLOSING_NAME:
                    tokens.push_back({TokenType::TAG_CLOSE, "/" + words[i], {}});
                    expected[0] = expects::GREATER_THEN;
                    ++closeTokens;
                    break;
                
                case expects::NOT_EXPECTED:
                    throw std::runtime_error("Not expected");
                }
            }
        }

        if (closeTokens != openTokens)
        {
            std::cerr << "\033[1;31mWarning! Missing Opening Or Closing Tags\033[0m\nOpening " << openTokens << " Closing " << closeTokens << '\n';
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
            std::cerr << "Warning! File is empty!\n";
            return std::make_shared<xmlNode>();
        }

        std::shared_ptr<xmlNode> root = std::make_shared<xmlNode>(tokens[0], nullptr);
        std::shared_ptr<xmlNode> current = root;

        // Ugly ahh syntax for skipping 1 elem :)
        for (const auto &token : tokens | std::ranges::views::drop(1))
        {
            switch (token.kind)
            {
            case TokenType::TAG_OPEN:
            {

                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(token, current);
                current->addChild(child);
                current = child;

                break;
            }

            case TokenType::TAG_CLOSE:
            {

                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(token, root);
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
                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(token, current);
                current->addChild(child);
                break;
            }

            default:
                std::cerr << "Unreachable! How did we get here?\n";
                exit(EXIT_FAILURE);
            }
        }

        return root;
    }
}