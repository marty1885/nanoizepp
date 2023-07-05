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

static const std::set<std::string> tags_cdata_allowed = {
    "svg", "math"
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

static std::string minimize_html_text(const std::string_view sv)
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
        minimized_text += std::string(text.substr(0, space));
        minimized_text += " ";

        text = text.substr(space);
        auto non_space = text.find_first_not_of(" \t\n\r");
        if(non_space == std::string_view::npos)
            break;
        text = text.substr(non_space);
    }
    // replace NUL characters with U+FFFD
    do {
        auto nul = minimized_text.find('\0');
        if(nul == std::string_view::npos)
            break;
        minimized_text.replace(nul, 1, "\xEF\xBF\xBD");
    } while(true);
    if(minimized_text == " ")
        minimized_text = "";
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
            break;
        remaining = remaining.substr(whitespace);
        // check if we are at the end of the tag
        if(remaining[0] == '>')
            break;
        
        // find the attribute name
        auto attribute_name_end = remaining.find_first_of(" \t\n\r=>");
        if(attribute_name_end == std::string_view::npos)
            break;
        auto attribute_name = remaining.substr(0, attribute_name_end);
        remaining = remaining.substr(attribute_name_end);
        // skip whitespaces
        whitespace = remaining.find_first_not_of(" \t\n\r");
        if(whitespace == std::string_view::npos)
            break;
        // We expect an = here
        if(remaining[whitespace] != '=') {
            remaining = remaining.substr(whitespace);
            if(attributes.contains(std::string(attribute_name)) == false)
                attributes[std::string(attribute_name)] = "";
            continue;
        }
        remaining = remaining.substr(whitespace + 1);
        // skip whitespaces
        whitespace = remaining.find_first_not_of(" \t\n\r");
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
            // pretend to have the quote at the end of the string
            if(attribute_value_end == std::string_view::npos) {
                attribute_value = remaining;
                remaining = std::string_view();
            }
            else {
                attribute_value = remaining.substr(0, attribute_value_end);
                remaining = remaining.substr(attribute_value_end + 1);
            }
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
    node_stack.reserve(32);
    node_stack.push_back(&document_root);
    std::string_view remaining_html = html;
    while(remaining_html.empty() == false) {
        if(node_stack.empty())
            throw std::runtime_error("Nanoize++: Internal error: node_stack is empty");
        auto current_node = node_stack.back();

        // skip whitespaces and see if we can find the start of a tag
        auto whitespace = remaining_html.find_first_not_of(" \t\n\r");
        // Only speces and newlines left, we are done
        if(whitespace == std::string_view::npos)
            break;
        // We found something, is it a start of a tag?
        if(remaining_html[0] == '<') {
            remaining_html = remaining_html.substr(whitespace).substr(1);
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
            // Is possible to be a incorrectly-opened-comment?
            if(remaining_html.size() >= 1 && remaining_html.starts_with("!")) {
                // Is it really a incorrectly-opened-comment? Try by checking if the following character is
                // not a [ (CDATA) or is DOCTYPE
                if(remaining_html.starts_with("![CDATA[") == false && remaining_html.starts_with("!DOCTYPE") == false) {
                    // It is, let's try to find the end of the comment
                    auto comment_end = remaining_html.find(">");
                    if(comment_end == std::string_view::npos) {
                        break;
                    }
                    else {
                        remaining_html = remaining_html.substr(comment_end + 1);
                        continue;
                    }
                }
                // Else it's something else, let's just skip it
            }
           
            // find the actual tag name
            auto tag_begin = remaining_html.find_first_not_of(" \t\n\r");
            if(tag_begin == std::string_view::npos) {
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<"));
                break;
            }
            remaining_html = remaining_html.substr(tag_begin);
            auto tag_end = remaining_html.find_first_of(" \t\n\r>[");
            if(tag_end == std::string_view::npos) {
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<"+std::string(remaining_html)));
                break;
            }
            std::string tag_name = std::string(remaining_html.substr(0, tag_end));
            remaining_html = remaining_html.substr(tag_end);
            bool is_self_closed = self_closed_tags.contains(tag_name);
            // Now, it's possible we met the </ div> tag, but we only parsed the </ part. But it's fine
            // because of auto closing.

            // Check if we got CDATA and handle it
            if(tag_name == "!" && remaining_html.starts_with("[CDATA")) {
                // CDATA
                auto cdata_end = remaining_html.find("]]>");
                std::string_view cdata = remaining_html.substr(7, cdata_end - 7);

                // are we in a tag allowed to have CDATA?
                bool allowed = false;
                for(auto it = node_stack.rbegin(); it != node_stack.rend(); ++it) {
                    if(tags_cdata_allowed.contains((*it)->tag)) {
                        allowed = true;
                        break;
                    }
                }

                remaining_html = remaining_html.substr(cdata_end + 3);
                if(!allowed)
                    continue;
                // Parsing CDATA is hard. Give up and just add it as a text node
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "<![CDATA["+std::string(cdata)+"]]>"));
                continue;
            }
            
            // parse attributes
            auto [remaining, attributes] = parse_attributes(remaining_html);
            remaining_html = remaining;
            if(is_self_closed) {
                if(tag_name == "!DOCTYPE" && !(attributes.size() == 1 && attributes.contains("html") && attributes["html"] == ""))
                    throw std::runtime_error("Only HTML5 is supported by nanoizepp");

                current_node->children.push_back(HTMLNode(tag_name, attributes));
                continue;
            }

            assert(tag_name.empty() == false);
            // Is it a closing tag?
            if(tag_name[0] == '/') {
                // is the tag valid?
                if(tag_name == "/") {
                    continue;
                }

                if(current_node->tag != tag_name.substr(1)) {
                    // This is tricky. We are closing a tag that is not the current tag. First we try to find the tag in the stack
                    // and close all tags in between. If we can't find it, we just ignore it.

                    // But special handling for <hX> tags. We can close them if the current tag is <hY> and abs(X-Y) <= 2
                    if(tag_name.size() == 3 && tag_name[1] == 'h' && std::isdigit(tag_name[2])) {
                        if(current_node->tag.size() == 3 && current_node->tag[1] == 'h' && std::isdigit(current_node->tag[2])) {
                            try {
                                auto x = std::stoi(tag_name.substr(2));
                                auto y = std::stoi(current_node->tag.substr(2));
                                if(std::abs(x-y) <= 2) {
                                    node_stack.pop_back();
                                    continue;
                                }
                            }
                            catch(...) { }
                        }
                    }

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
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", "&lt;"+tag_name+"&gt;"));
                continue;
            }
            if(tag_name[0] == '?') {
                continue;
            }
            if(tag_name.back() == '/') {
                tag_name.pop_back();
            }
            current_node->children.push_back(HTMLNode(tag_name, attributes));
            node_stack.push_back(&current_node->children.back());
        }
        else {
            auto text_end = remaining_html.find('<');
            if(text_end == std::string_view::npos) {
                // no more tags, just text
                std::string minimized_text = minimize_html_text(remaining_html);
                if(minimized_text.empty())
                    break;
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", minimized_text));
                break;
            }
            else {
                std::string_view text = remaining_html.substr(0, text_end);
                std::string minimized_text = minimize_html_text(text);
                remaining_html = remaining_html.substr(text_end);
                if(minimized_text.empty())
                    continue;
                current_node->children.push_back(HTMLNode("NANOIZEPP-PLAINTEXT", minimized_text));
            }
        }
        
    }
    return serialize_html_node(document_root, indent, newline);
}
