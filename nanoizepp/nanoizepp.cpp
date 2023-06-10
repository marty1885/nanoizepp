#include "nanoizepp.hpp"

#include <string>
#include <vector>
#include <map>
#include <string_view>
#include <stack>
#include <set>
#include <iostream>
#include <algorithm>
#include <cassert>

using namespace nanoizepp;

struct HTMLNode
{
    HTMLNode(std::string tag, std::map<std::string, std::string> attributes={}, std::vector<HTMLNode> children={})
        : tag(tag), attributes(attributes), children(children)
    {
        assert(tag != "");
    }

    HTMLNode(std::string tag, std::string text, std::map<std::string, std::string> attributes={}, std::vector<HTMLNode> children={})
        : tag(tag), text(text), attributes(attributes), children(children)
    {
        assert(tag != "");
    }
    std::string tag;
    std::string text;
    std::map<std::string, std::string> attributes;
    std::vector<HTMLNode> children;
};

static const std::set<std::string> self_closed_tags = {
    "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
    "meta", "param", "source", "track", "wbr", "!DOCTYPE"
};

static const std::set<std::string> tags_never_minimize_content = {
    "script", "style", "pre", "code", "textarea", "plaintext"
};

static std::string serialize_html_node(const HTMLNode& root, size_t indent, bool newline, std::string current = "", int depth = 0)
{
    bool is_text = root.tag == "NANOIZEPP-PLAINTEXT";
    if(depth != 0) {
        if(indent != 0)
            current += std::string(indent * (depth-1), ' ');
        if(is_text == false) {
            current += "<" + root.tag;
            for(const auto& [key, value] : root.attributes) {
                if(value == "") {
                    if(root.tag == "!DOCTYPE")
                        current += " " + key;
                }
                else
                    current += " " + key + "=\"" + value + "\"";
            }
            current += ">";
        }
        else {
            current += root.text + "";
        }
        if(newline)
            current += "\n";
    }
    for(const auto& child : root.children) {
        current = serialize_html_node(child, indent, newline, current, depth + 1);
    }
    if(depth != 0 && !is_text && self_closed_tags.contains(root.tag) == false) {
        if(indent != 0)
            current += std::string(indent * (depth-1), ' ');
        current += "</" + root.tag + ">";
        if(newline)
            current += "\n";
    }
    return current;
}

std::string minimize_html_text(const std::string_view sv)
{
    std::string_view text = sv;
    std::string minimized_text;
    bool last_was_space = false;
    while(text.empty() == false) {
        auto space = text.find_first_of(" \t\n\r");
        if(space == std::string_view::npos) {
            minimized_text += text;
            break;
        }
        auto end_space = text.substr(space).find_first_not_of(" \t\n\r");
        if(end_space == std::string_view::npos) {
            minimized_text += " ";
            break;
        }
        minimized_text += std::string(text.substr(0, space)) + " ";
        text = text.substr(space + end_space);
    }
    // replace NUL characters with U+FFFD
    do {
        auto nul = minimized_text.find('\0');
        if(nul == std::string_view::npos)
            break;
        minimized_text.replace(nul, 1, "\xEF\xBF\xBD");
    } while(true);
    return minimized_text;
}

/**
 * @brief Parse attributes from a tag without the tag name
 * @param sv String to parse attributes from ex: " id=\"test\" class=\"test\"> ..."
*/
static std::pair<std::string_view, std::map<std::string, std::string>> parse_attributes(const std::string_view sv)
{
    std::map<std::string, std::string> attributes;
    std::string_view remaining = sv;
    while(remaining.empty() == false) {
        // skip whitespaces and / (because HTML5 standard)
        auto whitespace = remaining.find_first_not_of(" \t\n\r/");
        if(whitespace == std::string_view::npos)
            throw std::runtime_error("Unexpected end of tag");
        remaining = remaining.substr(whitespace);
        // check if we are at the end of the tag
        if(remaining[0] == '>')
            break;
        
        // find the attribute name
        auto attribute_name_end = remaining.find_first_of(" \t\n\r=>");
        if(attribute_name_end == std::string_view::npos)
            throw std::runtime_error("Unexpected end of tag");
        auto attribute_name = remaining.substr(0, attribute_name_end);
        remaining = remaining.substr(attribute_name_end);
        // skip whitespaces and =
        whitespace = remaining.find_first_not_of(" \t\n\r=");
        if(whitespace == std::string_view::npos)
            throw std::runtime_error("Unexpected end of tag");
        remaining = remaining.substr(whitespace);
        // check if we are at the end of the tag
        if(remaining[0] == '>') {
            attributes[std::string(attribute_name)] = "";
            break;
        }
        // now we should be at the start of the attribute value
        std::string_view attribute_value;
        if(remaining[0] != '"') {
            // attribute value is not quoted, find the next whitespace
            auto attribute_value_end = remaining.find_first_of(" \t\n\r>");
            attribute_value = remaining.substr(0, attribute_value_end);
            remaining = remaining.substr(attribute_value_end);
        }
        else {
            // attribute value is quoted, find the next quote
            remaining = remaining.substr(1);
            auto attribute_value_end = remaining.find_first_of("\"");
            if(attribute_value_end == std::string_view::npos)
                throw std::runtime_error("Unexpected end of tag");
            attribute_value = remaining.substr(0, attribute_value_end);
            remaining = remaining.substr(attribute_value_end + 1);
        }

        if(attributes.contains(std::string(attribute_name)))
            continue;
        attributes[std::string(attribute_name)] = std::string(attribute_value);
    }

    if(remaining.empty() == false && remaining[0] == '>')
        remaining = remaining.substr(1);
    return {remaining, attributes};
}

std::string nanoizepp::nanoize(const std::string_view html, size_t indent, bool newline)
{
    HTMLNode document_root("NANOIZEPP-ROOT");
    std::vector<HTMLNode*> node_stack;
    node_stack.push_back(&document_root);
    std::string_view remaining_html = html;
    while(remaining_html.empty() == false) {
        // try to skip whitespace
        auto whitespace = remaining_html.find_first_not_of(" \t\n\r");
        if(whitespace == std::string_view::npos)
            break;

        remaining_html = remaining_html.substr(whitespace);
        if(node_stack.empty())
            throw std::runtime_error("Nanoize++: Internal error: node_stack is empty");
        auto current_node = node_stack.back();
        // We found something, is it a start of a tag?
        if(remaining_html[0] == '<') {
            remaining_html = remaining_html.substr(1);
            if(remaining_html.empty()) {
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<"));
                break;
            }

            // Is it a comment?
            if(remaining_html.size() >= 3 && remaining_html.starts_with("!--")) {
                // check if it is `abrupt-closing-of-empty-comment` (<!-->)
                auto sv = remaining_html.substr(3);
                auto possibe_end = sv.find_first_not_of("-");
                if(possibe_end == std::string_view::npos)
                    break;
                if(sv[possibe_end] == '>') {
                    remaining_html = sv.substr(possibe_end + 1);
                    continue;
                }

                // It's not, let's try to find the end of the comment
                auto comment_end = remaining_html.find("-->");
                size_t end_size = 3;
                if(comment_end == std::string_view::npos) {
                    comment_end = remaining_html.find("--!>");
                    end_size = 4;
                }


                if(comment_end == std::string_view::npos) {
                    break;
                }
                else {
                    remaining_html = remaining_html.substr(comment_end + end_size);
                    continue;
                }
            }
           
            // find the actual tag name
            auto tag_end = remaining_html.find_first_of(" \t\n\r>");
            if(tag_end == std::string_view::npos) {
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<"+std::string(remaining_html)));
                break;
            }
            std::string tag_name = std::string(remaining_html.substr(0, tag_end));
            remaining_html = remaining_html.substr(tag_end);
            bool is_self_closed = self_closed_tags.contains(tag_name);
            
            // parse attributes
            auto [remaining, attributes] = parse_attributes(remaining_html);
            remaining_html = remaining;
            if(is_self_closed) {
                current_node->children.push_back(HTMLNode(tag_name, attributes));
                continue;
            }

            // Is it a closing tag?
            if(tag_name[0] == '/') {
                // is the tag valid?
                if(tag_name == "/") {
                    continue;
                }

                if(current_node->tag != tag_name.substr(1)) {
                    // This is tricky. We are closing a tag that is not the current tag. First we try to find the tag in the stack
                    // and close all tags in between. If we can't find it, we just ignore it.
                    auto it = std::find_if(node_stack.rbegin(), node_stack.rend(), [&](auto& node) {
                        return node->tag == tag_name.substr(1);
                    });
                    if(it == node_stack.rend()) {
                        continue;
                    }
                    node_stack.resize(std::distance(it, node_stack.rend()));
                    continue;
                }
                node_stack.pop_back();
                continue;
            }

            // Special handling for <script>, <pre>, <style> and alike
            if(tags_never_minimize_content.contains(tag_name)) {
                auto end_tag = remaining_html.find("</"+tag_name+">");
                if(end_tag == std::string_view::npos) {
                    current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<"+tag_name));
                    break;
                }
                current_node->children.push_back(HTMLNode(tag_name, attributes));
                current_node->children.back().children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", std::string(remaining_html.substr(0, end_tag))));
                remaining_html = remaining_html.substr(end_tag + tag_name.size() + 3);
                continue;
            }
            // is the tag valid?
            if(std::isdigit(tag_name[0]) == true) {
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", tag_name));
                continue;
            }
            if(tag_name[0] == '?') {
                continue;
            }
            if(tag_name.back() == '/') {
                tag_name.pop_back();
            }
            HTMLNode new_node(tag_name, attributes);
            current_node->children.push_back(new_node);
            node_stack.push_back(&current_node->children.back());
        }
        else {
            auto text_end = remaining_html.find('<');
            if(text_end == std::string_view::npos) {
                // no more tags, just text
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", minimize_html_text(remaining_html)));
                break;
            }
            else {
                std::string_view text = remaining_html.substr(0, text_end);
                std::string minimized_text = minimize_html_text(text);
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", minimized_text));
                remaining_html = remaining_html.substr(text_end);
            }
        }
        
    }
    return serialize_html_node(document_root, indent, newline);
}