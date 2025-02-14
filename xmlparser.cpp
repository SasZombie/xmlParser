#include "xmlparser.hpp"

namespace xmlParser
{

    void xmlNode::findNode(std::string_view target, std::vector<std::shared_ptr<xmlNode>> &results)
    {
        if (tagName == target)
        {
            results.push_back(shared_from_this());
        }

        for (const auto &child : nodes)
        {
            child->findNode(target, results);
        }
    }

    std::vector<std::shared_ptr<xmlNode>> xmlNode::findAllNodes(std::string_view target)
    {
        std::vector<std::shared_ptr<xmlNode>> results;

        findNode(target, results);

        return results;
    }

    void xmlNode::printTree(int level) const
    {
        std::cout << std::string(level * 2, ' ') << this->tagName << "\n";
        // if (!this->text.empty())
        // {
        //     std::cout << this->text << '\n';
        // }
        for (const auto &child : nodes)
        {
            child->printTree(level + 1);
        }
    }

    void xmlNode::addChild(std::shared_ptr<xmlNode> child)
    {
        child->prev = shared_from_this();
        nodes.push_back(std::move(child));
    }

    std::vector<xmlToken> tokenizeString(std::ifstream &input)
    {
        std::string line;

        std::vector<xmlToken> tokens;
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
                    ++size;

                    tokens.push_back({(line[start + 1] == '/') ? TokenType::TAG_CLOSE : TokenType::TAG_OPEN, line.substr(start, size)});
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

                    if (size == 0)
                        continue;
                    tokens.push_back({TokenType::TEXT, line.substr(start, size)});
                }
            }
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

        std::shared_ptr<xmlNode> root = std::make_shared<xmlNode>();
        std::shared_ptr<xmlNode> current = root;

        for (const auto &[kind, value] : tokens)
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

            case TokenType::TEXT:
            {
                std::shared_ptr<xmlNode> child = std::make_shared<xmlNode>(kind, value, current);
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