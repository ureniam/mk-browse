#include "html_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stack>
#include <unordered_set>

static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string HTMLParser::decodeEntities(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '&') {
            size_t semi = input.find(';', i);
            if (semi != std::string::npos && semi - i <= 10) {
                std::string entity = input.substr(i + 1, semi - i - 1);
                if (entity == "nbsp") { out += ' '; i = semi; continue; }
                if (entity == "amp") { out += '&'; i = semi; continue; }
                if (entity == "lt") { out += '<'; i = semi; continue; }
                if (entity == "gt") { out += '>'; i = semi; continue; }
                if (entity == "quot") { out += '"'; i = semi; continue; }
                if (entity == "apos" || entity == "#39") { out += '\''; i = semi; continue; }
                if (entity == "copy") { out += "(c)"; i = semi; continue; }
                if (entity == "reg") { out += "(R)"; i = semi; continue; }
                if (entity == "trade") { out += "(TM)"; i = semi; continue; }
                if (entity == "bull") { out += " - "; i = semi; continue; }
                if (entity == "ndash" || entity == "mdash") { out += "-"; i = semi; continue; }
                if (!entity.empty() && entity[0] == '#') {
                    int code = 0;
                    if (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X')) {
                        code = std::strtol(entity.substr(2).c_str(), nullptr, 16);
                    } else {
                        code = std::strtol(entity.substr(1).c_str(), nullptr, 10);
                    }
                    if (code > 0 && code < 128) {
                        out += static_cast<char>(code);
                        i = semi;
                        continue;
                    }
                }
            }
        }
        out += input[i];
    }
    return out;
}

std::string HTMLParser::extractTitle(const std::shared_ptr<DOMNode>& root) {
    if (!root) return "MK-Browse";
    auto titleNode = root->findFirstByTag("title");
    if (titleNode) {
        std::string title = trim(titleNode->getInnerText());
        if (!title.empty()) return title;
    }
    return "MK-Browse";
}

std::shared_ptr<DOMNode> HTMLParser::parse(const std::string& html) {
    auto root = std::make_shared<DOMNode>(NodeType::DOCUMENT, "#document");
    std::stack<std::shared_ptr<DOMNode>> nodeStack;
    nodeStack.push(root);

    static const std::unordered_set<std::string> voidTags = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };

    size_t pos = 0;
    const size_t len = html.size();

    while (pos < len) {
        // Find next tag
        size_t tagStart = html.find('<', pos);
        if (tagStart == std::string::npos) {
            // Remainder is text
            std::string text = html.substr(pos);
            text = decodeEntities(text);
            if (!trim(text).empty() || (text.find(' ') != std::string::npos)) {
                auto textNode = std::make_shared<DOMNode>(NodeType::TEXT, "#text");
                textNode->textContent = text;
                textNode->parent = nodeStack.top();
                nodeStack.top()->addChild(textNode);
            }
            break;
        }

        // Add text before tag
        if (tagStart > pos) {
            std::string text = html.substr(pos, tagStart - pos);
            text = decodeEntities(text);
            if (!trim(text).empty() || (text.find(' ') != std::string::npos)) {
                auto textNode = std::make_shared<DOMNode>(NodeType::TEXT, "#text");
                textNode->textContent = text;
                textNode->parent = nodeStack.top();
                nodeStack.top()->addChild(textNode);
            }
        }

        // Check for comment <!-- ... -->
        if (tagStart + 3 < len && html.substr(tagStart, 4) == "<!--") {
            size_t commentEnd = html.find("-->", tagStart + 4);
            if (commentEnd == std::string::npos) {
                pos = len; // Unterminated comment
            } else {
                pos = commentEnd + 3;
            }
            continue;
        }

        // Find end of tag
        size_t tagEnd = html.find('>', tagStart);
        if (tagEnd == std::string::npos) {
            pos = len;
            break;
        }

        std::string tagContent = html.substr(tagStart + 1, tagEnd - tagStart - 1);
        pos = tagEnd + 1;

        if (tagContent.empty()) continue;

        // Check DOCTYPE
        if (tagContent[0] == '!' || tagContent[0] == '?') {
            continue;
        }

        // Check for closing tag </tag>
        if (tagContent[0] == '/') {
            std::string closingTag = toLower(trim(tagContent.substr(1)));
            // Unwind stack until matching tag or root
            std::stack<std::shared_ptr<DOMNode>> temp;
            bool found = false;
            while (nodeStack.size() > 1) {
                if (nodeStack.top()->tagName == closingTag) {
                    found = true;
                    nodeStack.pop();
                    break;
                }
                temp.push(nodeStack.top());
                nodeStack.pop();
            }
            if (!found) {
                // Restore stack if tag not found
                while (!temp.empty()) {
                    nodeStack.push(temp.top());
                    temp.pop();
                }
            }
            continue;
        }

        // It is an opening or self-closing tag
        bool selfClosing = false;
        if (!tagContent.empty() && tagContent.back() == '/') {
            selfClosing = true;
            tagContent.pop_back();
        }

        // Parse tag name and attributes
        std::istringstream iss(tagContent);
        std::string tagName;
        iss >> tagName;
        tagName = toLower(trim(tagName));
        if (tagName.empty()) continue;

        auto elem = std::make_shared<DOMNode>(NodeType::ELEMENT, tagName);

        // Parse attributes: key="value" or key='value' or key=value or bare key
        std::string attrRemainder;
        std::getline(iss, attrRemainder);
        size_t aPos = 0;
        const size_t aLen = attrRemainder.size();

        while (aPos < aLen) {
            while (aPos < aLen && std::isspace(static_cast<unsigned char>(attrRemainder[aPos]))) aPos++;
            if (aPos >= aLen) break;

            size_t keyStart = aPos;
            while (aPos < aLen && !std::isspace(static_cast<unsigned char>(attrRemainder[aPos])) && 
                   attrRemainder[aPos] != '=' && attrRemainder[aPos] != '/' && attrRemainder[aPos] != '>') {
                aPos++;
            }
            std::string key = toLower(trim(attrRemainder.substr(keyStart, aPos - keyStart)));
            if (key.empty()) {
                aPos++;
                continue;
            }

            while (aPos < aLen && std::isspace(static_cast<unsigned char>(attrRemainder[aPos]))) aPos++;

            std::string value;
            if (aPos < aLen && attrRemainder[aPos] == '=') {
                aPos++; // Skip '='
                while (aPos < aLen && std::isspace(static_cast<unsigned char>(attrRemainder[aPos]))) aPos++;
                if (aPos < aLen) {
                    char quote = attrRemainder[aPos];
                    if (quote == '"' || quote == '\'') {
                        aPos++; // Skip quote
                        size_t valStart = aPos;
                        while (aPos < aLen && attrRemainder[aPos] != quote) aPos++;
                        value = attrRemainder.substr(valStart, aPos - valStart);
                        if (aPos < aLen && attrRemainder[aPos] == quote) aPos++;
                    } else {
                        size_t valStart = aPos;
                        while (aPos < aLen && !std::isspace(static_cast<unsigned char>(attrRemainder[aPos])) && attrRemainder[aPos] != '>') aPos++;
                        value = attrRemainder.substr(valStart, aPos - valStart);
                    }
                }
            }
            elem->setAttribute(key, decodeEntities(value));
        }

        // Check if script or style - we can skip script content
        if (tagName == "script") {
            size_t scriptEnd = html.find("</script>", pos);
            if (scriptEnd != std::string::npos) {
                pos = scriptEnd + 9;
            } else {
                size_t scriptEndCap = html.find("</SCRIPT>", pos);
                if (scriptEndCap != std::string::npos) pos = scriptEndCap + 9;
            }
            continue; // Do not add script node to DOM
        }

        // Add to parent
        elem->parent = nodeStack.top();
        nodeStack.top()->addChild(elem);

        // Check if void tag
        if (voidTags.count(tagName) > 0 || selfClosing) {
            // Do not push to stack
        } else {
            nodeStack.push(elem);
        }
    }

    return root;
}
