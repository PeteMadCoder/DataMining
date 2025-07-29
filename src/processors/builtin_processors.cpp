#include "builtin_processors.h"
#include <gumbo.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <queue>

ProcessedData GenericProcessor::process(const std::string& url, const std::string& html_content) {
    ProcessedData data;
    data.url = url;
    data.html_content = html_content;
    data.processed_time = std::chrono::system_clock::now();

    // Parse HTML with Gumbo
    GumboOutput* output = gumbo_parse(html_content.c_str());

    if (output && output->root) {
        std::queue<GumboNode*> nodes;
        nodes.push(output->root);

        std::ostringstream text_content;
        std::vector<std::string> links;
        std::vector<std::string> images;
        
        while (!nodes.empty()) {
            GumboNode* node = nodes.front();
            nodes.pop();
            
            if (node->type == GUMBO_NODE_ELEMENT) {
                // Extract title
                if (node->v.element.tag == GUMBO_TAG_TITLE) {
                    if (node->v.element.children.length > 0) {
                        GumboNode* text = static_cast<GumboNode*>(node->v.element.children.data[0]);
                        if (text->type == GUMBO_NODE_TEXT) {
                            data.title = text->v.text.text;
                        }
                    }
                }
                // Extract links
                else if (node->v.element.tag == GUMBO_TAG_A) {
                    GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
                    if (href) {
                        links.push_back(href->value);
                    }
                }
                // Extract images
                else if (node->v.element.tag == GUMBO_TAG_IMG) {
                    GumboAttribute* src = gumbo_get_attribute(&node->v.element.attributes, "src");
                    if (src) {
                        images.push_back(src->value);
                    }
                }
                // Extract text content
                else if (node->v.element.tag == GUMBO_TAG_P || 
                         node->v.element.tag == GUMBO_TAG_H1 ||
                         node->v.element.tag == GUMBO_TAG_H2 ||
                         node->v.element.tag == GUMBO_TAG_H3 ||
                         node->v.element.tag == GUMBO_TAG_H4 ||
                         node->v.element.tag == GUMBO_TAG_H5 ||
                         node->v.element.tag == GUMBO_TAG_H6) {
                    // Process child nodes for text
                    GumboVector* children = &node->v.element.children;
                    for (unsigned int i = 0; i < children->length; ++i) {
                        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
                        if (child->type == GUMBO_NODE_TEXT) {
                            text_content << child->v.text.text << " ";
                        }
                    }
                }
                
                // Process child nodes
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    nodes.push(static_cast<GumboNode*>(children->data[i]));
                }
            }
        }
        
        data.text_content = text_content.str();
        data.links = links;
        data.images = images;
    }

    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }

    return data;
}

ProcessedData TextProcessor::process(const std::string& url, const std::string& html_content) {
    GenericProcessor generic;
    ProcessedData data = generic.process(url, html_content);
    
    // Focus on text content extraction
    // Additional text processing could go here
    return data;
}

ProcessedData MetadataProcessor::process(const std::string& url, const std::string& html_content) {
    ProcessedData data;
    data.url = url;
    data.processed_time = std::chrono::system_clock::now();
    
    // Parse HTML with Gumbo
    GumboOutput* output = gumbo_parse(html_content.c_str());
    
    if (output && output->root) {
        std::queue<GumboNode*> nodes;
        nodes.push(output->root);
        
        while (!nodes.empty()) {
            GumboNode* node = nodes.front();
            nodes.pop();
            
            if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_META) {
                GumboAttribute* name_attr = gumbo_get_attribute(&node->v.element.attributes, "name");
                GumboAttribute* content_attr = gumbo_get_attribute(&node->v.element.attributes, "content");
                
                if (name_attr && content_attr) {
                    data.metadata[name_attr->value] = content_attr->value;
                }
                
                // Also check for property attribute (OpenGraph)
                GumboAttribute* property_attr = gumbo_get_attribute(&node->v.element.attributes, "property");
                if (property_attr && content_attr) {
                    data.metadata[property_attr->value] = content_attr->value;
                }
            }
            else if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_TITLE) {
                if (node->v.element.children.length > 0) {
                    GumboNode* text = static_cast<GumboNode*>(node->v.element.children.data[0]);
                    if (text->type == GUMBO_NODE_TEXT) {
                        data.title = text->v.text.text;
                    }
                }
            }
            
            // Process child nodes
            if (node->type == GUMBO_NODE_ELEMENT) {
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    nodes.push(static_cast<GumboNode*>(children->data[i]));
                }
            }
        }
    }
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return data;
}

ProcessedData LinkProcessor::process(const std::string& url, const std::string& html_content) {
    ProcessedData data;
    data.url = url;
    data.processed_time = std::chrono::system_clock::now();
    
    // Parse HTML with Gumbo
    GumboOutput* output = gumbo_parse(html_content.c_str());
    
    if (output && output->root) {
        std::queue<GumboNode*> nodes;
        nodes.push(output->root);
        
        std::vector<std::string> links;
        std::vector<std::string> images;
        
        while (!nodes.empty()) {
            GumboNode* node = nodes.front();
            nodes.pop();
            
            if (node->type == GUMBO_NODE_ELEMENT) {
                if (node->v.element.tag == GUMBO_TAG_A) {
                    GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
                    if (href) {
                        links.push_back(href->value);
                    }
                }
                else if (node->v.element.tag == GUMBO_TAG_IMG) {
                    GumboAttribute* src = gumbo_get_attribute(&node->v.element.attributes, "src");
                    if (src) {
                        images.push_back(src->value);
                    }
                }
                
                // Process child nodes
                GumboVector* children = &node->v.element.children;
                for (unsigned int i = 0; i < children->length; ++i) {
                    nodes.push(static_cast<GumboNode*>(children->data[i]));
                }
            }
        }
        
        data.links = links;
        data.images = images;
    }
    
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return data;
}