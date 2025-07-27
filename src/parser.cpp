#include "parser.h"
#include "utils.h"
#include <gumbo.h>
#include <iostream>

void LinkParser::extractLinks(const std::string& html, 
                             const std::string& base_url, 
                             std::queue<std::string>& url_queue, 
                             std::unordered_set<std::string>& visited) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::queue<GumboNode*> nodes;
    nodes.push(output->root);
    
    while (!nodes.empty()) {
        GumboNode* node = nodes.front();
        nodes.pop();
        
        if (node->type != GUMBO_NODE_ELEMENT) continue;
        
        GumboAttribute* href = nullptr;
        if (node->v.element.tag == GUMBO_TAG_A) {
            href = gumbo_get_attribute(&node->v.element.attributes, "href");
        }
        
        // Process child nodes
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            nodes.push(static_cast<GumboNode*>(children->data[i]));
        }
        
        if (!href) continue;
        
        std::string link_url = href->value;
        
        // Skip invalid links
        if (link_url.empty() || link_url[0] == '#') continue;
        
        // Resolve relative URLs
        std::string absolute_url = Utils::resolveUrl(base_url, link_url);
        
        // Only follow HTTP/HTTPS links
        if (absolute_url.find("http://") != 0 && absolute_url.find("https://") != 0) {
            continue;
        }
        
        // Add new URL to queue if not visited and within domain
        if (absolute_url.find(base_url) == 0 && visited.find(absolute_url) == visited.end()) {
            visited.insert(absolute_url);
            url_queue.push(absolute_url);
        }
    }
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}