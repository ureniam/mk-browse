#pragma once
#include "dom.hpp"
#include <string>
#include <memory>
#include <vector>

class HTMLParser {
public:
    static std::shared_ptr<DOMNode> parse(const std::string& html);
    static std::string decodeEntities(const std::string& input);
    static std::string extractTitle(const std::shared_ptr<DOMNode>& root);
};
