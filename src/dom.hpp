#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

enum class NodeType {
    DOCUMENT,
    ELEMENT,
    TEXT,
    COMMENT
};

class DOMNode {
public:
    NodeType type;
    std::string tagName;                  // e.g., "h1", "p", "a", "div" (lowercase)
    std::string textContent;              // For TEXT nodes or raw text
    std::map<std::string, std::string> attributes; // e.g., href, src, style, class
    std::vector<std::shared_ptr<DOMNode>> children;
    std::weak_ptr<DOMNode> parent;

    DOMNode(NodeType t, const std::string& name = "") 
        : type(t), tagName(name) {}

    bool hasAttribute(const std::string& key) const {
        return attributes.find(key) != attributes.end();
    }

    std::string getAttribute(const std::string& key, const std::string& defaultVal = "") const {
        auto it = attributes.find(key);
        if (it != attributes.end()) return it->second;
        return defaultVal;
    }

    void setAttribute(const std::string& key, const std::string& val) {
        attributes[key] = val;
    }

    void addChild(std::shared_ptr<DOMNode> child) {
        children.push_back(child);
    }

    // Helper: collect all visible text from this node and its descendants
    std::string getInnerText() const {
        if (type == NodeType::TEXT) {
            return textContent;
        }
        std::string res;
        for (const auto& child : children) {
            res += child->getInnerText();
        }
        return res;
    }

    // Helper: search descendant elements by tag name
    std::shared_ptr<DOMNode> findFirstByTag(const std::string& tag) const {
        for (const auto& child : children) {
            if (child->type == NodeType::ELEMENT && child->tagName == tag) {
                return child;
            }
            auto found = child->findFirstByTag(tag);
            if (found) return found;
        }
        return nullptr;
    }
};
